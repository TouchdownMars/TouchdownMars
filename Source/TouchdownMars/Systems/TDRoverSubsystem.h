// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDRoverSubsystem.generated.h"

/**
 * The outpost's machines at work, so the base never looks frozen:
 * - the RASSOR regolith excavator (NASA KSC Swamp Works) drives out, digs with its drums, backs up to the berm and
 *   dumps, again and again (it drives both ways without turning, like the real robot);
 * - the pressurized rover (NASA SEV) drives a slow loop between waypoints around the base and waits at each.
 * Both follow the terrain (height, pitch and roll), take the crew walk grid around obstacles, and stop when a person
 * stands in front of them. The models are found by their NASA mesh, so no level change is needed. Speeds and pauses
 * are look values (no NASA number is shown).
 */
UCLASS()
class TOUCHDOWNMARS_API UTDRoverSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	struct FMachine
	{
		TWeakObjectPtr<AActor> Actor;
		bool bExcavator = false;
		TArray<FVector> Route;      // waypoints (excavator: dig spot, berm)
		int32 RouteIndex = 0;
		TArray<FVector> Leg;        // walk-grid points of the current leg
		int32 LegIndex = 0;
		float Speed = 100.f;        // cm/s
		float Wait = 0.f;           // seconds left at a stop
		float PuffClock = 0.f;
		float YawOffset = 0.f;      // mesh forward relative to actor yaw
		float Height = 0.f;         // pivot height above the ground at start
		bool bReverse = false;      // excavator backing to the berm
	};
	TArray<FMachine> Machines;
	TArray<TWeakObjectPtr<class ALandscapeProxy>> Landscapes;
	bool bReady = false;

	float GroundZ(const FVector& At) const;
	void StartLeg(FMachine& M);
	void Place(FMachine& M, const FVector& At, float YawDeg, float DeltaTime);
	bool PersonAhead(const FMachine& M, const FVector& Dir, float Range) const;
};
