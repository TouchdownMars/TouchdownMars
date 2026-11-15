// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDDustFXSubsystem.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class ULocalFogVolumeComponent;

/**
 * Wind-blown dust around the outpost (Scripts/make_dust_vfx.py builds the Niagara systems).
 * - Drift: thin fine dust carried past the player by the daytime wind from the east-southeast.
 * - Storm: dense blowing dust and sand streaks while a dust-storm phase runs (follows the sky's dust amount).
 * - Ground haze: a local fog volume that hugs the ground in a storm (volumetric, lit by the dimmed sun).
 * - Dust devils: a few whirlwinds far out on the plain in the convective hours, drifting west.
 * Real values (wind direction, devil hours and speed) are in research_notes/Mars VFX/vfx_research.md;
 * particle counts and colours are look values.
 */
UCLASS()
class TOUCHDOWNMARS_API UTDDustFXSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/** A small dust puff on the ground (boot step, hammer, scoop, drill); skipped far from the camera. */
	static void Puff(const UObject* WorldContext, const FVector& At, float Scale = 1.f);

private:
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Drift;
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Storm;
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> StormFar;
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> SandSkim;
	UPROPERTY(Transient) TArray<TObjectPtr<UNiagaraComponent>> Devils;
	UPROPERTY(Transient) TObjectPtr<ULocalFogVolumeComponent> GroundHaze;
	UPROPERTY(Transient) TObjectPtr<AActor> HazeActor;
	TArray<FVector> DevilStart;
	float Clock = 0.f;
	bool bReady = false;

	UNiagaraComponent* Spawn(const TCHAR* Path, const FVector& At);
	static void SetActive(UNiagaraComponent* Component, bool bActive);
};
