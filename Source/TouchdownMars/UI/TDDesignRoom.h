// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Model/TDMissionDesign.h"

class UTDMissionSubsystem;
class SBorder;

/** What the mission design screen shows. */
enum class ETDDesignScreen : uint8
{
	Objectives, // pick the mission objective (scenario)
	Design,     // the Mission Design Room: role tabs, option cards, live dashboard, review, GO/NO-GO poll
	Transit,    // launch and cruise card after "Launch"
	Debrief     // Act 3: score axes, what broke, comparison, team debrief, redesign
};

DECLARE_DELEGATE(FTDDesignScreenAction);

/**
 * Mission Design Room and Mission Debrief (Slate, built in code). Reads and changes the design only through
 * UTDMissionSubsystem. The screen rebuilds itself after every click; there is no per-frame logic.
 */
class TOUCHDOWNMARS_API STDDesignRoom : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STDDesignRoom) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UTDMissionSubsystem>, Mission)
		/** Launch pressed and the mission started: the owner hides this screen after the transit card. */
		SLATE_EVENT(FTDDesignScreenAction, OnLaunched)
		/** The transit card finished (or was clicked away). */
		SLATE_EVENT(FTDDesignScreenAction, OnTransitDone)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void Show(ETDDesignScreen InScreen);
	ETDDesignScreen GetScreen() const { return Screen; }
	void Rebuild();
#if !UE_BUILD_SHIPPING
	/** Dev hooks for the screenshot tour (-TDShotTour). */
	void DevSetRole(int32 Role) { ActiveRole = static_cast<ETDRole>(FMath::Clamp(Role, 0, 3)); Rebuild(); }
	void DevSetBeat(int32 Beat) { SetBeat(static_cast<EBeat>(Beat)); Rebuild(); }
	void DevStartPoll() { PollStart = FPlatformTime::Seconds(); Rebuild(); }
#endif

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	TSharedRef<SWidget> BuildObjectives();
	TSharedRef<SWidget> BuildDesign();
	TSharedRef<SWidget> BuildTransit();
	TSharedRef<SWidget> BuildDebrief();

	TSharedRef<SWidget> BuildHeader(const FTDDesignBudget& Budget);
	TSharedRef<SWidget> BuildRoleBar(const FTDDesignBudget& Budget);
	TSharedRef<SWidget> BuildSlot(ETDDesignSlot Slot, const FTDDesignBudget& Budget);
	TSharedRef<SWidget> BuildOptionCard(FName OptionId, const FTDDesignOptionRow& Row, const FTDDesignBudget& Budget);
	TSharedRef<SWidget> BuildDashboard(const FTDDesignBudget& Budget);
	TSharedRef<SWidget> BuildIssues(const FTDDesignBudget& Budget);
	TSharedRef<SWidget> BuildForecast();
	TSharedRef<SWidget> BuildPoll(const FTDDesignBudget& Budget);
	/** GO / NO-GO launch checklist: what still blocks LAUNCH, each row with a FIX that takes you to the console. */
	TSharedRef<SWidget> BuildLaunchChecklist(const FTDDesignBudget& Budget);
	TSharedRef<SWidget> BuildMeter(const FText& Label, double Value, double Max, const FText& ValueText, bool bOverIsBad, const FText& Detail,
		FName BarId = NAME_None, bool bHighlight = false);

	FText Txt(const TCHAR* Key) const;
	FText Txt(FName Key) const;
	FText RoleName(ETDRole Role) const;
	FText SlotName(ETDDesignSlot Slot) const;
	FText IssueText(const FTDDesignIssueLine& Issue) const;
	FText StatsLine(const FTDDesignOptionRow& Row) const;
	bool IsNasaValue(FName ConstantKey) const;

	TWeakObjectPtr<UTDMissionSubsystem> Mission;
	FTDDesignScreenAction OnLaunched;
	FTDDesignScreenAction OnTransitDone;
	TSharedPtr<SBorder> Root;
	ETDDesignScreen Screen = ETDDesignScreen::Objectives;
	ETDRole ActiveRole = ETDRole::Planner;

	/** First-flight guide (one step at a time, highlights the thing to do). */
	bool bGuide = true;

	enum class EGuide : uint8 { None, Mass, Power, Instruments, Data, Poll, Launch };
	EGuide ComputeGuide(const FTDDesignBudget& Budget) const;
	TSharedRef<SWidget> BuildGuide(const FTDDesignBudget& Budget);
	TSharedRef<SWidget> FixButton(const FTDDesignIssueLine& Issue, bool bHighlight);
	bool bDataStepSkipped = false;

	/** First-flight script, ordered by cause (science chain, cargo chain, power chain, gate): research note onboarding_logic.md §4. */
	enum class EBeat : uint8 { ColdOpen, Instrument, DataPredict, DataLink, Rockets, Power, StormPredict, StormPreview, Storm, Poll, Launch, Done };
	bool IsFirstFlight() const;
	EBeat GetBeat() const;
	void SetBeat(EBeat NewBeat, const TCHAR* NamedKey = nullptr);
	void AdvanceBeats(const FTDDesignBudget& Budget);
	EGuide HighlightStep(const FTDDesignBudget& Budget) const;
	bool IsLocked(FName OptionId, FText* OutReason = nullptr) const;
	bool WasLockedOnFirstFlight(FName OptionId) const;
	bool HasMassIssue(const FTDDesignBudget& Budget) const;
	TArray<FIntPoint> DustPhaseSols() const;
	TSharedRef<SWidget> BuildColdOpen();
	TSharedRef<SWidget> BuildBeatStrip(const FTDDesignBudget& Budget);
	TSharedRef<SWidget> BuildCausalMap(const TSet<FName>& Lit, const TSet<FName>& Failing, FName Current, int32 Size = 12) const;
	TSet<FName> LitNodes() const;
	TSet<FName> FailingNodes(const FTDDesignBudget& Budget) const;
	FName CurrentNode() const;
	TSharedRef<SWidget> BuildCalendar(bool bHighlight);
	TSharedRef<SWidget> BuildCells(const FTDDesignBudget& Budget, bool bHighlight);
	TSharedRef<SWidget> BuildStormPreview(const FTDDesignBudget& Budget);
	TSharedRef<SWidget> FixChoices(const FTDDesignIssueLine& Issue, bool bHighlight, bool bStorm = false);
	TSharedRef<SWidget> BuildDebriefPick();
	FName RootCause(FText& OutTrace) const;
	TSharedRef<SWidget> PollReports(const FTDDesignBudget& Budget, bool bAnimated);

	double ColdOpenStart = 0.0;
	double StormPreviewStart = -1.0;
	double PollStart = -1.0;
	FName NamedKey;        // FLIGHT's line naming the cause of the beat just finished
	FName DebriefPick;     // decision the player picked in the debrief (self-explanation before the answer)
	int32 DebriefFlight = -1;
	double TransitStart = 0.0;
	bool bTransitDone = false;
};
