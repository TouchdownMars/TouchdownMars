// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/Object.h"
#include "Model/TDTypes.h"

#include "TDMissionDesign.generated.h"

class UDataTable;

/**
 * Mission design layer (Act 1, "Space Mission Design Game"). The player designs the cargo mission that lands before
 * the crew: launch vehicle and number of launches, power source, shelter, greenhouse, spares, science instruments and
 * the communications path. Every number comes from DT_TD_Constants through a key, so the Source Panel can show where
 * it comes from (NASA value or labelled designer value).
 */

/** One decision slot of the design. The order is the order of the design screens (NASA mission segments). */
UENUM(BlueprintType)
enum class ETDDesignSlot : uint8
{
	Launcher,
	Power,
	Shelter,
	Greenhouse,
	Spares,
	Instrument,
	Comms
};

/** Team roles (up to four players share one screen; one player plays all four alone). */
UENUM(BlueprintType)
enum class ETDRole : uint8
{
	Planner,
	Engineer,
	Science,
	Comms
};

namespace TD
{
	inline constexpr int32 NumRoles = 4;
	inline constexpr int32 NumDesignSlots = 7;
}

/** A design issue shown in the review list. Blocking issues lock "Launch". */
UENUM(BlueprintType)
enum class ETDDesignIssue : uint8
{
	OverMass,
	OverBudget,
	PowerShort,
	DataShort,
	MissingSlot,
	TooFewInstruments,
	RoleGoalMissed,
	ItemTooBig
};

USTRUCT(BlueprintType)
struct FTDDesignOptionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design")
	ETDDesignSlot Slot = ETDDesignSlot::Power;

	/** Role that owns the decision. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design")
	ETDRole Role = ETDRole::Engineer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design|Text")
	FName NameKey;

	/** One line: what it is. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design|Text")
	FName InfoKey;

	/** One line: why the choice matters (the trade-off). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design|Text")
	FName WhyKey;

	// Numbers: keys into DT_TD_Constants (None = 0). Mass in kg, power in kWe, money in million USD, data in Mb per sol.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design|Numbers")
	FName MassKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design|Numbers")
	FName PowerOutKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design|Numbers")
	FName PowerDrawKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design|Numbers")
	FName CostKey;

	/** Launchers: mass the vehicle sends to Mars per launch (kg). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design|Numbers")
	FName CapacityKey;

	/** Instruments: data made per sol. Comms: data the link returns per sol. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design|Numbers")
	FName DataKey;

	/** Instruments: science value (designer value). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design|Numbers")
	FName ScienceKey;

	// Effects on the surface simulation (Act 2).

	/** Solar: output falls to the dust-storm share (constants DustStormSolarShare, InSight) in a dust-storm phase. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design|Effects")
	bool bDustSensitive = false;

	/** Comms: a relay orbiter (more data per sol; Perseverance returns 99.9 % of its science data this way). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design|Effects")
	bool bRelay = false;

	/** Start-level bonus for the matching system (shelter, greenhouse). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design|Effects")
	int32 StartLevelBonus = 0;

	/** Shelter: severe radiation demand is reduced by this many cells. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design|Effects")
	int32 RadiationRelief = 0;

	/** Spares: equipment-failure cards cost this many fewer cells (per mission). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design|Effects")
	int32 RepairCharges = 0;

	/** Launcher: not yet flown to Mars; adds a launch-delay risk card. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design|Effects")
	bool bUnflown = false;

	/** Stays in Mars orbit (relay orbiter): counts against launch capacity but needs no landing system. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design|Effects")
	bool bNotLanded = false;

	/** Launchers: how many launches the player may buy. Instruments: 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design")
	int32 MaxCount = 1;

	/** Picked in a fresh design. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design")
	bool bDefault = false;

	/** Two-letter code used in the shareable design code. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design")
	FString Code;

	/** Credit line of the card photo (e.g. "NASA/JPL-Caltech"; in-game renders say so). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design")
	FString ImageCredit;

	/** Real hardware this option is based on (shown on the card), e.g. "Perseverance MOXIE". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design")
	FString BasedOn;
};

/** Mission objective (scenario). Sets the requirements and the score weights. */
USTRUCT(BlueprintType)
struct FTDObjectiveRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FName NameKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FName InfoKey;

	/** Score weights in percent; they add up to 100. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective|Weights")
	int32 WeightSuccess = 40;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective|Weights")
	int32 WeightScience = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective|Weights")
	int32 WeightEfficiency = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective|Weights")
	int32 WeightDecisions = 20;

	/** Requirements. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective|Requirements")
	int32 MinInstruments = 1;

	/** Minimum level of every system at lift-off (base left for the next crew). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective|Requirements")
	int32 MinEndLevel = 1;

	/** Science walks the crew must complete. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective|Requirements")
	int32 MinScienceWalks = 0;

	/** Constants key of the budget cap (million USD, designer value). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FName BudgetKey;

	/** Requirement lines shown on the objective card (text keys). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	TArray<FName> RequirementKeys;

	/** Sandbox: no budget cap, nothing is scored. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	bool bSandbox = false;

	/** Credit line of the objective picture. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FString ImageCredit;

	/** A design that launches, e.g. "FalconHeavy:2;Kilopower:3": the "Start from a working design" button. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FString RecommendedPicks;
};

/** One suggested change that resolves a review line ("Fix" button). */
USTRUCT(BlueprintType)
struct FTDDesignFix
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	bool bValid = false;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	FName OptionId;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	int32 NewCount = 0;
};

/** One picked option and how many (launches). */
USTRUCT(BlueprintType)
struct FTDDesignPick
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Design")
	FName OptionId;

	UPROPERTY(BlueprintReadWrite, Category = "Design")
	int32 Count = 1;
};

/** The player's design. */
USTRUCT(BlueprintType)
struct FTDMissionDesign
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Design")
	FName ObjectiveId;

	UPROPERTY(BlueprintReadWrite, Category = "Design")
	TArray<FTDDesignPick> Picks;

	/** Roles that signed off this version of the design (index = ETDRole). Any change clears them. */
	UPROPERTY(BlueprintReadWrite, Category = "Design")
	TArray<bool> SignedOff;

	/** Realistic model: the entry, descent and landing system takes a share of the transfer mass. */
	UPROPERTY(BlueprintReadWrite, Category = "Design")
	bool bRealisticModel = true;
};

USTRUCT(BlueprintType)
struct FTDDesignIssueLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	ETDDesignIssue Issue = ETDDesignIssue::MissingSlot;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	bool bBlocking = true;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	ETDRole Role = ETDRole::Planner;

	/** Numbers for the sentence (e.g. mass and capacity in tonnes); the UI formats Design_Issue_<Issue>. */
	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double ValueA = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double ValueB = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double ValueC = 0.0;

	/** MissingSlot: which slot. */
	UPROPERTY(BlueprintReadOnly, Category = "Design")
	ETDDesignSlot Slot = ETDDesignSlot::Launcher;

	/** ItemTooBig: which option (its NameKey is shown). */
	UPROPERTY(BlueprintReadOnly, Category = "Design")
	FName OptionId;
};

/** One launch and what it carries (each launch is a separate package; items are not split). */
USTRUCT(BlueprintType)
struct FTDLaunchPackage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	FName LauncherId;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double CapacityKg = 0.0;

	/** Payload + landing hardware (or orbital payload). */
	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double UsedKg = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double LandingHardwareKg = 0.0;

	/** Option ids carried (consumables use the id "Consumables"). */
	UPROPERTY(BlueprintReadOnly, Category = "Design")
	TArray<FName> Items;
};

/** The live dashboard numbers of a design. */
USTRUCT(BlueprintType)
struct FTDDesignBudget
{
	GENERATED_BODY()

	/** Cargo sent to Mars: hardware + consumables (+ landing system share in the realistic model), kg. */
	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double MassKg = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double HardwareKg = 0.0;

	/** Crew consumables for the surface stay (BVAD values x crew x sols), kg. */
	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double ConsumablesKg = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double LandingSystemKg = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double CapacityKg = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double CostM = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double BudgetM = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double PowerOutKw = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double PowerDrawKw = 0.0;

	/** Power cells per phase on the surface (clear sky). */
	UPROPERTY(BlueprintReadOnly, Category = "Design")
	int32 Cells = 0;

	/** Power cells in a dust-storm phase. */
	UPROPERTY(BlueprintReadOnly, Category = "Design")
	int32 DustCells = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double DataMadeMb = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double DataLinkMb = 0.0;

	/** Data over the whole stay (stored on Mars, sent when a link is open; lost only when storage overflows). */
	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double DataMadeTotalMb = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double DataReturnedMb = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double DataLostMb = 0.0;

	/** Still in storage at lift-off (not yet sent). */
	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double DataStoredMb = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double StorageMb = 0.0;

	/** Launch packages after packing (first fit, largest item first). */
	UPROPERTY(BlueprintReadOnly, Category = "Design")
	TArray<FTDLaunchPackage> Packages;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	double Science = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	int32 Instruments = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	int32 Launches = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	bool bRelay = false;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	bool bUnflown = false;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	int32 StartBonusShelter = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	int32 StartBonusGreenhouse = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	int32 RadiationRelief = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	int32 RepairCharges = 0;

	/** JPL Concept Maturity Level reached by the design (1 = objective chosen ... 4 = point design, signed off). */
	UPROPERTY(BlueprintReadOnly, Category = "Design")
	int32 ConceptMaturity = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	TArray<FTDDesignIssueLine> Issues;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	bool bLaunchable = false;
};

/** Score of a flown mission (Act 3). Every axis 0-100; Total uses the objective weights. */
USTRUCT(BlueprintType)
struct FTDMissionScore
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	int32 Success = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	int32 Science = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	int32 Efficiency = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	int32 Decisions = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	int32 Total = 0;

	/** Team play: how many role goals were met together (0-4). */
	UPROPERTY(BlueprintReadOnly, Category = "Score")
	int32 RoleGoalsMet = 0;

	/** One "why" text key per axis (Success, Science, Efficiency, Decisions), picked from what happened. */
	UPROPERTY(BlueprintReadOnly, Category = "Score")
	TArray<FName> WhyKeys;

	/** The objective's acceptance criteria (text keys) and whether each passed. */
	UPROPERTY(BlueprintReadOnly, Category = "Score")
	TArray<FName> RequirementKeys;

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	TArray<bool> RequirementMet;
};

/** What the design hands to the surface simulation (Act 2). */
USTRUCT(BlueprintType)
struct FTDDesignEffects
{
	GENERATED_BODY()

	bool bValid = false;
	int32 Cells = 6;
	int32 DustCells = 6;
	bool bRelay = false;
	int32 StartBonusShelter = 0;
	int32 StartBonusGreenhouse = 0;
	int32 RadiationRelief = 0;
	int32 RepairCharges = 0;
	int32 Instruments = 0;
	double DataMadeMb = 0.0;
	double DataLinkMb = 0.0;
};

/**
 * Pure design rules: evaluate a design (mass, power, money, data), check launch compatibility, apply the design to
 * the surface rules and score the flown mission. No world, no UI; deterministic.
 */
UCLASS()
class TOUCHDOWNMARS_API UTDDesignRules : public UObject
{
	GENERATED_BODY()

public:
	/** Constant value by key; 0 when the key is None or missing. */
	static double Constant(const UDataTable* Constants, FName Key);

	static bool IsMultiPick(ETDDesignSlot Slot);

	/** Default picks (bDefault rows) for the objective. */
	static FTDMissionDesign MakeDefaultDesign(const UDataTable* Options, FName ObjectiveId);

	/**
	 * Set how many of an option are picked (0 removes it). Single-pick slots replace the previous pick. Clamped to
	 * MaxCount. Clears all sign-offs. Returns false if nothing changed.
	 */
	static bool SetPick(FTDMissionDesign& Design, const UDataTable* Options, FName OptionId, int32 Count);

	static int32 GetPickCount(const FTDMissionDesign& Design, FName OptionId);

	/** Live dashboard and review list. */
	static FTDDesignBudget Evaluate(const FTDMissionDesign& Design, const UDataTable* Options, const UDataTable* Objectives,
		const UDataTable* Constants);

	/** Whether a role's own goal is met by the current numbers (the review screen shows all four). */
	static bool IsRoleGoalMet(ETDRole Role, const FTDDesignBudget& Budget, const FTDObjectiveRow* Objective, const UDataTable* Constants);

	static FTDDesignEffects MakeEffects(const FTDDesignBudget& Budget);

	/** Score the finished mission. */
	static FTDMissionScore Score(const FTDMissionDesign& Design, const FTDDesignBudget& Budget, const FTDObjectiveRow* Objective,
		const FTDMissionState& State, const FTDMissionReport& Report, int32 ScienceWalks, int32 GoodDecisions, int32 Decisions,
		const UDataTable* Constants);

	/** The objective's recommended design (it launches). Keeps the model setting. */
	static FTDMissionDesign MakeRecommendedDesign(const UDataTable* Options, const UDataTable* Objectives, FName ObjectiveId, bool bRealisticModel);

	/** One change that resolves a review line, if there is a simple one. */
	static FTDDesignFix SuggestFix(const FTDDesignIssueLine& Issue, const FTDMissionDesign& Design, const UDataTable* Options, const UDataTable* Constants);

	/** Short design code for sharing and comparing, e.g. "FH3-KP3-SW-GS-SP-I4-RL". */
	static FString MakeDesignCode(const FTDMissionDesign& Design, const UDataTable* Options);
};
