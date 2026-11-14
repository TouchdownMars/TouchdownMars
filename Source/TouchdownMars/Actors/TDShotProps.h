// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
// Placeholder props that only matter inside Outcome Shots (engine basic shapes; no new 3D models).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDShotProps.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

/**
 * A basic-shape prop shown only during the listed shots (cable at the power unit, message icon over the dish).
 * Optional travel: moves by TravelOffset across the shot, pausing between PauseStart and PauseEnd (0..1 of the path time).
 */
UCLASS()
class TOUCHDOWNMARS_API ATDShotProp : public AActor
{
	GENERATED_BODY()

public:
	ATDShotProp();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	TArray<FName> ShotIds;

	/** True: hidden outside its shots. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	bool bOnlyDuringShot = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	FVector TravelOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	float TravelSeconds = 2.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	float PauseStart = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	float PauseEnd = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	FLinearColor Color = FLinearColor(0.95f, 0.75f, 0.25f, 1.f);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	UFUNCTION()
	void HandleShotStarted(FName ShotId, FName CameraTag, int32 OutcomeState);

	UFUNCTION()
	void HandleShotEnded(FName ShotId);

	FVector HomeLocation;
	float Elapsed = 0.f;
	bool bActive = false;
};

/** Placeholder plant racks for S04_GREEN: healthy green when GREENHOUSE gets its demand, stressed (yellow, drooping) when not. Never grows. */
UCLASS()
class TOUCHDOWNMARS_API ATDPlantRack : public AActor
{
	GENERATED_BODY()

public:
	ATDPlantRack();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	TArray<FName> ShotIds = { TEXT("S04_GREEN") };

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Camera")
	void ShowHealth(bool bHealthy);

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	TArray<TObjectPtr<UStaticMeshComponent>> Shelves;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	TArray<TObjectPtr<UStaticMeshComponent>> Plants;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	TObjectPtr<UPointLightComponent> GrowLight;

private:
	UFUNCTION()
	void HandleShotStarted(FName ShotId, FName CameraTag, int32 OutcomeState);
};
