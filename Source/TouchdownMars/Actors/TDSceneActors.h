// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
// Greybox scene actors for L_TD_Outpost. No Tick: delegates and short timers only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Model/TDTypes.h"
#include "TDSceneActors.generated.h"

class UAnimSequence;
class UMaterialInterface;
class UCineCameraComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
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

	/** Line radius scale per cell given (thicker per cell, thin minimum). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	float ThicknessPerCell = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	float MinThickness = 0.03f;

	/** The diorama's floating energy lines. Off in the first-person base, which has real cable trays (setup_base.py). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	bool bShowEnergyLines = false;

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

/** One step of a crew member's phase plan: walk the points, then work (or stay inside) for WorkSeconds. */
struct FTDCrewStep
{
	TArray<FVector> Points;
	FVector Look = FVector::ZeroVector;
	float WorkSeconds = 0.f;
	bool bInside = false;
	FText Status;
	/** The task worked at this step (picks the work pose and tool). */
	FName TaskId;
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

	/** Optional rigged suit (Tripo astronaut). When set, it replaces the static body and plays the animations below. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Suit")
	TObjectPtr<USkeletalMesh> SuitMesh;

	/** Optional material for every suit slot (system-color suit, no printed patches). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Suit")
	TObjectPtr<UMaterialInterface> SuitMaterial;

	/** Height the suit is scaled to (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Suit")
	float SuitHeight = 180.f;

	/** Ground speed of the walk clip at play rate 1 (cm/s): the walk play rate is path speed / this, so the feet do
	 *  not slide. [DESIGNER VALUE, measured from the Tripo walk clip at the 180 cm scale, +-5 %] */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Suit")
	float WalkClipSpeedCmPerSec = 97.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Suit")
	TObjectPtr<UAnimSequence> IdleAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Suit")
	TObjectPtr<UAnimSequence> WorriedAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Suit")
	TObjectPtr<UAnimSequence> WalkAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Suit")
	TObjectPtr<UAnimSequence> WorkAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Suit")
	TObjectPtr<UAnimSequence> CelebrateAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Suit")
	TObjectPtr<UAnimSequence> FarewellAnim;

	/** Where the suit mesh faces, in degrees of actor yaw: the Tripo astronaut faces the actor's +Y (90).
	 *  Walking, working and escorting turn the actor so the suit (not the actor X axis) faces the way it moves. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Suit")
	float SuitForwardYaw = 90.f;

	/** Crew name shown over the head and on the base computer (fictional adult crew). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Crew")
	FString CrewName = TEXT("Crew");

	/** Walks along a navigation path to Target (straight line when there is no nav mesh), then calls OnArrived. */
	void WalkTo(const FVector& Target, float SpeedCmPerSec, TFunction<void()> OnArrived = nullptr);
	/** Walks through the given points (from the base path graph), then calls OnArrived. */
	void WalkPath(const TArray<FVector>& Points, float SpeedCmPerSec, TFunction<void()> OnArrived = nullptr);
	void StopWalking();
	/** Walk clip play rate for a path speed (feet planted). */
	float WalkRate(float SpeedCmPerSec) const;
	bool IsWalking() const { return PathPoints.Num() > 0; }

	/** Plays the work pose for the task on the spot, facing LookAt, with the task's tool in the right hand. */
	void WorkAt(const FVector& LookAt, FName TaskId = NAME_None);

	/** Indoor task: the suit is hidden while the crew member is inside a module. */
	void SetInside(bool bInside);
	bool IsInside() const { return bInsideModule; }

	/** One-line status for the HUD label ("Deploying solar field 2") and task progress 0-1 (negative: none). */
	FText StatusText;
	float StatusProgress = -1.f;

	/** Short spoken line shown over the head for a few seconds (barks). */
	void Say(const FText& Line, float Seconds = 4.f);
	FText GetBark() const;

	FVector GetHomeLocation() const { return HomeLocation; }

	/** Runs the steps in order (walk, work, next), then calls OnAllDone. Status and progress feed the HUD label. */
	void RunSteps(TArray<FTDCrewStep> Steps, float WalkSpeedCmPerSec, TFunction<void()> OnAllDone);
	bool IsOnTask() const { return bOnTask; }

	/** EVA buddy (never alone outside): stays at the player's side, walks or hurries by distance, never teleports. */
	void StartEscort(AActor* Target);
	/** Stops following and walks home. */
	void StopEscort();
	bool IsEscorting() const { return EscortTarget.IsValid(); }

	/** Idle life: true while the crew member is free (no task, escort, staging or alarm) and can join a chat. */
	bool IsIdleFree() const;
	/** A colleague starts a short chat: face them and listen until Until (idle clock seconds). */
	void JoinChat(const ATDCrewMember* Other, float Seconds);
	FVector GetBaseLocation() const { return BaseLocation; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Suit")
	TObjectPtr<USkeletalMeshComponent> Suit;

private:
	/** Plays the clip (looping) on the rigged suit; no-op without a suit. */
	void PlayClip(UAnimSequence* Clip, float Rate = 1.f);
	void ApplySuitTint(const FLinearColor& Color);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> SuitMaterials;

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

	/** Idle life (actor Tick, frame rate): slow look-around and breathing; worried/alarm sway faster; celebrate hops. */
	void SetBasePose(const FVector& Location, const FRotator& Rotation);
	void StepIdle(float DeltaSeconds);

	/** Walking: path points from the base walk grid or navigation system, advanced every frame. */
	void StepWalk(float DeltaSeconds);
	float GroundZ(const FVector& At) const;
	TArray<FVector> PathPoints;
	int32 PathIndex = 0;
	float WalkSpeed = 140.f;
	float FeetOffset = 0.f;
	TFunction<void()> ArriveCallback;
	void RunNextStep();
	void StepEscort(float DeltaSeconds);
	TWeakObjectPtr<AActor> EscortTarget;
	bool bEscortMoving = false;
	// Escort detour: when a module stands between the buddy and its slot, follow the walk grid instead.
	bool bEscortDetour = false;
	float EscortCheckAt = 0.f;
	FVector EscortWaypoint = FVector::ZeroVector;
	bool EscortLineBlocked(const FVector& To) const;
	// Turn in place toward the first path leg before striding (suits turn slowly, EVA Ops 101 p.4).
	bool bTurningToPath = false;
	// Footsteps heard near the crew member (3D, fades out over about 15 m: thin Mars air carries little).
	float StepTravel = 0.f;
	void StepSound();
	// Idle spots used recently (idle clock seconds, keyed on a 50 cm grid), so the crew vary what they do.
	TMap<FIntPoint, float> IdleSpotUsed;
	FVector IdleClaim = FVector::ZeroVector; // the spot this crew member claimed (nobody else walks to it)
	TWeakObjectPtr<ATDCrewMember> ChatPartner;
	float NextChatAt = 0.f;
	float ClipSwapAt = -1.f;
	ATDCrewMember* FindChatPartner() const;
	int32 PickIdleSpot();
	void BeginInspect(const FVector& Look);
	void BeginChat(float Seconds);
	void BeginWorkPose(bool bGround, int32 VerbIndex);
	TArray<FTDCrewStep> StepQueue;
	TFunction<void()> StepsDone;
	float StepSpeed = 300.f;
	bool bOnTask = false;
	FTimerHandle WorkTimer;
	double WorkStart = 0.0;
	float WorkLength = 0.f;
	bool bInsideModule = false;
	FText Bark;
	double BarkUntil = 0.0;

	FVector HomeLocation;
	FRotator HomeRotation;
	FVector BaseLocation;
	FRotator BaseRotation;
	float IdleTime = 0.f;

	// Idle life (ambient behaviour when nothing is assigned). Smart-object style: small activities at points of
	// interest around home (instrument checks, a look at the horizon, a chat with a colleague, a short walk), each
	// with its own randomized length, so no crew member stands frozen in a loop and no two are in sync.
	enum class EIdleAct : uint8 { Relax, LookAround, Inspect, Chat, Listen, Stroll, ReturnHome };
	EIdleAct IdleAct = EIdleAct::Relax;
	float IdleActUntil = 0.f;
	FVector IdleFocus = FVector::ZeroVector;
	int32 IdleActsAway = 0;
	FRandomStream IdleRand;
	TArray<FVector> IdlePoints;   // stand spots in front of modules and work spots near home
	TArray<FVector> IdleLooks;    // what each stand spot faces
	bool bIdlePointsReady = false;
	void GatherIdlePoints();
	void StepIdleLife(float DeltaSeconds);
	void StartIdleAct(EIdleAct Act);
	void PickNextIdleAct();
	bool WalkIdleTo(const FVector& Target, TFunction<void()> OnArrived);
	FVector AvoidPush(const FVector& Dir) const;
	TArray<TWeakObjectPtr<class ALandscapeProxy>> Landscapes;

	// A6: a sub-range of the work clip played back and forth (squat for ground work, standing for waist work).
	bool bWorkRange = false;
	float WorkRangeStart = 0.f;
	float WorkRangeEnd = 0.f;
	void StepWorkRange();
	// A7: the tool in the right hand while working.
	UPROPERTY(Transient) TObjectPtr<class UStaticMeshComponent> HandToolA;
	UPROPERTY(Transient) TObjectPtr<class UStaticMeshComponent> HandToolB;
	void ShowHandTool(int32 VerbIndex);
	void HideHandTool();
	// A8: facing spring (turn the whole suit toward the player or the work, slowly, with a little overshoot).
	FFloatSpringState FacingSpring;
	void FaceToward(const FVector& Point, float DeltaSeconds, float ThresholdDegrees);
	float IdleSeed = 0.f;
	bool bStaged = false;
	FTransform PreShotTransform;
	ETDCrewBehaviour Current = ETDCrewBehaviour::Working;
};
