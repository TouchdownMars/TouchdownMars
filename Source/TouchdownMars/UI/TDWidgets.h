// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
// C++ bases for the WBP_TD_* widgets. The Blueprint child holds the layout (built by Scripts/build_greybox_ui.py with
// VibeUE); the names of the bound widgets below must match the designer names exactly. Colors and sizes come from
// TDTokens. Only UTDHudWidget listens to the mission subsystem; it refreshes its children.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Model/TDTypes.h"
#include "TDWidgets.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UProgressBar;
class UTextBlock;
class UWidget;
class UTDMissionSubsystem;

UCLASS(Abstract)
class TOUCHDOWNMARS_API UTDWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Redraw from the current mission state. Parents call this on their children. */
	virtual void RefreshFromState(const FTDMissionState& State) {}

protected:
	UTDMissionSubsystem* GetMission() const;
	FText Txt(FName Key) const;
	FText BandTxt(FName Key) const;
	static void StyleText(UTextBlock* Text, int32 Size, const FLinearColor& Color);
	static void SetLabel(UTextBlock* Text, const FText& Value, int32 Size, const FLinearColor& Color);
	static void StyleButton(UButton* Button, const FLinearColor& Fill);
	/** Builds a filled or outlined square in Box (greybox cells, segments, dots). */
	UBorder* AddSquare(UHorizontalBox* Box, float Size, const FLinearColor& Fill, const FLinearColor& Outline, float Gap);
};

/** WBP_TD_PowerCell: one large power cell. */
UCLASS(Abstract)
class TOUCHDOWNMARS_API UTDPowerCellWidget : public UTDWidgetBase
{
	GENERATED_BODY()

public:
	void SetUsed(bool bUsed);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> CellBorder;
};

/** WBP_TD_SystemSlot: click the slot to place a cell, the small button to take one back. */
UCLASS(Abstract)
class TOUCHDOWNMARS_API UTDSystemSlotWidget : public UTDWidgetBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	ETDSystem System = ETDSystem::Air;

	virtual void RefreshFromState(const FTDMissionState& State) override;

	/** Opening minute: only the taught slot is active. */
	void SetOpeningBlocked(bool bBlocked) { bOpeningBlocked = bBlocked; }

	/** Ghost-hand hint: a pulsing accent bar above the slot. */
	void SetHint(bool bOn);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SlotButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StateText;

	/** Four-segment level fill. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> SegmentBox;

	/** Placed cells (filled) and still-needed cells (outlined); no bare numbers. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> CellBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RemoveButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> HintBar;

	/** Countdown mark for the delayed GREENHOUSE result and the upcoming-event icon. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MarkText;

private:
	UFUNCTION()
	void HandleSlotClicked();

	UFUNCTION()
	void HandleRemoveClicked();

	void PulseHint();

	bool bOpeningBlocked = false;
	bool bHintBright = false;
	FTimerHandle HintTimer;
};

/** WBP_TD_NextPhaseButton: disabled until every cell is placed (and the event answered). */
UCLASS(Abstract)
class TOUCHDOWNMARS_API UTDNextPhaseButtonWidget : public UTDWidgetBase
{
	GENERATED_BODY()

public:
	virtual void RefreshFromState(const FTDMissionState& State) override;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Label;

private:
	UFUNCTION()
	void HandleClicked();
};

/** WBP_TD_PowerPanel: cells left, three slots centre, Next phase right. */
UCLASS(Abstract)
class TOUCHDOWNMARS_API UTDPowerPanelWidget : public UTDWidgetBase
{
	GENERATED_BODY()

public:
	virtual void RefreshFromState(const FTDMissionState& State) override;

	UTDSystemSlotWidget* GetSlot(ETDSystem System) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars")
	TSubclassOf<UTDPowerCellWidget> PowerCellClass;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> PanelBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> CellsBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTDSystemSlotWidget> SlotAir;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTDSystemSlotWidget> SlotGreenhouse;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTDSystemSlotWidget> SlotShelter;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTDNextPhaseButtonWidget> NextPhase;
};

/** WBP_TD_PhaseStrip: dots for the band's phases, active phase name and sol range. */
UCLASS(Abstract)
class TOUCHDOWNMARS_API UTDPhaseStripWidget : public UTDWidgetBase
{
	GENERATED_BODY()

public:
	virtual void RefreshFromState(const FTDMissionState& State) override;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> DotBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PhaseNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SolText;
};

/** WBP_TD_EventCard: question and two choices, then the flip to "What really happened?". Never closes by itself. */
UCLASS(Abstract)
class TOUCHDOWNMARS_API UTDEventCardWidget : public UTDWidgetBase
{
	GENERATED_BODY()

public:
	void ShowEvent(FName EventId);
	void HideCard();
	/** Starts the flip to the back (any source of the choice: button, keyboard, script). */
	void FlipToBack();
	bool IsShowingBack() const;
	bool IsOpen() const;
	void RefreshTexts();

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UBorder> CardRoot;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> FrontBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> QuestionText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ChoiceAButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ChoiceALabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ChoiceBButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ChoiceBLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> PriorityText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> BackBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> BackTitleText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> RevealText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> FactsText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> MeasureText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SourceText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> CloseButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CloseLabel;

private:
	UFUNCTION() void HandleChoiceA();
	UFUNCTION() void HandleChoiceB();
	UFUNCTION() void HandleClose();
	void Choose(bool bA);
	void StepFlip();
	void ShowSide(bool bBack);

	FName EventId;
	bool bFlipping = false;
	FTimerHandle FlipTimer;
	float FlipElapsed = 0.f;
	bool bSwapped = false;
};

/** WBP_TD_Capcom: Mission Control portrait and one line that stays until the next message. */
UCLASS(Abstract)
class TOUCHDOWNMARS_API UTDCapcomWidget : public UTDWidgetBase
{
	GENERATED_BODY()

public:
	void SetLine(const FText& Line);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UBorder> Portrait;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> PortraitText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> LineText;
};

/** WBP_TD_MessageTrack: the message dot travelling the Earth line, stamped with the phase it was sent in. */
UCLASS(Abstract)
class TOUCHDOWNMARS_API UTDMessageTrackWidget : public UTDWidgetBase
{
	GENERATED_BODY()

public:
	virtual void RefreshFromState(const FTDMissionState& State) override;

protected:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> TrackBar;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> StampText;
};

/** WBP_TD_MissionReport: four stamps, Minimum/Full, patch, seed, last time vs this time. */
UCLASS(Abstract)
class TOUCHDOWNMARS_API UTDMissionReportWidget : public UTDWidgetBase
{
	GENERATED_BODY()

public:
	void ShowReport();

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CrewText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> BaseText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> EmergencyText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> BonusText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ResultText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> PatchText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SeedText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CompareText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> MessageText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> FlyAgainButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> FlyAgainLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> NewMissionButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> NewMissionLabel;

private:
	UFUNCTION() void HandleFlyAgain();
	UFUNCTION() void HandleNewMission();
	void Stamp(UTextBlock* Text, FName Key, bool bOn);
};

/** WBP_TD_BackToLander: the lose screen. No "Game Over"; same-seed retry. */
UCLASS(Abstract)
class TOUCHDOWNMARS_API UTDBackToLanderWidget : public UTDWidgetBase
{
	GENERATED_BODY()

public:
	void ShowLost();

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> LineText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> RetryButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> RetryLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> NewButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> NewLabel;

private:
	UFUNCTION() void HandleRetry();
	UFUNCTION() void HandleNew();
};

/** WBP_TD_SunToday: corner badge (LIVE / ARCHIVE) that opens the DONKI panel. */
UCLASS(Abstract)
class TOUCHDOWNMARS_API UTDSunTodayWidget : public UTDWidgetBase
{
	GENERATED_BODY()

public:
	void ShowWeather(const FTDSpaceWeather& Weather);
	void OpenPanel();
	void ClosePanel();
	bool IsPanelOpen() const;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> BadgeButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> BadgeText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> SunPanel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SunTitleText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SummaryText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> RowsText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> DisclaimerText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SunCloseButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SunCloseLabel;

private:
	UFUNCTION() void HandleBadge();
	UFUNCTION() void HandleClose();

	FTDSpaceWeather Current;
	bool bHasWeather = false;
};

/** WBP_TD_SourcePanel: every number with value, unit, source, URL, retrieved date and NASA vs designer value. */
UCLASS(Abstract)
class TOUCHDOWNMARS_API UTDSourcePanelWidget : public UTDWidgetBase
{
	GENERATED_BODY()

public:
	void Open();
	void Close();

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SourcesTitleText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ListText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SourcesCloseButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SourcesCloseLabel;

private:
	UFUNCTION() void HandleClose();
};

/** WBP_TD_Settings: pause menu and settings (Esc or gear). The game is turn-based, so nothing runs behind it. */
UCLASS(Abstract)
class TOUCHDOWNMARS_API UTDSettingsWidget : public UTDWidgetBase
{
	GENERATED_BODY()

public:
	void Open();
	void Close();
	void RefreshTexts();

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SettingsTitleText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ResumeButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ResumeLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> RestartButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> RestartLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> NewMissionButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> NewMissionLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> BandButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> BandLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> LanguageButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> LanguageLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SoundButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SoundLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> MotionButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> MotionLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ScreenButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ScreenLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> QuitButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> QuitLabel;

private:
	UFUNCTION() void HandleResume();
	UFUNCTION() void HandleRestart();
	UFUNCTION() void HandleNewMission();
	UFUNCTION() void HandleLanguage();
	UFUNCTION() void HandleSound();
	UFUNCTION() void HandleMotion();
	UFUNCTION() void HandleScreen();
	UFUNCTION() void HandleQuit();
};

/** WBP_TD_HUD: root widget; the only widget that listens to the mission delegates. Runs the opening minute. */
UCLASS(Abstract)
class TOUCHDOWNMARS_API UTDHudWidget : public UTDWidgetBase
{
	GENERATED_BODY()

public:
	virtual void RefreshFromState(const FTDMissionState& State) override;

	/** Esc: close the card back, then any open panel, else open the pause menu. */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|UI")
	void HandleBack();

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|UI")
	void OpenSettings();

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|UI")
	void OpenSources();

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|UI")
	void OpenSunPanel();

	/** First person: true = mission console open (panel, card, buttons); false = walking (helmet view). */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|UI")
	void SetConsoleMode(bool bOpen);

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|UI")
	bool IsConsoleMode() const { return bConsoleMode; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTDPhaseStripWidget> PhaseStrip;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTDPowerPanelWidget> PowerPanel;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DebugText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTDEventCardWidget> EventCard;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTDCapcomWidget> Capcom;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTDMessageTrackWidget> MessageTrack;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CallHomeButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> CallHomeLabel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ResultText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DoseText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTDMissionReportWidget> MissionReport;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTDBackToLanderWidget> BackToLander;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTDSunTodayWidget> SunToday;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTDSourcePanelWidget> SourcePanel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTDSettingsWidget> SettingsPanel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> SettingsButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SettingsLabel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> LanguageButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> LanguageLabel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> MuteButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MuteLabel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> SourcesButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SourcesLabel;

	/** Seconds without a purposeful click before the ghost-hand hint (design value 6-8 s). */
	UPROPERTY(EditAnywhere, Category = "Touchdown Mars")
	float IdleHintSeconds = 7.f;

private:
	UFUNCTION() void HandleAllocation(const FTDMissionState& State);
	UFUNCTION() void HandleLevels(const FTDMissionState& State, const TArray<ETDSystem>& Changed);
	UFUNCTION() void HandlePhaseStarted(ETDPhase Phase);
	UFUNCTION() void HandleEventDrawn(FName EventId);
	UFUNCTION() void HandlePhaseResolved(const FText& Cause);
	UFUNCTION() void HandleMessage(const FText& Message);
	UFUNCTION() void HandleEnded(ETDMissionResult Result);
	UFUNCTION() void HandleWeather(const FTDSpaceWeather& Weather);
	UFUNCTION() void HandleSettingsChanged();
	UFUNCTION() void HandleCallHome();
	UFUNCTION() void HandleSettingsButton();
	UFUNCTION() void HandleLanguageButton();
	UFUNCTION() void HandleMuteButton();
	UFUNCTION() void HandleSourcesButton();
	UFUNCTION() void HandleShotStarted(FName ShotId, FName CameraTag, int32 OutcomeState);
	UFUNCTION() void HandleShotEnded(FName ShotId);
	UFUNCTION() void HandleShotFinished(FName EventId);

	/** Loop step 4: the result sentence now, the Outcome Shot, then (OnShotFinished) the card flip. */
	void RequestOutcomeShot(const FTDMissionState& State);
	void ShowReportNow();

	void RefreshNow();
	void RefreshCorner();
	static void ShowPanelOf(UTextBlock* Text, bool bShown);
	void SayCapcom(FName Key);
	void AdvanceOpening(const FTDMissionState& State);
	void RestartIdleTimer();
	void ShowIdleHint();
	void ClearHints();

	/** -1 = no opening, 0 = waiting for the first AIR cell, 1 = second cell, 2 = until all cells placed. */
	int32 OpeningStep = -1;
	bool bConsoleMode = true;
	void ApplyConsoleMode();
	FName ShotRequestedFor;
	bool bAwaitingShot = false;
	bool bPendingReport = false;
	bool bShotDimmed = false;
	ESlateVisibility VisibilityBeforeShot = ESlateVisibility::SelfHitTestInvisible;
	FTimerHandle IdleTimer;
	FTimerHandle OpeningLineTimer;
};
