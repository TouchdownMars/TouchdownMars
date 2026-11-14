// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Model/TDTypes.h"
#include "TDSystemModule.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

/** Base scene actor for AIR, GREENHOUSE and SHELTER: its light brightens with the system level. */
UCLASS()
class TOUCHDOWNMARS_API ATDSystemModule : public AActor
{
	GENERATED_BODY()

public:
	ATDSystemModule();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	ETDSystem System = ETDSystem::Air;

	/** Light intensity in lumens by level 0..4. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	TArray<float> IntensityByLevel = { 0.f, 300.f, 1200.f, 3000.f, 4500.f };

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars")
	void ApplyLevel(int32 Level);

	/** Outcome Shots in which this module dims when it got no cell this phase (S03_STORM). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	TArray<FName> DimWhenUnpoweredShots = { TEXT("S03_STORM") };

	/** Light multiplier while dimmed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	float ShotDimFactor = 0.12f;

	/** Blueprint children add fans, plants, doors here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Touchdown Mars")
	void OnLevelChanged(int32 Level);

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	TObjectPtr<UPointLightComponent> Light;

private:
	UFUNCTION()
	void HandleLevelsChanged(const FTDMissionState& State, const TArray<ETDSystem>& Changed);

	UFUNCTION()
	void HandleAllocation(const FTDMissionState& State);

	void UpdateLight();

	UFUNCTION()
	void HandleShotStarted(FName ShotId, FName CameraTag, int32 OutcomeState);

	UFUNCTION()
	void HandleShotEnded(FName ShotId);

	bool bShotDimmed = false;

	int32 CurrentLevel = INDEX_NONE;
	bool bLit = false;
};
