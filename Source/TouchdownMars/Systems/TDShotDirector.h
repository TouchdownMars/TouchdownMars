// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
// Camera system: one fixed main camera (CAM_Main) and short, skippable Outcome Shots at loop step 4.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Model/TDTypes.h"
#include "TDShotDirector.generated.h"

class AActor;
class APlayerController;
class UDataTable;

/** One Outcome Shot (DT_OutcomeShots, source Data/Source/outcome_shots.csv). Row name = any unique key. */
USTRUCT(BlueprintType)
struct FOutcomeShot : public FTableRowBase
{
	GENERATED_BODY()

	/** Shot identity; rows of several events may share one ShotId (replays use the short hold). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	FName ShotId;

	/** Event card id (events.csv Name) or "Departure" for the mission end. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	FName EventId;

	/** Actor Tag of the CineCameraActor placed in the level for this shot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	FName CameraTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	float BlendIn = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	float Hold = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	float BlendOut = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	bool bAllowedInCadet = true;

	/** text.csv key of the one-sentence result (EN + TR). Key_A / Key_B, if present, win for choice A / B. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	FName ResultTextKey;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnShotFinished, FName, EventId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTDOnShotStarted, FName, ShotId, FName, CameraTag, int32, OutcomeState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnShotEnded, FName, ShotId);

/**
 * Lives on ATDGameMode (one per world, nothing to place, cannot be forgotten in a level).
 * Owns the main view (CAM_Main + optional idle drift) and plays Outcome Shots. Uses no random numbers,
 * so the seeded mission stream is untouched. Every PlayShot call fires OnShotFinished exactly once.
 */
UCLASS(ClassGroup = (TouchdownMars), meta = (BlueprintSpawnableComponent))
class TOUCHDOWNMARS_API UTDShotDirector : public UActorComponent
{
	GENERATED_BODY()

public:
	UTDShotDirector();

	static UTDShotDirector* Get(const UObject* WorldContext);

	/** Actor Tag of the fixed main gameplay camera. */
	static const FName MainCameraTag;

	/** Points the player at CAM_Main (falls back to the first ATDCameraRig in test maps). */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Camera")
	void ActivateMainCamera();

	/** Loop step 4: blend to the event's shot, hold, blend back, then OnShotFinished. Never blocks the turn. */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Camera")
	void PlayShot(FName EventId, int32 OutcomeState);

	/** Any left click during a shot: back to CAM_Main in 0.15 s, OnShotFinished now (once). */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Camera")
	void SkipShot();

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Camera")
	bool IsPlayingShot() const { return bPlaying; }

	/** The one-sentence result for this event and choice (empty if the row has none). */
	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Camera")
	FText GetResultText(FName EventId, int32 OutcomeState) const;

	/** The result text key actually used (for redraw on a language switch). */
	FName GetResultTextKey(FName EventId, int32 OutcomeState) const;

	/** Debug / demo recording (not in Shipping): plays a shot by ShotId with full hold, ignoring the gates. */
	void DebugPlayShot(FName ShotId);
	void DebugPlayAllShots();

	UPROPERTY(BlueprintAssignable, Category = "Touchdown Mars|Camera")
	FTDOnShotFinished OnShotFinished;

	/** For scene props (crew staging, dimmed modules, plant racks, message icon). */
	UPROPERTY(BlueprintAssignable, Category = "Touchdown Mars|Camera")
	FTDOnShotStarted OnShotStarted;

	UPROPERTY(BlueprintAssignable, Category = "Touchdown Mars|Camera")
	FTDOnShotEnded OnShotEnded;

	/** No shot in the first seconds of play (the opening minute stays fast). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	float NoShotSeconds = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	float ReplayHold = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	float CadetMaxHold = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	float SkipBlend = 0.15f;

	/** Idle life of CAM_Main: at most this far (cm) over DriftSeconds, ping-pong, no rotation. 0 = off. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	float DriftDistance = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	float DriftSeconds = 10.f;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	const FOutcomeShot* FindRowForEvent(FName EventId) const;
	const FOutcomeShot* FindRowForShot(FName ShotId) const;
	AActor* FindTaggedActor(FName Tag) const;
	APlayerController* GetPC() const;
	bool LoadTable() const;
	bool IsReduceMotion() const;
	void FinishNextTick(FName EventId);
	void StartShot(const FOutcomeShot& Row, AActor* Camera, FName EventId, int32 OutcomeState, float HoldSeconds);
	void BeginBlendOut();
	void Finish();
	void ApplyDrift();

	UFUNCTION()
	void HandlePhaseStarted(ETDPhase Phase);

	UFUNCTION()
	void HandleDebugChainStep(FName EventId);

	UPROPERTY(Transient)
	mutable TObjectPtr<UDataTable> ShotTable;

	UPROPERTY(Transient)
	TObjectPtr<AActor> MainCamera;

	FVector MainCameraBase = FVector::ZeroVector;
	float DriftTime = 0.f;

	bool bPlaying = false;
	bool bFinishFired = true;
	float ShotElapsed = 0.f;
	float ShotBlendOut = 0.f;
	FName ActiveEventId;
	FName ActiveShotId;
	FTimerHandle BlendOutTimer;
	FTimerHandle FinishTimer;
	FTimerHandle SafetyTimer;

	/** ShotIds seen in this playthrough (first time = full hold). */
	TSet<FName> PlayedShots;

	/** Debug "play all" queue of ShotIds. */
	TArray<FName> DebugQueue;
	bool bDebugChain = false;
};
