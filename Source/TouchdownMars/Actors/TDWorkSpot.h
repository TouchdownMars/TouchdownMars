// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
// Hands-on work (research note hands_on_interaction_hud.md §3.6 / §6.1): every task the player does in person is
// a set of physical work spots (stakes, sockets, locks, latches...). The player walks to the lit spot and HOLDS
// E / X; the work comes in beats (hammer swings, ratchet clicks), each beat changes the object for good.
// Also the Mission Control station: the one console where power, crew plan and decisions are set.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDWorkSpot.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UMaterialInterface;

UENUM()
enum class ETDWorkVerb : uint8
{
	Hammer, // survey / guy-line stakes
	Plug,   // power and data cables: seat, push, twist-lock
	Wrench, // launch locks and bolts
	Latch,  // cargo crate latches
	Brush,  // dust off solar panels
	Drill,  // rock cores
	Scan,   // label scanner (inventory, harvest trays)
	Scoop,  // regolith into a hopper or onto a berm
	Switch  // service-panel toggles (suit check, maintenance)
};

/** How one task is done by hand: verb, number of spots, beats per spot, seconds per beat. [DESIGNER VALUE] */
struct FTDHandsOnSpec
{
	ETDWorkVerb Verb = ETDWorkVerb::Hammer;
	int32 Spots = 3;
	int32 Beats = 3;
	float BeatSeconds = 0.55f;
};

namespace TDHandsOn
{
	TOUCHDOWNMARS_API FTDHandsOnSpec SpecFor(FName TaskId);
	TOUCHDOWNMARS_API const TCHAR* VerbKey(ETDWorkVerb Verb);
}

/** One physical work spot. Spawned by the astronaut for the task in reach; destroyed when the task is done. */
UCLASS(NotBlueprintable)
class TOUCHDOWNMARS_API ATDWorkSpot : public AActor
{
	GENERATED_BODY()

public:
	ATDWorkSpot();

	void Setup(ETDWorkVerb InVerb, int32 InBeats, float InFaceYaw);
	/** One unit of work landed: the object changes for good. Returns true when this spot is finished. */
	bool Beat();
	void SetActive(bool bInActive);
	bool IsDone() const { return BeatsDone >= Beats; }
	int32 GetBeatsDone() const { return BeatsDone; }
	int32 GetBeats() const { return Beats; }
	float GetProgress() const { return Beats > 0 ? float(BeatsDone) / float(Beats) : 1.f; }
	ETDWorkVerb GetVerb() const { return Verb; }
	/** Where the tool hits (for the ring and the dust puff). */
	FVector GetWorkPoint() const;

	virtual void Tick(float DeltaSeconds) override;

private:
	UStaticMeshComponent* AddPart(UStaticMesh* Mesh, const FVector& Loc, const FVector& Scale, const FRotator& Rot, UMaterialInterface* Mat);
	void SetLamp(const FLinearColor& Color);
	void Puff(const FVector& At, int32 Count);
	void ApplyVisual();

	UPROPERTY() TObjectPtr<USceneComponent> Root;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Moving;   // the part the work moves (stake, plug, bolt, dust, bit...)
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Extra;    // flag, lid, hopper fill
	UPROPERTY() TObjectPtr<UStaticMeshComponent> LampMesh;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Marker;   // floating diamond over the active spot
	UPROPERTY() TObjectPtr<UPointLightComponent> Light;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> LampMat;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Puffs;
	TArray<FVector> PuffVelocity;
	TArray<float> PuffAge;

	ETDWorkVerb Verb = ETDWorkVerb::Hammer;
	int32 Beats = 3;
	int32 BeatsDone = 0;
	bool bActive = false;
	float Time = 0.f;
	float BeatFlash = 0.f;
	FVector MovingStart = FVector::ZeroVector;
	// A5: ApplyVisual sets targets; Tick eases the parts there (the object changes smoothly on every beat).
	FTransform MovingTarget;
	FTransform ExtraTarget;
	bool bHasTargets = false;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> ScanBar;
	float DustGravity = 300.f;
	float ScanSweep = 0.f;
	float ScoopPileZ = 0.5f;   // pile height scale: basic sphere 0.5, Tripo mound 1.43
	FVector ExtraScale = FVector::OneVector;
};

/** The Mission Control console next to the habitat (walk up, press E / X). */
UCLASS(NotBlueprintable)
class TOUCHDOWNMARS_API ATDMissionControlStation : public AActor
{
	GENERATED_BODY()

public:
	ATDMissionControlStation();
	virtual void Tick(float DeltaSeconds) override;
	/** Where the player stands to use it. */
	FVector GetUsePoint() const;
	/** The beacon pulses when Mission Control needs the player (power not routed, decision waiting). */
	void SetNeedsYou(bool bNeeds) { bNeedsYou = bNeeds; }

	static ATDMissionControlStation* Find(const UWorld* World);
	/** Spawns the console once per world beside the habitat (layout [DESIGNER VALUE]). */
	static ATDMissionControlStation* Ensure(UWorld* World);

private:
	UPROPERTY() TObjectPtr<USceneComponent> Root;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Screen;
	/** Diegetic readout on the screen: one bar per system (level), one lamp per power cell (placed or free). */
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Bars;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> BarMats;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> CellLamps;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> CellMats;
	float ReadoutTimer = 0.f;
	void UpdateReadout();
	UPROPERTY() TObjectPtr<UPointLightComponent> ScreenLight;
	UPROPERTY() TObjectPtr<UPointLightComponent> BeaconLight;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ScreenMat;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BeaconMat;
	bool bNeedsYou = false;
	float Time = 0.f;
};
