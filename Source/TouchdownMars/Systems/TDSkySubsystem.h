// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDSkySubsystem.generated.h"

class UDirectionalLightComponent;
class USkyAtmosphereComponent;
class UExponentialHeightFogComponent;
class USkyLightComponent;

/**
 * Mars sky over the outpost. The sun stands where it really is over the base: the solar longitude (season) comes
 * from the Earth date of the phase's first sol (mars_weather.csv), the sun's declination and position from the
 * NASA GISS Mars24 algorithm (Allison and McEwen 2000), at the base latitude (constants.csv). Local solar time
 * runs from DayStartHour to DayEndHour while a surface phase is played (designer values: time is compressed).
 * A dust-storm phase thickens the dust: more haze, a dimmer sun and softer shadows (look values, no NASA number).
 * The clear-sky look itself lives in the level (Scripts/setup_environment.py); this only moves it.
 */
UCLASS()
class TOUCHDOWNMARS_API UTDSkySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/** Local true solar time at the base, hours 0-24. */
	float GetLocalSolarHours() const { return LocalHours; }
	float GetSunElevationDeg() const { return SunElevationDeg; }
	float GetSunAzimuthDeg() const { return SunAzimuthDeg; }
	/** Solar longitude Ls (Mars season), degrees. */
	float GetSolarLongitudeDeg() const { return SolarLongitudeDeg; }
	/** 0 = clear sky, 1 = dust-storm sky (eased). */
	float GetDustAmount() const { return Dust; }

	/** Mars24 (Allison and McEwen 2000) for an Earth date at 12:00 UTC: solar longitude Ls and declination, degrees. */
	static void SolarLongitudeAndDeclination(int32 Year, int32 Month, int32 Day, double& OutLsDeg, double& OutDeclinationDeg);
	/** Sun elevation and azimuth (from north, clockwise), degrees, for a latitude, declination and local solar time. */
	static void SunPosition(double LatitudeDeg, double DeclinationDeg, double LocalSolarHours, double& OutElevationDeg, double& OutAzimuthDeg);

private:
	void FindSkyActors();
	void UpdateSeason();
	void Apply();

	TWeakObjectPtr<UDirectionalLightComponent> Sun;
	TWeakObjectPtr<USkyAtmosphereComponent> Sky;
	TWeakObjectPtr<UExponentialHeightFogComponent> Fog;
	TWeakObjectPtr<USkyLightComponent> SkyLight;

	// Clear-sky values read from the level at start.
	float BaseSunIntensity = 0.f;
	float BaseSunAngle = 0.35f;
	float BaseMieScattering = 0.f;
	float BaseMieAbsorption = 0.f;
	float BaseFogDensity = 0.f;
	float BaseFogStart = 0.f;
	FLinearColor BaseFogColor = FLinearColor::White;
	float BaseSkyLightIntensity = 1.f;

	int32 SeasonPhase = -2;
	int32 DayPhase = -2;
	double DayStartSeconds = 0.0;
	double DeclinationDeg = 0.0;
	float SolarLongitudeDeg = 0.f;
	float LocalHours = 15.5f;
	float SunElevationDeg = 0.f;
	float SunAzimuthDeg = 0.f;
	float Dust = 0.f;
	float FixedHours = -1.f;   // -TDTime=HH.H (dev)
	float FixedDust = -1.f;    // -TDDust=0..1 (dev)
};
