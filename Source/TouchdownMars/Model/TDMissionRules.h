// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Model/TDTypes.h"
#include "Model/TDMissionDesign.h"
#include "TDMissionRules.generated.h"

class UDataTable;

/** Everything the rules need, built from DT_TD_Phases, DT_TD_AgeBands and DT_TD_Constants. */
USTRUCT(BlueprintType)
struct FTDRulesConfig
{
	GENERATED_BODY()

	/** Indexed by ETDPhase (6 rows). */
	UPROPERTY(BlueprintReadOnly, Category = "Rules")
	TArray<FTDPhaseRow> Phases;

	UPROPERTY(BlueprintReadOnly, Category = "Rules")
	FTDAgeBandRow BandRow;

	/** Indexed by ETDSystem. */
	UPROPERTY(BlueprintReadOnly, Category = "Rules")
	TArray<int32> StartLevels;

	UPROPERTY(BlueprintReadOnly, Category = "Rules")
	int32 LevelMin = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Rules")
	int32 LevelMax = 4;

	UPROPERTY(BlueprintReadOnly, Category = "Rules")
	int32 ShelterSafeLevel = 2;

	UPROPERTY(BlueprintReadOnly, Category = "Rules")
	int32 HarvestMinLevel = 3;

	/** Cadet-locked systems are held at this level. */
	UPROPERTY(BlueprintReadOnly, Category = "Rules")
	int32 LockedSystemLevel = 3;

	/** Effects of the player's mission design (Act 1). Not valid = the band's fixed cells (no design). */
	FTDDesignEffects Design;

	bool IsValid() const { return Phases.Num() == TD::NumPhases && StartLevels.Num() == TD::NumSystems; }
};

/** One event card in the deck: its row name and row. */
USTRUCT(BlueprintType)
struct FTDDeckEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Rules")
	FName EventId;

	UPROPERTY(BlueprintReadOnly, Category = "Rules")
	FTDEventRow Row;
};

/** What EndPhase changed, for delegates and cause sentences. */
USTRUCT(BlueprintType)
struct FTDPhaseOutcome
{
	GENERATED_BODY()

	/** Level change applied now, indexed by ETDSystem (GREENHOUSE is applied next phase and reported in PendingGreenhouseDelta). */
	UPROPERTY(BlueprintReadOnly, Category = "Rules")
	TArray<int32> LevelChanges;

	UPROPERTY(BlueprintReadOnly, Category = "Rules")
	int32 PendingGreenhouseDelta = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Rules")
	bool bStormExposure = false;

	UPROPERTY(BlueprintReadOnly, Category = "Rules")
	bool bLost = false;

	/** Text key of the one-reason sentence (DT_TD_Text), e.g. Cause_Air_Down. */
	UPROPERTY(BlueprintReadOnly, Category = "Rules")
	FName CauseKey;
};

UENUM(BlueprintType)
enum class ETDAllocationPolicy : uint8
{
	/** Every cell goes to a random unlocked system; random event choice. */
	Random,
	/** Cover the lowest systems' demand first; take the cheaper event choice. */
	Greedy
};

USTRUCT(BlueprintType)
struct FTDBalanceReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Balance")
	int32 Missions = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Balance")
	int32 Losses = 0;

	/** Indexed by ETDPhase. */
	UPROPERTY(BlueprintReadOnly, Category = "Balance")
	TArray<int32> LossesByPhase;

	UPROPERTY(BlueprintReadOnly, Category = "Balance")
	int32 FullMissions = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Balance")
	int32 MinimumMissions = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Balance")
	int32 MissionsWithEvents = 0;
};

/**
 * Pure mission rules (gdd.md §2 Mechanics 1-8). No world, no UI; every function is deterministic for a given
 * state and seed. UTDMissionSubsystem owns the state and calls these functions.
 */
UCLASS()
class TOUCHDOWNMARS_API UTDMissionRules : public UObject
{
	GENERATED_BODY()

public:
	// Config

	static bool MakeConfig(const UDataTable* PhasesTable, const UDataTable* AgeBandsTable, const UDataTable* ConstantsTable,
		ETDAgeBand Band, FTDRulesConfig& OutConfig, FString& OutError);

	static TArray<FTDDeckEntry> MakeDeck(const UDataTable* EventsTable);

	// Mission flow

	/** Fresh state; call StartPhase next. */
	static FTDMissionState NewMission(const FTDRulesConfig& Config, ETDAgeBand Band, int32 Seed);

	/** Phase indices (ETDPhase) this band plays, in order. Cadet merges Arrival and FirstWalk into Arrival. */
	static TArray<int32> GetPlayablePhases(const FTDRulesConfig& Config, ETDAgeBand Band);

	/** Applies the delayed GREENHOUSE change, resets the allocation, checks the harvest, draws the event. */
	static void StartPhase(const FTDRulesConfig& Config, FTDMissionState& State, const TArray<FTDDeckEntry>& Deck);

	static bool IsSystemLocked(const FTDRulesConfig& Config, const FTDMissionState& State, ETDSystem System);

	static int32 GetCellsPlaced(const FTDMissionState& State);

	static bool CanPlaceCell(const FTDRulesConfig& Config, const FTDMissionState& State, ETDSystem System);

	/** All cells placed and, if an event is active, a choice made. */
	static bool CanEndPhase(const FTDRulesConfig& Config, const FTDMissionState& State);

	/** Demand per system this phase including the chosen event option; locked systems demand 0. */
	static TArray<int32> GetDemand(const FTDRulesConfig& Config, const FTDMissionState& State, const FTDEventRow* Event);

	/** Level change for cells given against demand: >= +1 → +1, 0 → 0, -1 → -1, <= -2 → -2. */
	static int32 GetLevelChange(int32 Cells, int32 Demand);

	/** Applies allocation and event, records exposure/science, checks the lose rule, advances sols. Does not advance the phase. */
	static FTDPhaseOutcome EndPhase(const FTDRulesConfig& Config, FTDMissionState& State, const FTDEventRow* Event);

	/** Moves to the next playable phase. Returns false when the mission is over (lift-off or lost) and sets Result. */
	static bool AdvancePhase(const FTDRulesConfig& Config, FTDMissionState& State, const TArray<FTDDeckEntry>& Deck);

	// Events

	/** Same seed and phase → same card. Prefers the phase's family; never repeats a card in one game. */
	static FName DrawEvent(const FTDRulesConfig& Config, const FTDMissionState& State, const TArray<FTDDeckEntry>& Deck);

	/** A choice costs something when it raises any demand or records an exposure (pillar P1). */
	static bool ChoiceHasCost(const FTDEventRow& Row, bool bChoiceA);

	// Mission Control

	/** Adds a message; returns false if already called this phase or the phase is a blackout. */
	static bool CallHome(const FTDRulesConfig& Config, FTDMissionState& State);

	/** Weakest system (ETDSystem index); ties go AIR, then SHELTER, then GREENHOUSE (crew first, then base). */
	static int32 WeakestSystem(const FTDMissionState& State);

	/** Removes and returns messages due now; nothing is delivered during a blackout phase. */
	static TArray<FTDPendingMessage> TakeDueMessages(const FTDRulesConfig& Config, FTDMissionState& State);

	// Report

	static FTDMissionReport MakeReport(const FTDRulesConfig& Config, const FTDMissionState& State);

	// Balance (Scripts/balance_check.py)

	static FTDBalanceReport Simulate(const FTDRulesConfig& Config, const TArray<FTDDeckEntry>& Deck, ETDAgeBand Band,
		ETDAllocationPolicy Policy, int32 FirstSeed, int32 NumSeeds);

	/** Python/Blueprint entry: loads config from the tables and runs Simulate. */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Balance")
	static FTDBalanceReport SimulateFromTables(const UDataTable* PhasesTable, const UDataTable* AgeBandsTable,
		const UDataTable* ConstantsTable, const UDataTable* EventsTable, ETDAgeBand Band, ETDAllocationPolicy Policy,
		int32 FirstSeed = 1000, int32 NumSeeds = 1000);

	static const FTDEventRow* FindEvent(const TArray<FTDDeckEntry>& Deck, FName EventId);
};
