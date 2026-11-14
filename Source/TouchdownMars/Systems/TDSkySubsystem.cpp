// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
#include "Systems/TDSkySubsystem.h"

#include "Actors/TDPlayerController.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "Misc/CommandLine.h"
#include "Model/TDTypes.h"
#include "Systems/TDMissionSubsystem.h"

namespace TDSky
{
	// Dust-storm look: multipliers on the clear sky. Optical depth goes from about 0.45 to about 2 in the Jezero
	// regional storm of January 2022 (Lemmon et al. 2022), so the dust is about 4.4 times thicker; the rest are
	// designer values (look only).
	constexpr float StormMieScattering = 4.4f;
	constexpr float StormMieAbsorption = 9.0f;      // storm dust also darkens the sky (look value)
	constexpr float StormFog = 5.0f;
	constexpr float StormFogStartCm = 400.f;
	constexpr float StormSunIntensity = 0.35f;
	constexpr float StormSunAngleDeg = 2.5f;
	constexpr float StormSkyLight = 0.6f;
	const FLinearColor StormHaze(0.32f, 0.19f, 0.11f);
	constexpr float DustEasePerSecond = 0.12f;
	constexpr float IdleHours = 15.5f;     // outside a mission: a mid-afternoon sky
	constexpr float MinElevationDeg = 4.f; // keep the scene lit for play
	// Blue glow around a low sun (M_TD_SkyGlow, Scripts/make_sky_glow.py): weak by day, full near sunset, mostly
	// gone in a dust storm (designer values).
	constexpr float GlowDay = 0.25f;
	constexpr float GlowSunset = 2.2f;
	constexpr float GlowFullBelowDeg = 6.f;
	constexpr float GlowStartBelowDeg = 25.f;

	double Sin(double Deg) { return FMath::Sin(FMath::DegreesToRadians(Deg)); }
	double Cos(double Deg) { return FMath::Cos(FMath::DegreesToRadians(Deg)); }
}

bool UTDSkySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

TStatId UTDSkySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDSkySubsystem, STATGROUP_Tickables);
}

void UTDSkySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	FindSkyActors();
	SkyParams = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/TouchdownMars/Materials/MPC_TD_Sky.MPC_TD_Sky"));
#if !UE_BUILD_SHIPPING
	FParse::Value(FCommandLine::Get(), TEXT("TDTime="), FixedHours);
	FParse::Value(FCommandLine::Get(), TEXT("TDDust="), FixedDust);
#endif
	if (FixedDust >= 0.f)
	{
		Dust = FMath::Clamp(FixedDust, 0.f, 1.f);
	}
}

void UTDSkySubsystem::FindSkyActors()
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (UDirectionalLightComponent* Light = It->FindComponentByClass<UDirectionalLightComponent>(); Light && Light->IsUsedAsAtmosphereSunLight() && !Sun.IsValid())
		{
			Sun = Light;
			Light->SetMobility(EComponentMobility::Movable);
			BaseSunIntensity = Light->Intensity;
			BaseSunAngle = Light->LightSourceAngle;
		}
		if (USkyAtmosphereComponent* Atmosphere = It->FindComponentByClass<USkyAtmosphereComponent>(); Atmosphere && !Sky.IsValid())
		{
			Sky = Atmosphere;
			BaseMieScattering = Atmosphere->MieScatteringScale;
			BaseMieAbsorption = Atmosphere->MieAbsorptionScale;
		}
		if (UExponentialHeightFogComponent* Haze = It->FindComponentByClass<UExponentialHeightFogComponent>(); Haze && !Fog.IsValid())
		{
			Fog = Haze;
			BaseFogDensity = Haze->FogDensity;
			BaseFogStart = Haze->StartDistance;
			BaseFogColor = Haze->FogInscatteringLuminance;
		}
		if (USkyLightComponent* Ambient = It->FindComponentByClass<USkyLightComponent>(); Ambient && !SkyLight.IsValid())
		{
			SkyLight = Ambient;
			BaseSkyLightIntensity = Ambient->Intensity;
		}
	}
	UE_LOG(LogTemp, Display, TEXT("Sky: sun %d, atmosphere %d, haze %d, sky light %d"), Sun.IsValid(), Sky.IsValid(), Fog.IsValid(), SkyLight.IsValid());
}

void UTDSkySubsystem::SolarLongitudeAndDeclination(int32 Year, int32 Month, int32 Day, double& OutLsDeg, double& OutDeclinationDeg)
{
	// Julian date at 12:00 UTC (Meeus), then terrestrial time (TT - UTC = 69.184 s since 2017), Mars24 steps A and B.
	int32 Y = Year, M = Month;
	if (M <= 2)
	{
		Y -= 1;
		M += 12;
	}
	const int32 A = Y / 100;
	const int32 B = 2 - A + A / 4;
	const double JdUt = FMath::FloorToDouble(365.25 * (Y + 4716)) + FMath::FloorToDouble(30.6001 * (M + 1)) + Day + B - 1524.5 + 0.5;
	const double Dt = JdUt + 69.184 / 86400.0 - 2451545.0;                             // days since J2000 (TT)
	const double MeanAnomaly = 19.3871 + 0.52402073 * Dt;                               // B-1
	const double FictitiousSun = 270.3871 + 0.524038496 * Dt;                           // B-2
	// B-4 without the small planetary perturbation sum (under 0.01 degrees).
	const double EquationOfCenter = (10.691 + 3.0e-7 * Dt) * TDSky::Sin(MeanAnomaly) + 0.623 * TDSky::Sin(2.0 * MeanAnomaly)
		+ 0.050 * TDSky::Sin(3.0 * MeanAnomaly) + 0.005 * TDSky::Sin(4.0 * MeanAnomaly) + 0.0005 * TDSky::Sin(5.0 * MeanAnomaly);
	OutLsDeg = FMath::Fmod(FictitiousSun + EquationOfCenter, 360.0);                    // B-5
	if (OutLsDeg < 0.0)
	{
		OutLsDeg += 360.0;
	}
	// D-1: planetographic declination of the sun.
	OutDeclinationDeg = FMath::RadiansToDegrees(FMath::Asin(0.42565 * TDSky::Sin(OutLsDeg))) + 0.25 * TDSky::Sin(OutLsDeg);
}

void UTDSkySubsystem::SunPosition(double LatitudeDeg, double DeclinationDeg, double LocalSolarHours, double& OutElevationDeg, double& OutAzimuthDeg)
{
	const double HourAngle = 15.0 * (LocalSolarHours - 12.0);
	const double SinE = TDSky::Sin(LatitudeDeg) * TDSky::Sin(DeclinationDeg) + TDSky::Cos(LatitudeDeg) * TDSky::Cos(DeclinationDeg) * TDSky::Cos(HourAngle);
	OutElevationDeg = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(SinE, -1.0, 1.0)));
	// Azimuth from south toward west, turned to "from north, clockwise".
	const double FromSouth = FMath::RadiansToDegrees(FMath::Atan2(TDSky::Sin(HourAngle),
		TDSky::Cos(HourAngle) * TDSky::Sin(LatitudeDeg) - FMath::Tan(FMath::DegreesToRadians(DeclinationDeg)) * TDSky::Cos(LatitudeDeg)));
	OutAzimuthDeg = FMath::Fmod(FromSouth + 180.0 + 360.0, 360.0);
}

void UTDSkySubsystem::UpdateSeason()
{
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	const int32 Phase = Mission && Mission->HasMission() ? Mission->GetState().PhaseIndex : -1;
	if (Phase == SeasonPhase)
	{
		return;
	}
	SeasonPhase = Phase;
	// The Earth date of the phase's first sol (MEDA archive); the first archived sol outside a mission.
	FString Date = TEXT("2021-12-28");
	if (Mission)
	{
		int32 SolStart = 1, SolEnd = 1;
		if (Phase >= 0)
		{
			Mission->GetPhaseSols(Phase, SolStart, SolEnd);
		}
		if (const FTDWeatherRow* Weather = Mission->GetWeather(FMath::Max(1, SolStart)); Weather && !Weather->EarthDate.IsEmpty())
		{
			Date = Weather->EarthDate;
		}
	}
	TArray<FString> Parts;
	Date.ParseIntoArray(Parts, TEXT("-"));
	double Ls = 0.0;
	if (Parts.Num() == 3)
	{
		SolarLongitudeAndDeclination(FCString::Atoi(*Parts[0]), FCString::Atoi(*Parts[1]), FCString::Atoi(*Parts[2]), Ls, DeclinationDeg);
	}
	SolarLongitudeDeg = static_cast<float>(Ls);
	UE_LOG(LogTemp, Display, TEXT("Sky: %s -> Ls %.1f, sun declination %.2f"), *Date, Ls, DeclinationDeg);
}

void UTDSkySubsystem::Tick(float DeltaTime)
{
	if (!Sun.IsValid())
	{
		return;
	}
	UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	UpdateSeason();

	// Local solar time: each surface phase starts in the morning; the sun moves only while the surface is played.
	const double Now = GetWorld()->GetTimeSeconds();
	const bool bRunning = Mission && Mission->IsMissionRunning();
	const ATDPlayerController* Controller = Cast<ATDPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	const bool bOnSurface = bRunning && !(Controller && Controller->IsDesignScreenOpen());
	const int32 Phase = bRunning ? Mission->GetState().PhaseIndex : -1;
	const float StartHour = Mission ? static_cast<float>(Mission->GetConstant(TEXT("DayStartHour"))) : 7.5f;
	const float EndHour = Mission ? static_cast<float>(Mission->GetConstant(TEXT("DayEndHour"))) : 17.5f;
	const float RealSeconds = Mission ? FMath::Max(1.f, static_cast<float>(Mission->GetConstant(TEXT("DayRealSeconds")))) : 600.f;
	if (Phase != DayPhase)
	{
		DayPhase = Phase;
		DayStartSeconds = Now;
	}
	if (!bOnSurface)
	{
		DayStartSeconds += DeltaTime;   // the clock waits while the design room is open
	}
	if (FixedHours >= 0.f)
	{
		LocalHours = FixedHours;
	}
	else if (!bRunning || EndHour <= StartHour)
	{
		LocalHours = TDSky::IdleHours;
	}
	else
	{
		const float Alpha = FMath::Clamp(static_cast<float>(Now - DayStartSeconds) / RealSeconds, 0.f, 1.f);
		LocalHours = FMath::Lerp(StartHour, EndHour, Alpha);
	}

	// Dust: a dust-storm phase fills the sky; it clears again over a few seconds.
	const float DustTarget = FixedDust >= 0.f ? FixedDust : (bRunning && Mission->GetState().bDustPhase ? 1.f : 0.f);
	Dust = FMath::FInterpConstantTo(Dust, DustTarget, DeltaTime, TDSky::DustEasePerSecond);
	Apply();
}

void UTDSkySubsystem::Apply()
{
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	const double Latitude = Mission ? Mission->GetConstant(TEXT("BaseLatitudeDeg")) : 0.0;
	double Elevation = 0.0, Azimuth = 0.0;
	SunPosition(Latitude, DeclinationDeg, LocalHours, Elevation, Azimuth);
	SunElevationDeg = static_cast<float>(Elevation);
	SunAzimuthDeg = static_cast<float>(Azimuth);

	// Level axes (from the DEM): +X is east, +Y is south. The light points from the sun to the ground.
	const double E = FMath::Max(Elevation, static_cast<double>(TDSky::MinElevationDeg));
	const FVector ToSun(TDSky::Sin(Azimuth) * TDSky::Cos(E), -TDSky::Cos(Azimuth) * TDSky::Cos(E), TDSky::Sin(E));
	if (AActor* Owner = Sun->GetOwner())
	{
		Owner->SetActorRotation((-ToSun).Rotation());
	}

	Sun->SetIntensity(BaseSunIntensity * FMath::Lerp(1.f, TDSky::StormSunIntensity, Dust));
	Sun->SetLightSourceAngle(FMath::Lerp(BaseSunAngle, TDSky::StormSunAngleDeg, Dust));
	if (Sky.IsValid())
	{
		Sky->SetMieScatteringScale(BaseMieScattering * FMath::Lerp(1.f, TDSky::StormMieScattering, Dust));
		Sky->SetMieAbsorptionScale(BaseMieAbsorption * FMath::Lerp(1.f, TDSky::StormMieAbsorption, Dust));
	}
	if (Fog.IsValid())
	{
		Fog->SetFogDensity(BaseFogDensity * FMath::Lerp(1.f, TDSky::StormFog, Dust));
		Fog->SetStartDistance(FMath::Lerp(BaseFogStart, TDSky::StormFogStartCm, Dust));
		Fog->SetFogInscatteringColor(FMath::Lerp(BaseFogColor, TDSky::StormHaze, Dust));
	}
	if (SkyLight.IsValid())
	{
		SkyLight->SetIntensity(BaseSkyLightIntensity * FMath::Lerp(1.f, TDSky::StormSkyLight, Dust));
	}
	if (SkyParams)
	{
		const float Low = 1.f - FMath::SmoothStep(TDSky::GlowFullBelowDeg, TDSky::GlowStartBelowDeg, static_cast<float>(E));
		const float Glow = FMath::Lerp(TDSky::GlowDay, TDSky::GlowSunset, Low) * (1.f - 0.85f * Dust);
		UKismetMaterialLibrary::SetScalarParameterValue(this, SkyParams, TEXT("BlueGlow"), Glow);
		UKismetMaterialLibrary::SetVectorParameterValue(this, SkyParams, TEXT("SunDir"), FLinearColor(ToSun.X, ToSun.Y, ToSun.Z, 0.f));
	}
}
