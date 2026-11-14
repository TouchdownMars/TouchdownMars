// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Model/TDMissionRules.h"
#include "Model/TDOperations.h"
#include "TDMissionSubsystem.generated.h"

class UDataTable;
class UTDDonkiClient;
class UTDSaveGame;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnAllocationChanged, const FTDMissionState&, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnLevelsChanged, const FTDMissionState&, State, const TArray<ETDSystem>&, Changed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnPhaseStarted, ETDPhase, Phase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnEventDrawn, FName, EventId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnEventResolved, FName, EventId, const FText&, CauseSentence);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnPhaseResolved, const FText&, CauseSentence);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnMessageArrived, const FText&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnMissionEnded, ETDMissionResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnSpaceWeatherReady, const FTDSpaceWeather&, Weather);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTDOnSettingsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTDOnDesignChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTDOnDesignLaunched);
DECLARE_MULTICAST_DELEGATE(FTDOnOpsChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FTDOnOpsReport, const TArray<FText>& /*Lines*/);

/** One flown mission of this session, for the post-mission comparison. */
USTRUCT(BlueprintType)
struct FTDFlightRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	FString DesignCode;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	FName ObjectiveId;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	int32 Seed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	FTDMissionScore Score;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	FTDDesignBudget Budget;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	FTDMissionDesign Design;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	int32 ScienceWalks = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	int32 GoodDecisions = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Design")
	int32 Decisions = 0;
};

/** First-flight onboarding (docs/gdd.md "First flight"): kept in the subsystem so it survives the design screen being rebuilt. */
struct FTDFirstFlightState
{
	bool bSkipped = false;          // "Flown before? Skip guidance": no locks, warnings-only guide
	bool bColdOpenSeen = false;     // the Ares-0 cold open was shown
	uint8 Beat = 0;                 // STDDesignRoom::EBeat
	int32 DataGuess = -1;           // 0 all, 1 most, 2 half or less
	int32 StormGuess = -1;          // 0 yes, 1 no
	bool bFissionUnlocked = false;  // unlocked by the storm preview
	bool bStormFailed = false;      // the storm preview failed at least once
	bool bWorkedExample = false;    // "Show me a working design" was used
	double StartSeconds = 0.0;      // for time-to-first-GO
	int32 FocusSlot = -1;           // debrief pick: the slot to pulse on redesign
};

/**
 * Owns the mission state for one world. UI and scene never change state directly: they call these functions and
 * listen to the delegates (technical_design.md §4). All rules live in UTDMissionRules. Also holds the player's
 * settings (UTDSaveGame) and the session's DONKI snapshot.
 */
UCLASS()
class TOUCHDOWNMARS_API UTDMissionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Mission

	/** Seed 0 = pick a new four-digit seed. Returns false if the DataTables are missing. */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Mission")
	bool StartMission(ETDAgeBand Band, int32 Seed = 0);

	/** New seed, band from the settings. */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Mission")
	bool StartNewMission();

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Mission")
	bool PlaceCell(ETDSystem System);

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Mission")
	bool RemoveCell(ETDSystem System);

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Mission")
	bool CanEndPhase() const;

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Mission")
	bool EndPhase();

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Mission")
	bool ChooseEventOption(bool bOptionA);

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Mission")
	bool CallHome();

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Mission")
	bool CanCallHome() const;

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Mission")
	bool RestartSameSeed();

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Mission")
	const FTDMissionState& GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Mission")
	FTDMissionReport GetReport() const;

	// Mission design (Act 1)

	/** Open the design room for an objective. bRedesign keeps the seed and starts from the last flown design. */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Design")
	bool BeginDesign(FName ObjectiveId, bool bRedesign);

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Design")
	bool SetDesignPick(FName OptionId, int32 Count);

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Design")
	void SetRealisticModel(bool bRealistic);

	/** GO (true) or NO-GO (false) from a role in the launch poll. */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Design")
	void SetRoleGo(ETDRole Role, bool bGo);

	/** Replace the design with the objective's recommended (launchable) design. */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Design")
	void UseRecommendedDesign();

	/** Apply the suggested fix of a review line. */
	bool ApplyFix(const FTDDesignIssueLine& Issue);

	/** One player: every role calls GO at once (only when the design fits the hard limits). */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Design")
	void SetAllRolesGo();

	/** Launch the design: apply its effects to the surface rules and start the mission. */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Design")
	bool LaunchDesign();

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Design")
	bool IsDesigning() const { return bDesigning; }

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Design")
	const FTDMissionDesign& GetDesign() const { return Design; }

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Design")
	FTDDesignBudget GetDesignBudget() const;

	/** Budget of the design that flew (fixed at launch). */
	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Design")
	const FTDDesignBudget& GetFlownBudget() const { return FlownBudget; }

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Design")
	bool HasFlownDesign() const { return bHasFlownDesign; }

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Design")
	FTDMissionScore GetScore() const;

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Design")
	FString GetDesignCode() const;

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Design")
	bool IsRoleGoalMet(ETDRole Role) const;

	/** Flights of this session, oldest first. */
	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Design")
	const TArray<FTDFlightRecord>& GetFlights() const { return Flights; }

	/** Jezero weather (MEDA archive) for a mission sol, or null. */
	const FTDWeatherRow* GetWeather(int32 MissionSol) const;

	// ---------------------------------------------------------------- Base operations (gdd.md "Base operations")

	const FTDOpsState& GetOps() const { return Ops; }
	const UDataTable* GetTasksTable() const { return TasksTable; }
	const UDataTable* GetSitesTable() const { return SitesTable; }
	bool HasOps() const { return bHasMission && TasksTable && SitesTable; }

	/** Sols in the current phase (the crew-hour budget scales with them). */
	int32 GetCurrentPhaseSols() const;
	float GetPhaseHours() const;
	float GetPhaseEvaHours() const;
	float GetPlannedHours(int32 CrewIndex, bool bEvaOnly) const;
	ETDTaskBlock GetTaskBlock(FName TaskId, int32 CrewIndex) const;
	bool PlanTask(FName TaskId, int32 CrewIndex, bool bOverrideRest = false);
	float GetCrewPhaseHours(int32 CrewIndex) const;
	void UnplanTask(FName TaskId);
	void AutoPlanTasks();
	void ClearPlan();

	/** Crew display name (crew 0 is the player). */
	FText GetCrewName(int32 CrewIndex) const;

	/** Runs the phase: the crew walk to their sites and work as a time-lapse, then the phase ends. */
	bool RunPhase();

	/** Development: answer the event with the cheaper choice and place the cells (cover the lowest system). */
	void DevFillPhase();
	bool IsRunningPhase() const { return bOpsRunning; }

	/** The player did one of their own tasks in person (pressed E at the site). */
	bool CompleteOwnTask(FName TaskId);

	/** Lines of the last phase's work ("Deniz and Mira unpacked the science payload."). */
	const TArray<FText>& GetOpsReport() const { return OpsReport; }

	/** Level-space position of a site (Z from the ground under it is resolved by the walker). */
	FVector GetSitePosition(FName Site) const;

	FTDOnOpsChanged OnOpsChanged;
	FTDOnOpsReport OnOpsReport;

	FTDFirstFlightState FirstFlight;

	/** True on the first flight of the session unless the player skipped guidance. */
	bool IsFirstFlight() const { return Flights.Num() == 0 && !FirstFlight.bSkipped; }

	/** First-flight start: one solar field, no rocket booked, no instrument (the player causes each red line). */
	void ApplyFirstFlightStart();

	/** One line to Saved/Logs/TD_FirstFlight.csv (beat funnel, predictions, debrief picks; onboarding_logic.md §5). */
	void LogOnboarding(const FString& Event, const FString& Detail = FString()) const;

	const UDataTable* GetPhasesTable() const { return PhasesTable; }
	const UDataTable* GetEventsTable() const { return EventsTable; }
	const UDataTable* GetOptionsTable() const { return OptionsTable; }
	const UDataTable* GetObjectivesTable() const { return ObjectivesTable; }
	const FTDObjectiveRow* GetObjective() const;
	double GetConstant(FName Key) const;
	const FTDConstantRow* GetConstantRow(FName Key) const;

	UPROPERTY(BlueprintAssignable, Category = "Touchdown Mars|Design")
	FTDOnDesignChanged OnDesignChanged;

	UPROPERTY(BlueprintAssignable, Category = "Touchdown Mars|Design")
	FTDOnDesignLaunched OnDesignLaunched;

	/** Report of the previous finished mission this session or from the save (for "last time vs this time"). */
	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Mission")
	bool GetPreviousReport(FTDMissionReport& OutReport) const;

	/** Text key of the play-style patch name, decided at lift-off, never announced before. */
	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Mission")
	FName GetPatchKey() const;

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Mission")
	TArray<int32> GetDemand() const;

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Mission")
	bool IsSystemLocked(ETDSystem System) const;

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Mission")
	bool IsMissionRunning() const { return bHasMission && State.Result == ETDMissionResult::InProgress; }

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Mission")
	bool HasMission() const { return bHasMission; }

	/** Phase indices (ETDPhase) the current band plays, in order. */
	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Mission")
	TArray<int32> GetPlayablePhases() const;

	/** Sol range shown for a phase (Cadet's merged Arrival runs to the end of FirstWalk). */
	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Mission")
	void GetPhaseSols(int32 PhaseIndex, int32& OutSolStart, int32& OutSolEnd) const;

	const FTDPhaseRow* GetPhaseRow(int32 PhaseIndex) const;
	const FTDEventRow* GetCurrentEvent() const;
	const FTDEventRow* GetEvent(FName EventId) const;
	const FTDSourceRow* GetSource(FName SourceId) const;
	const UDataTable* GetConstantsTable() const { return ConstantsTable; }
	const UDataTable* GetSourcesTable() const { return SourcesTable; }

	/** Text keys of the lines on screen, so a language switch can redraw them. */
	UPROPERTY(BlueprintReadWrite, Category = "Touchdown Mars|Text")
	FName LastCapcomKey;

	UPROPERTY(BlueprintReadWrite, Category = "Touchdown Mars|Text")
	FName LastCauseKey;

	/** True once the opening minute has played in this session (restarts skip it). */
	UPROPERTY(BlueprintReadWrite, Category = "Touchdown Mars|Mission")
	bool bOpeningShown = false;

	// Text

	/** Text from DT_TD_Text in the current language; falls back to the key. */
	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Text")
	FText GetText(FName Key) const;

	/** Band variant first (Key_Cadet in Cadet band), then the plain key. */
	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Text")
	FText GetBandText(FName Key) const;

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Text")
	bool HasText(FName Key) const;

	// Settings (saved in slot TD_Settings)

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Settings")
	void SetLanguage(ETDLanguage NewLanguage);

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Settings")
	ETDLanguage GetLanguage() const;

	/** Applies from the next mission. */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Settings")
	void SetPreferredBand(ETDAgeBand NewBand);

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Settings")
	ETDAgeBand GetPreferredBand() const;

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Settings")
	void SetSoundOn(bool bOn);

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Settings")
	bool IsSoundOn() const;

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Settings")
	void SetReduceMotion(bool bOn);

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Settings")
	bool IsReduceMotion() const;

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Settings")
	void SetOutcomeShotsEnabled(bool bOn);

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Settings")
	bool IsOutcomeShotsEnabled() const;

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Settings")
	void SetFullscreen(bool bOn);

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Settings")
	bool IsFullscreen() const;

	// Today's Sun (DONKI)

	/** Starts the one DONKI fetch of the session; later calls only re-broadcast the result. */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|DONKI")
	void RequestSpaceWeather();

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|DONKI")
	bool HasSpaceWeather() const { return bHasSpaceWeather; }

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|DONKI")
	const FTDSpaceWeather& GetSpaceWeather() const { return SpaceWeather; }

	// Sound (design.md §5 tokens; SFX_TD_<Token> in Content/TouchdownMars/Audio)

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Sound")
	void PlaySfx(FName Token);

	// Debug

	/** One line for the L_TD_Test debug text block. */
	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Debug")
	FString GetDebugString() const;

	UPROPERTY(BlueprintAssignable, Category = "Touchdown Mars|Mission")
	FTDOnAllocationChanged OnAllocationChanged;

	UPROPERTY(BlueprintAssignable, Category = "Touchdown Mars|Mission")
	FTDOnLevelsChanged OnLevelsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Touchdown Mars|Mission")
	FTDOnPhaseStarted OnPhaseStarted;

	UPROPERTY(BlueprintAssignable, Category = "Touchdown Mars|Mission")
	FTDOnEventDrawn OnEventDrawn;

	UPROPERTY(BlueprintAssignable, Category = "Touchdown Mars|Mission")
	FTDOnEventResolved OnEventResolved;

	/** Every EndPhase: one sentence with one reason (pillar P4). */
	UPROPERTY(BlueprintAssignable, Category = "Touchdown Mars|Mission")
	FTDOnPhaseResolved OnPhaseResolved;

	UPROPERTY(BlueprintAssignable, Category = "Touchdown Mars|Mission")
	FTDOnMessageArrived OnMessageArrived;

	UPROPERTY(BlueprintAssignable, Category = "Touchdown Mars|Mission")
	FTDOnMissionEnded OnMissionEnded;

	UPROPERTY(BlueprintAssignable, Category = "Touchdown Mars|DONKI")
	FTDOnSpaceWeatherReady OnSpaceWeatherReady;

	UPROPERTY(BlueprintAssignable, Category = "Touchdown Mars|Settings")
	FTDOnSettingsChanged OnSettingsChanged;

private:
	bool LoadTables(FString& OutError);
	void BeginCurrentPhase();
	void DeliverDueMessages();
	void HandleMissionEnded();
	/** Show the base parts tagged DesignShow_<OptionId>_<N> when the design has at least N of that option. */
	void RefreshDesignScene(const FTDMissionDesign& ForDesign) const;
	void SaveSettings();
	int32 GetConstantInt(const TCHAR* Key, int32 Fallback) const;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> PhasesTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> AgeBandsTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> ConstantsTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> EventsTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> TextTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> SourcesTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> DonkiArchiveTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> OptionsTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> WeatherTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> TasksTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> SitesTable;

	FTDOpsState Ops;
	bool bOpsRunning = false;
	int32 OpsCrewRunning = 0;
	TArray<FText> OpsReport;
	FTimerHandle OpsSafetyTimer;
	TSharedPtr<class FTDWalkGrid> WalkGrid;
	void FinishRun();
	void ApplySolarDust();

	/** Shows the parts the crew has deployed (OpsShow_<TaskId> / OpsHide_<TaskId> tags, combined with DesignShow_). */
	void RefreshOpsScene();

	/** Every second: an idle AI crew member joins the player outside (buddy rule) and calls out the sites. */
	void TickEscort();
	FTimerHandle EscortTimer;
	TWeakObjectPtr<class ATDCrewMember> Escort;
	TSet<FName> CalledOut;
	bool bCalledFar = false;
	bool bCalledDust = false;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> ObjectivesTable;

	UPROPERTY(Transient)
	TObjectPtr<UTDSaveGame> Settings;

	UPROPERTY(Transient)
	TObjectPtr<UTDDonkiClient> DonkiClient;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<USoundBase>> SoundCache;

	FTDRulesConfig Config;
	TArray<FTDDeckEntry> Deck;
	FTDMissionState State;
	FTDMissionReport PreviousReport;
	bool bHasPreviousReport = false;
	bool bHasMission = false;
	bool bHasSpaceWeather = false;
	bool bSpaceWeatherRequested = false;
	FTDSpaceWeather SpaceWeather;

	FTDMissionDesign Design;
	FTDDesignBudget FlownBudget;
	FTDMissionDesign FlownDesign;
	TArray<FTDFlightRecord> Flights;
	int32 DesignSeed = 0;
	bool bDesigning = false;
	bool bHasFlownDesign = false;
};
