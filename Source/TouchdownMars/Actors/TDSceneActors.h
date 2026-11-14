// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
// Greybox scene actors for L_TD_Outpost (level-design.md §3). No Tick: delegates and short timers only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Model/TDTypes.h"
#include "TDSceneActors.generated.h"

class UCineCameraComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/** BP_TD_CameraRig parent: fixed three-quarter camera with a ~2 s opening push-in (off with Reduce motion). */
UCLASS()
class TOUCHDOWNMARS_API ATDCameraRig : public AActor
{
	GENERATED_BODY()

public:
	ATDCameraRig();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	float PushInDistance = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	float PushInSeconds = 2.f;

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars")
	void PlayPushIn();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	TObjectPtr<UCineCameraComponent> Camera;

private:
	void StepPushIn();

	FTimerHandle PushTimer;
	float PushElapsed = 0.f;
};

/** BP_TD_PowerUnit parent: the fission unit and one energy line per system; line thickness follows the cells given. */
UCLASS()
class TOUCHDOWNMARS_API ATDPowerUnit : public AActor
{
	GENERATED_BODY()

public:
	ATDPowerUnit();

	/** Line radius scale per cell given (design.md EnergyLine_Width: thicker per cell, thin minimum). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	float ThicknessPerCell = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	float MinThickness = 0.03f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	TArray<TObjectPtr<UStaticMeshComponent>> Lines;

private:
	UFUNCTION()
	void HandleAllocation(const FTDMissionState& State);

	void LayOutLines();

	TArray<FVector> Targets;
};

/** Crew member placeholder (BP_TD_FlightEngineer etc. derive from this): reacts to its system's level. */
UCLASS()
class TOUCHDOWNMARS_API ATDCrewMember : public AActor
{
	GENERATED_BODY()

public:
	ATDCrewMember();

	/** The system this crew member looks after (Engineer AIR, Science GREENHOUSE, Health SHELTER). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	ETDSystem System = ETDSystem::Air;

	/** Where the crew walks when the mission ends (lander). Set in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	FVector LanderLocation = FVector(-4000.f, 0.f, 0.f);

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars")
	void PlayBehaviour(ETDCrewBehaviour Behaviour);

	/** Blueprint children play animations here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Touchdown Mars")
	void OnBehaviour(ETDCrewBehaviour Behaviour);

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars")
	ETDCrewBehaviour GetBehaviour() const { return Current; }

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	TObjectPtr<UStaticMeshComponent> Body;

private:
	UFUNCTION()
	void HandleLevels(const FTDMissionState& State, const TArray<ETDSystem>& Changed);

	UFUNCTION()
	void HandleEventResolved(FName EventId, const FText& Cause);

	UFUNCTION()
	void HandleEnded(ETDMissionResult Result);

	UFUNCTION()
	void HandlePhaseStarted(ETDPhase Phase);

	/** Outcome Shot staging: stand at the TargetPoint tagged Stage_<CameraTag>_<System>[_A|_B], back afterwards. */
	UFUNCTION()
	void HandleShotStarted(FName ShotId, FName CameraTag, int32 OutcomeState);

	UFUNCTION()
	void HandleShotEnded(FName ShotId);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	/** Idle life (timer, no Tick): slow look-around and breathing; worried/alarm sway faster; celebrate hops. */
	void SetBasePose(const FVector& Location, const FRotator& Rotation);
	void StepIdle();

	FVector HomeLocation;
	FVector BaseLocation;
	FRotator BaseRotation;
	FTimerHandle IdleTimer;
	float IdleTime = 0.f;
	float IdleSeed = 0.f;
	bool bStaged = false;
	FTransform PreShotTransform;
	ETDCrewBehaviour Current = ETDCrewBehaviour::Working;
};
