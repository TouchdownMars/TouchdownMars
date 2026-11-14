// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "TDTypes.generated.h"

// Enums (technical_design.md §2.1)

UENUM(BlueprintType)
enum class ETDSystem : uint8
{
	Air,
	Greenhouse,
	Shelter
};

UENUM(BlueprintType)
enum class ETDAgeBand : uint8
{
	Cadet,
	Commander
};

UENUM(BlueprintType)
enum class ETDPhase : uint8
{
	Arrival,
	FirstWalk,
	Exploration,
	Resupply,
	HardWeek,
	Return
};

UENUM(BlueprintType)
enum class ETDEventFamily : uint8
{
	None,
	Power,
	Radiation,
	Comms,
	Equipment,
	Food
};

UENUM(BlueprintType)
enum class ETDCardLabel : uint8
{
	FlownMeasured,
	NASAPlan,
	AnalogMission,
	ModelEstimate
};

/** Extra effect of an event choice besides demand changes. */
UENUM(BlueprintType)
enum class ETDChoiceExtra : uint8
{
	None,
	StormExposure,
	ScienceBonus,
	ScienceWithExposure
};

UENUM(BlueprintType)
enum class ETDDataStatus : uint8
{
	Live,
	Archive
};

UENUM(BlueprintType)
enum class ETDCrewBehaviour : uint8
{
	Working,
	Worried,
	Alarm,
	Shelter,
	AtLander,
	Celebrate
};

UENUM(BlueprintType)
enum class ETDMissionResult : uint8
{
	InProgress,
	LostAir,
	Minimum,
	Full
};

UENUM(BlueprintType)
enum class ETDLanguage : uint8
{
	EN,
	TR
};

namespace TD
{
	inline constexpr int32 NumSystems = 3;
	inline constexpr int32 NumPhases = 6;
}

// DataTable rows (technical_design.md §2.2). Imported from Data/Source/*.csv by Scripts/import_data.py.
// The CSV row name is the key (PhaseId, EventId, Band, constant Key, SourceId, text Key, DONKI row id).

USTRUCT(BlueprintType)
struct FTDPhaseRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	ETDPhase Phase = ETDPhase::Arrival;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	int32 SolStart = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	int32 SolEnd = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	FName NameKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Demand")
	int32 DemandAir = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Demand")
	int32 DemandGreenhouse = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Demand")
	int32 DemandShelter = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	ETDEventFamily EventFamily = ETDEventFamily::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	FName OptionalScienceKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	bool bIsBlackout = false;

	/** Cadet band only: this system is held at Good for the phase and needs no cells. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	ETDSystem CadetLockedSystem = ETDSystem::Shelter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	TArray<FName> SourceIds;
};

USTRUCT(BlueprintType)
struct FTDEventRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	ETDEventFamily Family = ETDEventFamily::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	ETDCardLabel Label = ETDCardLabel::FlownMeasured;

	/** Bit N set = the card may appear in phase index N (0 = Arrival ... 5 = Return). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	int32 AllowedPhasesMask = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	TArray<ETDAgeBand> Bands;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Front")
	FName QuestionKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Choice A")
	FName ChoiceAKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Choice A")
	int32 ChoiceADemandAir = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Choice A")
	int32 ChoiceADemandGreenhouse = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Choice A")
	int32 ChoiceADemandShelter = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Choice A")
	ETDChoiceExtra ChoiceAExtra = ETDChoiceExtra::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Choice B")
	FName ChoiceBKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Choice B")
	int32 ChoiceBDemandAir = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Choice B")
	int32 ChoiceBDemandGreenhouse = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Choice B")
	int32 ChoiceBDemandShelter = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Choice B")
	ETDChoiceExtra ChoiceBExtra = ETDChoiceExtra::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Back")
	FName RevealKey;

	/** ISO date of the real event, e.g. 2017-09-10. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Back")
	FString EventDate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Back")
	FString Instrument;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Back")
	FName MeasurementKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Back")
	TArray<FName> SourceIds;

	/** Dust storm: solar output falls for the phase (design DustCells). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	bool bDustStorm = false;
};

USTRUCT(BlueprintType)
struct FTDAgeBandRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Age Band")
	ETDAgeBand Band = ETDAgeBand::Commander;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Age Band")
	int32 Cells = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Age Band")
	int32 Phases = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Age Band")
	int32 SystemsDecided = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Age Band")
	int32 AdviceDelayPhases = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Age Band")
	int32 CardSentences = 2;

	/** Maximum technical terms per game; 0 = no limit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Age Band")
	int32 TermLimit = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Age Band")
	int32 RitualTaps = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Age Band")
	bool bShowDoseDuringPlay = true;
};

USTRUCT(BlueprintType)
struct FTDConstantRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Constant")
	double Value = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Constant")
	FString Unit;

	/** Row name in DT_TD_Sources. Required when bIsDesignerValue is false. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Constant")
	FName SourceId;

	/** True = a game number chosen by the team, labelled as such in the game. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Constant")
	bool bIsDesignerValue = false;

	/** Where in the source the number is (table, page, date of the row). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Constant")
	FString SourceDetail;
};

USTRUCT(BlueprintType)
struct FTDSourceRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Human-readable title (the CSV row name is the SourceId). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Source")
	FString Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Source")
	FString URL;

	/** ISO date the team last opened the source and checked the value; empty = not yet checked. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Source")
	FString RetrievedOn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Source")
	FString Licence;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Source")
	FString UsedFor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Source")
	bool bIsAIGenerated = false;
};

USTRUCT(BlueprintType)
struct FTDTextRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
	FString EN;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
	FString TR;
};

USTRUCT(BlueprintType)
struct FTDDonkiRow : public FTableRowBase
{
	GENERATED_BODY()

	/** FLR, CME or GST. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DONKI")
	FString EventType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DONKI")
	FString ActivityId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DONKI")
	FString TimeUTC;

	/** Flare class (e.g. X8.2), CME speed, or storm Kp maximum. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DONKI")
	FString ClassType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DONKI")
	FString SourceURL;
};

/** Jezero weather for one mission sol: Perseverance MEDA archive (PDS), mapped to mission sols 1-31. Never live. */
USTRUCT(BlueprintType)
struct FTDWeatherRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	int32 MissionSol = 1;

	/** Perseverance sol the values come from. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	int32 MedaSol = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	FString EarthDate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	double AirMinC = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	double AirMaxC = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	double PressureMinPa = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	double PressureMaxPa = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	double PressureMeanPa = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	FName SourceId;
};

// Runtime structs (not DataTable rows)

USTRUCT(BlueprintType)
struct FTDSpaceWeather
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "DONKI")
	ETDDataStatus Status = ETDDataStatus::Archive;

	UPROPERTY(BlueprintReadOnly, Category = "DONKI")
	FDateTime FetchedAtUTC;

	UPROPERTY(BlueprintReadOnly, Category = "DONKI")
	int32 FlareCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "DONKI")
	FString StrongestClass;

	UPROPERTY(BlueprintReadOnly, Category = "DONKI")
	int32 CMECount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "DONKI")
	int32 StormCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "DONKI")
	TArray<FTDDonkiRow> Rows;
};

USTRUCT(BlueprintType)
struct FTDPendingMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mission Control")
	int32 SentPhase = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mission Control")
	int32 ArrivePhase = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mission Control")
	FName TextKey;
};

USTRUCT(BlueprintType)
struct FTDMissionState
{
	GENERATED_BODY()

	/** Four digits, 1000-9999. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 Seed = 1000;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	ETDAgeBand Band = ETDAgeBand::Commander;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 PhaseIndex = 0;

	/** Indexed by ETDSystem; 0 = failed, 1 = Danger, 2 = Low, 3-4 = Good. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	TArray<int32> Levels;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 CellsTotal = 0;

	/** Cells placed this phase, indexed by ETDSystem. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	TArray<int32> Allocation;

	/** Greenhouse level change applied at the start of the next phase. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 PendingGreenhouseDelta = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 StormExposures = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 SolsElapsed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	TArray<FTDPendingMessage> PendingMessages;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	TArray<FName> DrawnEventIds;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	bool bScienceDone = false;

	/** Event drawn for the current phase; None when the phase has no event. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	FName CurrentEventId;

	/** -1 = no choice yet, 0 = choice A, 1 = choice B. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 EventChoice = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	bool bCalledHomeThisPhase = false;

	/** GREENHOUSE reached the harvest level at the start of the last phase. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	bool bHarvest = false;

	/** Lowest system level seen at the end of an event phase (Emergency stamp). */
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 LowestEventLevel = 4;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	ETDMissionResult Result = ETDMissionResult::InProgress;

	/** Spare-part charges left from the design (each makes one equipment failure cheaper). */
	UPROPERTY(BlueprintReadOnly, Category = "Mission|Design")
	int32 RepairChargesLeft = 0;

	/** Radiation relief built on the surface (regolith piled on the shelter by the crew). */
	UPROPERTY(BlueprintReadOnly, Category = "Mission|Ops")
	int32 OpsRadiationRelief = 0;

	/** This phase is a dust-storm phase: solar output is down (design DustCells). */
	UPROPERTY(BlueprintReadOnly, Category = "Mission|Design")
	bool bDustPhase = false;

	/** Event decisions taken and how many kept every system at Low or better without exposure. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission|Score")
	int32 Decisions = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mission|Score")
	int32 GoodDecisions = 0;

	/** Science walks completed on the surface (event choices with science, or walks in first person). */
	UPROPERTY(BlueprintReadOnly, Category = "Mission|Score")
	int32 ScienceWalks = 0;
};

USTRUCT(BlueprintType)
struct FTDMissionReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Report")
	ETDMissionResult Result = ETDMissionResult::InProgress;

	/** Crew home safe and healthy: lift-off reached and no storm exposure. */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	bool bCrew = false;

	/** Base left working: every system at Low (2) or better at lift-off. */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	bool bBase = false;

	/** Emergency handled: no system fell below Low (2) at the end of an event phase. */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	bool bEmergency = false;

	/** Bonus: optional science done or lettuce harvest. */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	bool bBonus = false;

	UPROPERTY(BlueprintReadOnly, Category = "Report")
	bool bMessageStillOnItsWay = false;

	/** Phase index (ETDPhase) in which the mission was lost; -1 if not lost. */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	int32 LostInPhase = -1;
};
