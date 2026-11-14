// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UTDMissionSubsystem;
class SBorder;

DECLARE_DELEGATE(FTDBaseComputerAction);

/**
 * Base computer (habitat terminal, key B): the crew of four, their hours this phase, the task board and the
 * setup checklist. The player plans who does what; "Run sols" plays the phase as a time-lapse in which the AI
 * crew walk to the sites and work. Built in Slate like the design room; rebuilds on every change.
 */
class TOUCHDOWNMARS_API STDBaseComputer : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STDBaseComputer) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UTDMissionSubsystem>, Mission)
		SLATE_EVENT(FTDBaseComputerAction, OnClose)
		SLATE_EVENT(FTDBaseComputerAction, OnRun)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~STDBaseComputer() override;
	void Rebuild();
	/** Mission Control opens on the tab that needs the player (decision, then power, then crew). */
	void PickTab();
	enum class ETab : uint8 { Power, Crew, Decision, Data };
#if !UE_BUILD_SHIPPING
	/** Dev hook for the screenshot tour (-TDShotTour). */
	void DevSetTab(int32 InTab) { Tab = static_cast<ETab>(FMath::Clamp(InTab, 0, 3)); Rebuild(); }
#endif
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	/** Rebuilds when the mission changes under it (a choice made elsewhere, a phase that ended). */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildCrew();
	TSharedRef<SWidget> BuildBoard();
	TSharedRef<SWidget> BuildSide();
	TSharedRef<SWidget> BuildFooter();
	TSharedRef<SWidget> BuildTabs();
	TSharedRef<SWidget> BuildPower();
	TSharedRef<SWidget> BuildDecision();
	/** Today's Sun (NASA DONKI, LIVE or ARCHIVE) and where every number comes from (Source Panel). */
	TSharedRef<SWidget> BuildData();
	ETab Tab = ETab::Power;
	FString LastSeenState;
	FText Txt(const TCHAR* Key) const;
	FText Txt(FName Key) const;

	TWeakObjectPtr<UTDMissionSubsystem> Mission;
	FTDBaseComputerAction OnClose;
	FTDBaseComputerAction OnRun;
	TSharedPtr<SBorder> Root;
	int32 SelectedCrew = 1;
	FDelegateHandle OpsChangedHandle;
};
