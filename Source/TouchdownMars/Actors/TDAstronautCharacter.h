// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
// First-person astronaut: walk the outpost on Mars gravity; the mission console (the existing decision HUD)
// opens on the wrist tablet (Tab). Input actions are created in code (no input assets needed).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "TDAstronautCharacter.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class ATDWorkSpot;
class ATDMissionControlStation;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/** What the helmet HUD shows for hands-on work and the Mission Control console. */
struct FTDWorkUi
{
	bool bShow = false;
	bool bStation = false;  // the prompt is "use Mission Control"
	bool bHolding = false;
	FText Prompt;           // "Hold E / X: hammer stake 2/4"
	float Ring = 0.f;       // progress of the spot in reach, 0-1, in beat steps
	float BeatPulse = 0.f;  // 1 at a hit, fades (ring flash)
};

/** One task's work spots in the world (spawned while the task is yours). */
struct FTDWorkSet
{
	FName Task;
	TArray<TWeakObjectPtr<ATDWorkSpot>> Spots;
	double FinishedAt = -1.0;
};

UCLASS()
class TOUCHDOWNMARS_API ATDAstronautCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATDAstronautCharacter();

	/** Eye height above the capsule centre (cm); capsule half height 92 cm puts the eyes near 1.62 m. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Astronaut")
	float EyeHeight = 70.f;

	/** Suited walking speed (cm/s), a designer value. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Astronaut")
	float WalkSpeed = 140.f;

	/** Mouse look sensitivity (degrees per mouse unit). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Astronaut")
	float LookSensitivity = 0.5f;

	/** Field of view (degrees, horizontal); 90 on a monitor (accessibility guidance). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Astronaut")
	float FieldOfView = 90.f;

	/** How far the helmet looks for a named object (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Astronaut")
	float InspectDistance = 900.f;

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Astronaut")
	bool IsConsoleOpen() const { return bConsoleOpen; }

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Astronaut")
	void SetConsoleOpen(bool bOpen);

	/** Name of what the astronaut looks at (crew member, system, vehicle), empty if nothing. */
	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Astronaut")
	FText GetFocusText() const { return FocusText; }

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Astronaut")
	float GetMarsGravityRatio() const { return GravityRatio; }

	/** Prompt for the HUD when one of your own tasks is in reach ("E: Off-load the rover"). */
	FText GetInteractPrompt() const;

	/** Hands-on work state for the HUD (prompt, hold ring). */
	FTDWorkUi GetWorkUi() const;
	/** The next unfinished spot of a task you own (nearest first) and the count done; false if none. */
	bool GetTaskWork(FName Task, int32& OutDoneSpots, int32& OutSpots, FVector& OutNext) const;
	bool IsNearStation() const;
	/** Seconds since the player last pressed TAB/Y away from Mission Control (HUD shows the way). */
	double GetStationPingTime() const { return StationPingTime; }
	/** Dev (-TDWorkTest / -TDSurface): stand in front of the next work spot of a task, or of Mission Control. */
	void DevGoToNextSpot(FName Task);
	void DevGoToStation();
	void DevSetHold(bool bHold) { bInteractHeld = bHold; }
	/** True while E / X is physically down (hold mode works only while it is). */
	bool bInteractKeyDown = false;
	/** Tap to work: one press latched the spot in reach; cleared by finishing, leaving, moving or a screen. */
	bool bTapLatched = false;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Astronaut")
	TObjectPtr<UCameraComponent> Camera;

private:
	void CreateInput();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ToggleConsole();
	void UpdateFocus();
	bool ApplyMarsGravity();
	void SyncHud();

	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> Context;
	UPROPERTY(Transient) TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LookAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> ConsoleAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> OpsAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> InteractAction;
	void ToggleOps();
	void Interact();
	void InteractReleased();
	void TickAmbience();
	UPROPERTY(Transient) TObjectPtr<class UAudioComponent> WindSound;
	UPROPERTY(Transient) TObjectPtr<class UAudioComponent> HumSound;
	void UseStation();

	// Hands-on work
	void SyncWorkSets();
	void SpawnWorkSet(FName Task);
	ATDWorkSpot* SpotInReach() const;
	void TickWork(float DeltaSeconds);
	void BuildTool(int32 VerbIndex);
	void PoseTool(float DeltaSeconds, bool bNear);
	void LandBeat(ATDWorkSpot* Spot);
	TArray<FTDWorkSet> WorkSets;
	UPROPERTY(Transient) TArray<TObjectPtr<ATDWorkSpot>> SpotRefs;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> ToolRoot;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> ToolA;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> ToolB;
	int32 ToolVerb = -1;
	bool bInteractHeld = false;
	bool bWorking = false;
	float WorkAccum = 0.f;
	float WorkBeatSeconds = 0.5f;
	float ToolOut = 0.f;      // 0 lowered, 1 raised
	float Kick = 0.f;         // camera kick after a hit
	float BeatPulse = 0.f;
	float SyncTimer = 0.f;
	// A3: springs that give the tool weight (lag on turns, lean on side steps, dip on landing).
	struct FToolSprings { float Yaw = 0.f, Pitch = 0.f, Roll = 0.f, Dip = 0.f; };
	FToolSprings ToolSpring;
	FFloatSpringState YawSpring, PitchSpring, RollSpring, DipSpring;
	FRotator LastControl = FRotator::ZeroRotator;
	float BobPhase = 0.f;
	virtual void Landed(const FHitResult& Hit) override;
	double StationPingTime = -100.0;
	/** Your planned task whose site is within reach (none if far). */
	FName NearbyOwnTask() const;

	bool bConsoleOpen = false;
	bool bGravityApplied = false;
	bool bHudSynced = false;
	float GravityRatio = 1.f;
	FText FocusText;
};
