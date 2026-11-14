// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Model/TDMissionRules.h"
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
};
