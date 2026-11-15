// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Systems/TDDustFXSubsystem.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Systems/TDMissionSubsystem.h"
#include "Systems/TDSkySubsystem.h"

namespace TDDust
{
	// Level axes: +X east, +Y south. Daytime wind blows from the east-southeast (Newman et al. 2022, Fig. 1B), so the
	// dust moves toward the west-northwest: yaw 202.5 degrees.
	constexpr float WindYawDeg = 202.5f;
	// Dust devils: convective hours 10:30 to 16:00 local true solar time (Newman et al. 2022), drifting about
	// 5.3 m/s (JPL, PIA26074, about 19 km/h). Distances and count are look values.
	constexpr float DevilFromHour = 10.5f;
	constexpr float DevilToHour = 16.f;
	constexpr float DevilSpeedCmPerSec = 530.f;
	constexpr float DevilWrapCm = 900000.f;
	// Storm haze near the ground (look values).
	constexpr float HazeRadiusCm = 600000.f;
	constexpr float HazeExtinction = 0.0035f;
}

bool UTDDustFXSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

TStatId UTDDustFXSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDDustFXSubsystem, STATGROUP_Tickables);
}

UNiagaraComponent* UTDDustFXSubsystem::Spawn(const TCHAR* Path, const FVector& At)
{
	UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, Path);
	if (!System)
	{
		UE_LOG(LogTemp, Warning, TEXT("DustFX: %s missing (run Scripts/make_dust_vfx.py)"), Path);
		return nullptr;
	}
	return UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), System, At, FRotator(0.f, TDDust::WindYawDeg, 0.f),
		FVector(1.f), false, false, ENCPoolMethod::None, false);
}

void UTDDustFXSubsystem::SetActive(UNiagaraComponent* Component, bool bActive)
{
	if (!Component || Component->IsActive() == bActive)
	{
		return;
	}
	if (bActive)
	{
		Component->Activate(true);
	}
	else
	{
		Component->Deactivate(); // live particles finish their life: no pop
	}
}

void UTDDustFXSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (!InWorld.GetMapName().Contains(TEXT("Outpost")))
	{
		return;
	}
	Drift = Spawn(TEXT("/Game/TouchdownMars/VFX/NS_TD_DustDrift.NS_TD_DustDrift"), FVector::ZeroVector);
	Storm = Spawn(TEXT("/Game/TouchdownMars/VFX/NS_TD_DustStorm.NS_TD_DustStorm"), FVector::ZeroVector);
	StormFar = Spawn(TEXT("/Game/TouchdownMars/VFX/NS_TD_DustStormFar.NS_TD_DustStormFar"), FVector::ZeroVector);
	SandSkim = Spawn(TEXT("/Game/TouchdownMars/VFX/NS_TD_SandSkim.NS_TD_SandSkim"), FVector::ZeroVector);
	// Three devils far out on the crater floor (east, south-east and north-east of the base).
	DevilStart = { FVector(420000.f, 90000.f, 0.f), FVector(310000.f, 260000.f, 0.f), FVector(560000.f, -210000.f, 0.f) };
	for (const FVector& Start : DevilStart)
	{
		if (UNiagaraComponent* Devil = Spawn(TEXT("/Game/TouchdownMars/VFX/NS_TD_DustDevil.NS_TD_DustDevil"), Start))
		{
			Devil->SetWorldRotation(FRotator::ZeroRotator);
			Devils.Add(Devil);
		}
	}
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	HazeActor = InWorld.SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	if (HazeActor)
	{
		GroundHaze = NewObject<ULocalFogVolumeComponent>(HazeActor);
		HazeActor->SetRootComponent(GroundHaze);
		GroundHaze->RegisterComponent();
		GroundHaze->SetWorldScale3D(FVector(TDDust::HazeRadiusCm / ULocalFogVolumeComponent::GetBaseVolumeSize()));
		GroundHaze->SetRadialFogExtinction(0.f);
		GroundHaze->SetHeightFogExtinction(0.f);
		GroundHaze->SetHeightFogFalloff(1200.f);
		GroundHaze->SetHeightFogOffset(0.f);
		GroundHaze->SetFogPhaseG(0.7f); // Mars dust scatters strongly forward (g about 0.7, Tomasko et al. 1999)
		GroundHaze->SetFogAlbedo(FLinearColor(0.78f, 0.52f, 0.33f));
		GroundHaze->SetVisibility(false);
	}
	bReady = true;
}

void UTDDustFXSubsystem::Tick(float DeltaTime)
{
	if (!bReady)
	{
		return;
	}
	Clock += DeltaTime;
	const UTDSkySubsystem* Sky = GetWorld()->GetSubsystem<UTDSkySubsystem>();
	const float Dust = Sky ? Sky->GetDustAmount() : 0.f;
	const float Hours = Sky ? Sky->GetLocalSolarHours() : 12.f;
	FVector Eye = FVector::ZeroVector;
	if (const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0); PC && PC->PlayerCameraManager)
	{
		Eye = PC->PlayerCameraManager->GetCameraLocation();
	}
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	const bool bReduce = Mission && Mission->IsReduceMotion();

	// The near emitters follow the camera (world-space particles stay where they were born, so they stream past).
	// Spawn boxes are centred on the component: lift each so its box starts at the feet (eye height about 170 cm).
	const FVector Ground(Eye.X, Eye.Y, Eye.Z - 170.f);
	if (Drift)
	{
		Drift->SetWorldLocation(Ground + FVector(0.f, 0.f, 140.f));
		SetActive(Drift, !bReduce && Dust < 0.6f && Hours > 8.f && Hours < 18.5f);
	}
	if (Storm)
	{
		Storm->SetWorldLocation(Ground + FVector(0.f, 0.f, 420.f));
		SetActive(Storm, !bReduce && Dust > 0.3f);
	}
	if (StormFar)
	{
		StormFar->SetWorldLocation(Ground + FVector(0.f, 0.f, 1150.f));
		SetActive(StormFar, Dust > 0.15f);
	}
	if (SandSkim)
	{
		SandSkim->SetWorldLocation(Ground + FVector(0.f, 0.f, 30.f));
		SetActive(SandSkim, !bReduce && Dust > 0.45f);
	}
	if (GroundHaze)
	{
		GroundHaze->SetWorldLocation(Ground);
		GroundHaze->SetHeightFogExtinction(TDDust::HazeExtinction * Dust * Dust);
		GroundHaze->SetVisibility(Dust > 0.02f);
	}
	// Devils: daytime convection only, never in a storm (the storm hides them anyway).
	const bool bDevils = !bReduce && Hours >= TDDust::DevilFromHour && Hours <= TDDust::DevilToHour && Dust < 0.3f;
	for (int32 Index = 0; Index < Devils.Num(); ++Index)
	{
		if (UNiagaraComponent* Devil = Devils[Index])
		{
			// Drift west with the wind and wrap back to the east edge of the plain.
			const float Travel = FMath::Fmod(Clock * TDDust::DevilSpeedCmPerSec + Index * 170000.f, TDDust::DevilWrapCm);
			FVector At = DevilStart[Index] - FVector(Travel, 0.f, 0.f);
			At.Z = -1500.f; // the far terrain sits a little lower than the base
			Devil->SetWorldLocation(At);
			SetActive(Devil, bDevils);
		}
	}
}
