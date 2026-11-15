// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Systems/TDRoverSubsystem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "LandscapeProxy.h"
#include "Actors/TDSceneActors.h"
#include "Systems/TDDustFXSubsystem.h"
#include "Systems/TDMissionSubsystem.h"
#include "Systems/TDSoundscapeSubsystem.h"
#include "Components/AudioComponent.h"

namespace TDRover
{
	// Look values: a suited-walk pace for the rover on base roads, a crawl for the excavator.
	constexpr float RoverSpeed = 160.f;
	constexpr float DiggerSpeed = 45.f;
	constexpr float RoverWait = 18.f;
	constexpr float DigSeconds = 14.f;
	constexpr float DumpSeconds = 7.f;
	constexpr float TurnDegPerSec = 25.f;
}

bool UTDRoverSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

TStatId UTDRoverSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDRoverSubsystem, STATGROUP_Tickables);
}

float UTDRoverSubsystem::GroundZ(const FVector& At) const
{
	bool bFound = false;
	float Best = At.Z;
	for (const TWeakObjectPtr<ALandscapeProxy>& Landscape : Landscapes)
	{
		if (const ALandscapeProxy* Proxy = Landscape.Get())
		{
			const TOptional<float> Z = Proxy->GetHeightAtLocation(At);
			if (Z.IsSet() && (!bFound || Z.GetValue() > Best))
			{
				Best = Z.GetValue();
				bFound = true;
			}
		}
	}
	return Best;
}

void UTDRoverSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (!InWorld.GetMapName().Contains(TEXT("Outpost")))
	{
		return;
	}
	for (TActorIterator<ALandscapeProxy> It(&InWorld); It; ++It)
	{
		Landscapes.Add(*It);
	}
	UTDMissionSubsystem* Mission = InWorld.GetSubsystem<UTDMissionSubsystem>();
	for (TActorIterator<AActor> It(&InWorld); It; ++It)
	{
		UStaticMeshComponent* Root = Cast<UStaticMeshComponent>(It->GetRootComponent());
		if (!Root || !Root->GetStaticMesh() || It->IsHidden())
		{
			continue;
		}
		const FString Path = Root->GetStaticMesh()->GetPathName();
		const bool bDigger = Path.Contains(TEXT("RASSOR"));
		const bool bRover = Path.Contains(TEXT("Space_Exploration_Vehicle"));
		if (!bDigger && !bRover)
		{
			continue;
		}
		Root->SetMobility(EComponentMobility::Movable);
		FMachine M;
		M.Actor = *It;
		M.bExcavator = bDigger;
		M.Speed = bDigger ? TDRover::DiggerSpeed : TDRover::RoverSpeed;
		// Drive along the model's long axis.
		const FBox Box = Root->GetStaticMesh()->GetBoundingBox();
		M.YawOffset = Box.GetSize().Y > Box.GetSize().X ? 90.f : 0.f;
		const FVector Start = It->GetActorLocation();
		M.Height = Start.Z - GroundZ(Start);
		const FVector Forward = FRotator(0.f, It->GetActorRotation().Yaw + M.YawOffset, 0.f).Vector();
		if (bDigger)
		{
			// Dig spot ahead, berm at the start: out, dig, back up, dump.
			M.Route = { Start + Forward * 900.f, Start };
		}
		else
		{
			// A loop of reachable waypoints around the start (the crew walk grid keeps the rover off the modules).
			M.Route.Add(Start);
			for (int32 Step = 1; Step <= 4; ++Step)
			{
				const FVector Candidate = Start + FRotator(0.f, Step * 90.f - 45.f, 0.f).Vector() * 3200.f;
				// Only waypoints out on the plain, away from the modules around the base centre.
				if (Candidate.Size2D() > 3200.f && Mission && Mission->FindCrewPath(Start, Candidate).Num() > 0)
				{
					M.Route.Add(Candidate);
				}
			}
		}
		M.Wait = bDigger ? 4.f : 10.f;
		M.RouteIndex = 0;
		Machines.Add(M);
		UE_LOG(LogTemp, Display, TEXT("Rovers: %s %s at %s, %d waypoints"), bDigger ? TEXT("excavator") : TEXT("rover"), *It->GetName(), *Start.ToCompactString(), M.Route.Num());
	}
	bReady = Machines.Num() > 0;
}

void UTDRoverSubsystem::StartLeg(FMachine& M)
{
	AActor* Actor = M.Actor.Get();
	if (!Actor || M.Route.Num() < 2)
	{
		return;
	}
	M.RouteIndex = (M.RouteIndex + 1) % M.Route.Num();
	const FVector Goal = M.Route[M.RouteIndex];
	M.Leg.Reset();
	if (UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>(); Mission && !M.bExcavator)
	{
		M.Leg = Mission->FindCrewPath(Actor->GetActorLocation(), Goal);
	}
	if (M.Leg.Num() == 0)
	{
		M.Leg.Add(Goal);
	}
	M.LegIndex = 0;
	M.bReverse = M.bExcavator && M.RouteIndex == 1; // the excavator backs up to the berm
}

bool UTDRoverSubsystem::PersonAhead(const FMachine& M, const FVector& Dir, float Range) const
{
	const AActor* Actor = M.Actor.Get();
	auto Blocks = [Actor, &Dir, Range](const FVector& Where)
	{
		FVector To = Where - Actor->GetActorLocation();
		To.Z = 0.f;
		const float Ahead = FVector::DotProduct(To, Dir);
		const float Side = FVector::CrossProduct(Dir, To).Size();
		return Ahead > 0.f && Ahead < Range && Side < 350.f;
	};
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0); Player && Blocks(Player->GetActorLocation()))
	{
		return true;
	}
	for (TActorIterator<ATDCrewMember> It(GetWorld()); It; ++It)
	{
		if (!It->IsInside() && Blocks(It->GetActorLocation()))
		{
			return true;
		}
	}
	return false;
}

void UTDRoverSubsystem::Place(FMachine& M, const FVector& At, float YawDeg, float DeltaTime)
{
	AActor* Actor = M.Actor.Get();
	// Sit on the terrain: height from the ground under the centre, pitch and roll from the ground a few metres
	// ahead/behind and to the sides, eased so the body does not snap over bumps.
	const FRotator Heading(0.f, YawDeg + M.YawOffset, 0.f);
	const FVector F = Heading.Vector();
	const FVector R = FVector::CrossProduct(FVector::UpVector, F);
	const float Front = GroundZ(At + F * 180.f);
	const float Back = GroundZ(At - F * 180.f);
	const float Left = GroundZ(At - R * 120.f);
	const float Right = GroundZ(At + R * 120.f);
	const float Pitch = FMath::RadiansToDegrees(FMath::Atan2(Front - Back, 360.f));
	const float Roll = FMath::RadiansToDegrees(FMath::Atan2(Left - Right, 240.f));
	FRotator Target(0.f, YawDeg, 0.f);
	// Pitch and roll in the actor frame (the mesh forward may be the actor Y axis).
	if (M.YawOffset == 0.f)
	{
		Target.Pitch = Pitch;
		Target.Roll = -Roll;
	}
	else
	{
		Target.Roll = -Pitch;
		Target.Pitch = -Roll;
	}
	// Yaw is already rate limited by the caller; only the tilt is eased.
	FRotator Smoothed = FMath::RInterpTo(Actor->GetActorRotation(), Target, DeltaTime, 4.f);
	Smoothed.Yaw = YawDeg;
	const FVector Location(At.X, At.Y, FMath::FInterpTo(Actor->GetActorLocation().Z, GroundZ(At) + M.Height, DeltaTime, 6.f));
	Actor->SetActorLocationAndRotation(Location, Smoothed);
}

void UTDRoverSubsystem::Tick(float DeltaTime)
{
	if (!bReady)
	{
		return;
	}
	DeltaTime = FMath::Min(DeltaTime, 0.1f);
	for (FMachine& M : Machines)
	{
		AActor* Actor = M.Actor.Get();
		if (!Actor || M.Route.Num() < 2)
		{
			continue;
		}
		const FVector Here = Actor->GetActorLocation();
		const float Yaw = Actor->GetActorRotation().Yaw;
		const FVector Forward = FRotator(0.f, Yaw + M.YawOffset, 0.f).Vector();
		if (!M.Sound.IsValid())
		{
			M.Sound = UTDSoundscapeSubsystem::AttachLoop(Actor, M.bExcavator ? TEXT("SFX_TD_World_DiggerDrum") : TEXT("SFX_TD_World_RoverMotor"), 0.f);
		}
		auto Voice = [&M, DeltaTime, this](float Target)
		{
			M.Activity = FMath::FInterpTo(M.Activity, Target, DeltaTime, 1.5f);
			if (UAudioComponent* Sound = M.Sound.Get())
			{
				const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
				const bool bOn = Mission && Mission->IsSoundOn();
				Sound->SetVolumeMultiplier(bOn ? 0.15f + 0.85f * M.Activity : 0.f);
				Sound->SetPitchMultiplier(0.85f + 0.2f * M.Activity);
			}
		};
		if (M.Wait > 0.f)
		{
			// Rover parked: systems idle. Excavator stopped at the dig or the berm: drums turning.
			Voice(M.bExcavator ? 1.f : 0.f);
			M.Wait -= DeltaTime;
			// Digging and dumping kick up dust at the drums.
			M.PuffClock -= DeltaTime;
			if (M.bExcavator && M.PuffClock <= 0.f)
			{
				M.PuffClock = 0.5f;
				const FVector Drum = Here + Forward * (M.RouteIndex == 0 ? 120.f : -120.f) * (FMath::RandBool() ? 1.f : -1.f);
				UTDDustFXSubsystem::Puff(this, FVector(Drum.X, Drum.Y, GroundZ(Drum)), M.RouteIndex == 1 ? 2.2f : 1.5f);
			}
			if (M.Wait <= 0.f)
			{
				StartLeg(M);
			}
			Place(M, Here, Yaw, DeltaTime);
			continue;
		}
		if (M.Leg.Num() == 0)
		{
			StartLeg(M);
			continue;
		}
		const FVector Goal = M.Leg[M.LegIndex];
		FVector To(Goal.X - Here.X, Goal.Y - Here.Y, 0.f);
		const float Distance = To.Size();
		if (Distance < 40.f)
		{
			if (++M.LegIndex >= M.Leg.Num())
			{
				M.Leg.Reset();
				M.Wait = M.bExcavator ? (M.RouteIndex == 0 ? TDRover::DigSeconds : TDRover::DumpSeconds) : TDRover::RoverWait;
				M.PuffClock = 0.f;
			}
			continue;
		}
		To /= Distance;
		// The excavator backs up without turning; the rover turns toward the path first.
		const FVector Facing = M.bReverse ? -To : To;
		const float WantYaw = Facing.Rotation().Yaw - M.YawOffset;
		const float NewYaw = FMath::FixedTurn(Yaw, WantYaw, TDRover::TurnDegPerSec * DeltaTime);
		const bool bAligned = FMath::Abs(FRotator::NormalizeAxis(WantYaw - NewYaw)) < 25.f;
		float Step = 0.f;
		const bool bBlocked = PersonAhead(M, To, M.bExcavator ? 300.f : 600.f);
		if (bAligned && !bBlocked)
		{
			Step = FMath::Min(Distance, M.Speed * FMath::Clamp(Distance / 200.f, 0.3f, 1.f) * DeltaTime);
		}
		Voice(Step > 0.f || FMath::Abs(FRotator::NormalizeAxis(NewYaw - Yaw)) > 0.01f ? (M.bExcavator ? 0.5f : 1.f) : 0.1f);
		Place(M, Here + To * Step, NewYaw, DeltaTime);
	}
}
