// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/Object.h"
#include "Model/TDTypes.h"
#include "Model/TDMissionDesign.h"

#include "TDOperations.generated.h"

/**
 * Surface operations: the crew of four sets up the landed cargo, runs science
 * traverses, prospects for water, keeps the solar arrays clean and the base running. Each phase the player plans
 * the crew's hours on the base computer; the phase then runs as a time-lapse in which the crew walk to the sites
 * and do the work. Pure rules here, no engine objects: tests in Tests/TDOperationsTests.cpp.
 */

/** What a finished task does to the mission. */
UENUM(BlueprintType)
enum class ETDTaskEffect : uint8
{
	None,
	Unpack,        // cargo out of the lander: unlocks the tasks that need it
	SetUpGreenhouse, // greenhouse running: +1 GREENHOUSE level, harvest possible later
	Harvest,       // first harvest: food and morale
	ScienceTraverse, // a science walk/drive with the instruments (counts for the objective)
	Survey,        // instrument survey for water-bearing minerals: unlocks extraction
	ExtractWater,  // water from hydrated regolith: +1 AIR level (oxygen from electrolysis)
	BuildBerm,     // regolith piled on the shelter: +1 radiation relief
	CleanSolar,    // dust brushed off the arrays: dust losses cleared
	Maintain,      // filters and seals: +1 lowest system level
	Weather,       // weather station running: dust storms show one phase earlier
	LinkCheck,     // antenna or relay checked out: Call home works this phase
	Expedition,    // multi-sol rover traverse: science walks, then the rover recharges for a phase
	Seismic        // one InSight catalogue record studied: a science walk
};

/** A place in the base where crew work or pass through (Data/Source/sites.csv). Coordinates in cm, level space. */
USTRUCT(BlueprintType)
struct FTDSiteRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	float X = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	float Y = 0.f;

	/** Sites reachable on foot in a straight line (paths follow these links). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	TArray<FName> Links;

	/** Inside a pressurised module: the crew member is hidden while working there. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	bool bInside = false;

	/** Point the crew face while working (the facility). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	float LookX = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	float LookY = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	FName NameKey;
};

/** One kind of crew task (Data/Source/tasks.csv). */
USTRUCT(BlueprintType)
struct FTDTaskRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	FName NameKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	FName InfoKey;

	/** Phases (bit = ETDPhase index) in which the task can be planned; 0 = every phase. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	int32 PhaseMask = 0;

	/** Where the work happens (sites.csv row). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	FName Site;

	/** People who must work together (EVA tasks: at least two, the buddy rule). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	int32 Crew = 1;

	/** Constant (constants.csv) with the hours the task takes for that team. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	FName HoursKey;

	/** Outside in a suit: EVA hour limits apply, no EVA in a dust storm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	bool bEva = false;

	/** Design option that must have landed (none = always). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	FName RequiresOption;

	/** Task that must be finished first (none = no prerequisite). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	FName Prerequisite;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	ETDTaskEffect Effect = ETDTaskEffect::None;

	/** Role that prefers this work when the crew picks tasks themselves. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	ETDRole Role = ETDRole::Engineer;

	/** Can be done again in a later phase. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	bool bRepeatable = false;

	/** Power cells the task draws while it runs (taken from the phase's cells). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	int32 Cells = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ops")
	TArray<FName> SourceIds;
};

/** One crew member's plan for the phase. */
USTRUCT(BlueprintType)
struct FTDCrewPlan
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Ops")
	ETDRole Role = ETDRole::Engineer;

	/** Tasks assigned this phase, in order. */
	UPROPERTY(BlueprintReadOnly, Category = "Ops")
	TArray<FName> Tasks;

	/** The player's own astronaut (does the tasks on foot; the others are AI crew). */
	UPROPERTY(BlueprintReadOnly, Category = "Ops")
	bool bPlayer = false;
};

/** Operations state for the whole mission. */
USTRUCT(BlueprintType)
struct FTDOpsState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Ops")
	TArray<FTDCrewPlan> Crew;

	/** Times each task has been finished. */
	UPROPERTY(BlueprintReadOnly, Category = "Ops")
	TMap<FName, int32> Done;

	UPROPERTY(BlueprintReadOnly, Category = "Ops")
	float WaterKg = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Ops")
	float RegolithKg = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Ops")
	bool bWaterFound = false;

	/** Dust on the solar arrays after a dust storm (cells lost until cleaned). */
	UPROPERTY(BlueprintReadOnly, Category = "Ops")
	int32 SolarDustCells = 0;

	/** Weather station running: dust storms show one phase early. */
	UPROPERTY(BlueprintReadOnly, Category = "Ops")
	bool bWeatherStation = false;

	/** Hours of each crew member planned this phase (index = crew). */
	UPROPERTY(BlueprintReadOnly, Category = "Ops")
	TArray<float> PlannedHours;

	/** Phases of fatigue left per crew member (rest-sol rule broken): work hours cut while > 0. */
	UPROPERTY(BlueprintReadOnly, Category = "Ops")
	TArray<int32> Fatigue;

	/** Crew members planned beyond their spacewalk hours this phase (the rest-sol rule overridden). */
	UPROPERTY(BlueprintReadOnly, Category = "Ops")
	TArray<int32> RestBroken;

	/** Times the rest-sol rule was broken this mission (shown in the report). */
	UPROPERTY(BlueprintReadOnly, Category = "Ops")
	int32 RestBreaks = 0;

	/** Phase in which the rover is recharging after an expedition (-1: none). */
	UPROPERTY(BlueprintReadOnly, Category = "Ops")
	int32 RoverChargingPhase = -1;
};

/** Why a task cannot be planned right now. */
UENUM(BlueprintType)
enum class ETDTaskBlock : uint8
{
	Ok,
	WrongPhase,
	NotLanded,      // the design did not bring what it needs
	NeedsFirst,     // prerequisite not done
	AlreadyDone,
	NoHours,        // crew member out of hours this phase
	NoEvaHours,     // EVA hours used up
	Storm,          // no EVA during a dust storm
	NeedsBuddy,     // EVA needs a second free crew member
	RoverCharging   // the rover is recharging after an expedition
};

UCLASS()
class TOUCHDOWNMARS_API UTDOps : public UObject
{
	GENERATED_BODY()

public:
	/** Four crew: the player (Engineer role, AIR) and three AI crew (Planner, Science, Comms roles). */
	static FTDOpsState MakeState();

	/** Work hours each crew member has in a phase of Sols sols (CrewWorkHoursPerSol from constants.csv). */
	static float PhaseHours(int32 Sols, const UDataTable* Constants);

	/** Work hours of one crew member this phase: fatigue (rest-sol rule broken) cuts them (FatigueWorkShare). */
	static float CrewPhaseHours(const FTDOpsState& Ops, int32 CrewIndex, int32 Sols, const UDataTable* Constants);

	/** EVA hours each crew member may spend outside in a phase (EvaHoursPerWeek scaled to the phase). */
	static float PhaseEvaHours(int32 Sols, const UDataTable* Constants);

	static float TaskHours(const FTDTaskRow& Row, const UDataTable* Constants);

	/** Can this task be planned for crew member CrewIndex (plus a buddy when EVA) this phase? */
	static ETDTaskBlock CanPlan(const FTDOpsState& Ops, const UDataTable* Tasks, FName TaskId, int32 CrewIndex, const FTDMissionDesign& Design,
		const UDataTable* Options, int32 PhaseIndex, int32 PhaseSols, bool bDustStorm, const UDataTable* Constants, bool bOverrideRest = false);

	/** Plans the task for CrewIndex and, for team tasks, the free crew members with the most hours left. */
	static bool Plan(FTDOpsState& Ops, const UDataTable* Tasks, FName TaskId, int32 CrewIndex, const FTDMissionDesign& Design,
		const UDataTable* Options, int32 PhaseIndex, int32 PhaseSols, bool bDustStorm, const UDataTable* Constants, bool bOverrideRest = false);

	/** Which crew members are planned beyond their spacewalk hours (the rest-sol rule overridden). */
	static void RecomputeRestBroken(FTDOpsState& Ops, const UDataTable* Tasks, int32 PhaseSols, const UDataTable* Constants);

	/** Removes the task from every crew member's plan. */
	static void Unplan(FTDOpsState& Ops, FName TaskId);

	/** Crew members who have TaskId planned. */
	static TArray<int32> CrewOn(const FTDOpsState& Ops, FName TaskId);

	/** The crew pick work themselves by role (RimWorld-style priorities): fills free hours with available tasks. */
	static void AutoPlan(FTDOpsState& Ops, const UDataTable* Tasks, const FTDMissionDesign& Design, const UDataTable* Options,
		int32 PhaseIndex, int32 PhaseSols, bool bDustStorm, const UDataTable* Constants);

	/** End of the phase run: every planned task is finished; returns the finished task ids in order. Clears the plan. */
	static TArray<FName> Finish(FTDOpsState& Ops, const UDataTable* Tasks);

	/** Applies a finished task to the mission state (levels, relief, walks) and to the ops state. */
	static void ApplyEffect(const FTDTaskRow& Row, FTDOpsState& Ops, FTDMissionState& State, const UDataTable* Constants);

	/** Shortest path between two sites over the site links (Dijkstra on straight-line lengths). */
	static TArray<FName> FindPath(const UDataTable* Sites, FName From, FName To);

	static bool IsDone(const FTDOpsState& Ops, FName TaskId) { return Ops.Done.FindRef(TaskId) > 0; }
};
