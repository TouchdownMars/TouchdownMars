// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "UI/TDWidgets.h"

#include "Actors/TDPlayerController.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Model/TDMissionRules.h"
#include "Systems/TDMissionSubsystem.h"
#include "Systems/TDShotDirector.h"
#include "TimerManager.h"
#include "UI/TDDesignTokens.h"

namespace
{
	void SetShown(UWidget* Widget, bool bShown)
	{
		if (Widget)
		{
			Widget->SetVisibility(bShown ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}

	void SetShownInteractive(UWidget* Widget, bool bShown)
	{
		if (Widget)
		{
			Widget->SetVisibility(bShown ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}

	bool IsShown(const UWidget* Widget)
	{
		return Widget && Widget->GetVisibility() != ESlateVisibility::Collapsed && Widget->GetVisibility() != ESlateVisibility::Hidden;
	}

	FText PhaseName(const UTDMissionSubsystem* Mission, int32 PhaseIndex)
	{
		const FTDPhaseRow* Row = Mission ? Mission->GetPhaseRow(PhaseIndex) : nullptr;
		return Row ? Mission->GetText(Row->NameKey) : FText::GetEmpty();
	}

	int32 StampCount(const FTDMissionReport& Report)
	{
		return (Report.bCrew ? 1 : 0) + (Report.bBase ? 1 : 0) + (Report.bEmergency ? 1 : 0) + (Report.bBonus ? 1 : 0);
	}
}

// UTDWidgetBase

UTDMissionSubsystem* UTDWidgetBase::GetMission() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UTDMissionSubsystem>() : nullptr;
}

FText UTDWidgetBase::Txt(FName Key) const
{
	const UTDMissionSubsystem* Mission = GetMission();
	return Mission ? Mission->GetText(Key) : FText::FromName(Key);
}

FText UTDWidgetBase::BandTxt(FName Key) const
{
	const UTDMissionSubsystem* Mission = GetMission();
	return Mission ? Mission->GetBandText(Key) : FText::FromName(Key);
}

void UTDWidgetBase::StyleText(UTextBlock* Text, int32 Size, const FLinearColor& Color)
{
	if (!Text)
	{
		return;
	}
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = Size;
	Text->SetFont(Font);
	Text->SetColorAndOpacity(FSlateColor(Color));
}

void UTDWidgetBase::SetLabel(UTextBlock* Text, const FText& Value, int32 Size, const FLinearColor& Color)
{
	if (Text)
	{
		Text->SetText(Value);
		StyleText(Text, Size, Color);
	}
}

void UTDWidgetBase::StyleButton(UButton* Button, const FLinearColor& Fill)
{
	if (Button)
	{
		Button->SetBackgroundColor(Fill);
	}
}

UBorder* UTDWidgetBase::AddSquare(UHorizontalBox* Box, float Size, const FLinearColor& Fill, const FLinearColor& Outline, float Gap)
{
	UBorder* OuterBorder = WidgetTree->ConstructWidget<UBorder>();
	OuterBorder->SetBrushColor(Outline);
	OuterBorder->SetPadding(FMargin(3.f));
	UBorder* Inner = WidgetTree->ConstructWidget<UBorder>();
	Inner->SetBrushColor(Fill);
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SizeBox->SetWidthOverride(Size - 6.f);
	SizeBox->SetHeightOverride(Size - 6.f);
	Inner->SetContent(SizeBox);
	OuterBorder->SetContent(Inner);
	if (UHorizontalBoxSlot* BoxSlot = Box->AddChildToHorizontalBox(OuterBorder))
	{
		BoxSlot->SetPadding(FMargin(0.f, 0.f, Gap, 0.f));
		BoxSlot->SetVerticalAlignment(VAlign_Center);
	}
	return OuterBorder;
}

// UTDPowerCellWidget

void UTDPowerCellWidget::SetUsed(bool bUsed)
{
	if (CellBorder)
	{
		CellBorder->SetBrushColor(bUsed ? TDTokens::SemDisabled() : TDTokens::CoreAccent());
	}
}

// UTDSystemSlotWidget

void UTDSystemSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SlotButton->OnClicked.AddDynamic(this, &UTDSystemSlotWidget::HandleSlotClicked);
	RemoveButton->OnClicked.AddDynamic(this, &UTDSystemSlotWidget::HandleRemoveClicked);
	SetHint(false);
}

void UTDSystemSlotWidget::HandleSlotClicked()
{
	if (UTDMissionSubsystem* Mission = GetMission(); Mission && !bOpeningBlocked)
	{
		Mission->PlaceCell(System);
	}
}

void UTDSystemSlotWidget::HandleRemoveClicked()
{
	if (UTDMissionSubsystem* Mission = GetMission())
	{
		Mission->RemoveCell(System);
	}
}

void UTDSystemSlotWidget::SetHint(bool bOn)
{
	if (!HintBar)
	{
		return;
	}
	HintBar->SetBrushColor(TDTokens::CoreAccent());
	SetShown(HintBar, bOn);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HintTimer);
		if (bOn)
		{
			// Motion_CellPulse: glow 0.6 → 1.0, 900 ms loop.
			World->GetTimerManager().SetTimer(HintTimer, FTimerDelegate::CreateUObject(this, &UTDSystemSlotWidget::PulseHint), 0.45f, true);
		}
	}
}

void UTDSystemSlotWidget::PulseHint()
{
	bHintBright = !bHintBright;
	if (HintBar)
	{
		HintBar->SetRenderOpacity(bHintBright ? 1.f : 0.6f);
	}
}

void UTDSystemSlotWidget::RefreshFromState(const FTDMissionState& State)
{
	const UTDMissionSubsystem* Mission = GetMission();
	const int32 Index = static_cast<int32>(System);
	if (!Mission || !State.Levels.IsValidIndex(Index))
	{
		return;
	}
	const int32 Level = State.Levels[Index];
	const bool bLocked = Mission->IsSystemLocked(System);
	const FLinearColor SystemColor = TDTokens::System(System);

	SetLabel(NameText, Txt(TDTokens::SystemTextKey(System)), TDTokens::SizeBody, SystemColor);
	SetLabel(StateText, bLocked ? Txt(TEXT("UI_LockedGood")) : Txt(FName(FString::Printf(TEXT("Level_%d"), Level))),
		TDTokens::SizeCaption, bLocked ? TDTokens::CoreText() : TDTokens::LevelColor(System, Level));

	SegmentBox->ClearChildren();
	for (int32 Segment = 0; Segment < 4; ++Segment)
	{
		const bool bFilled = Segment < Level;
		AddSquare(SegmentBox, 28.f, bFilled ? TDTokens::LevelColor(System, Level) : TDTokens::CorePrimary(), TDTokens::CoreSecondary(), 4.f);
	}

	CellBox->ClearChildren();
	const TArray<int32> Demand = Mission->GetDemand();
	const int32 Placed = State.Allocation.IsValidIndex(Index) ? State.Allocation[Index] : 0;
	const int32 Needed = Demand.IsValidIndex(Index) ? Demand[Index] : 0;
	for (int32 Cell = 0; Cell < FMath::Max(Placed, Needed); ++Cell)
	{
		const bool bPlaced = Cell < Placed;
		AddSquare(CellBox, 40.f, bPlaced ? TDTokens::CoreAccent() : TDTokens::CorePrimary(), bPlaced ? TDTokens::CoreAccent() : TDTokens::CoreText(), 6.f);
	}

	// Countdown mark: GREENHOUSE result arrives next phase (plants grow slowly).
	if (MarkText)
	{
		const bool bPending = System == ETDSystem::Greenhouse && Placed > 0 && Placed != Needed && !bLocked;
		SetLabel(MarkText, bPending ? Txt(TEXT("UI_GrowsNextPhase")) : FText::GetEmpty(), TDTokens::SizeCaption, TDTokens::CoreText());
	}

	const bool bUsable = !bLocked && !bOpeningBlocked && Mission->IsMissionRunning();
	SlotButton->SetIsEnabled(bUsable);
	StyleButton(SlotButton, bUsable ? SystemColor : TDTokens::SemDisabled());
	RemoveButton->SetIsEnabled(Placed > 0 && Mission->IsMissionRunning());
}

// UTDNextPhaseButtonWidget

void UTDNextPhaseButtonWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Button->OnClicked.AddDynamic(this, &UTDNextPhaseButtonWidget::HandleClicked);
}

void UTDNextPhaseButtonWidget::HandleClicked()
{
	if (ATDPlayerController* PC = Cast<ATDPlayerController>(GetOwningPlayer()))
	{
		PC->RunPhase();
	}
	else if (UTDMissionSubsystem* Mission = GetMission())
	{
		Mission->RunPhase();
	}
}

void UTDNextPhaseButtonWidget::RefreshFromState(const FTDMissionState& State)
{
	const UTDMissionSubsystem* Mission = GetMission();
	const bool bReady = Mission && Mission->CanEndPhase() && !Mission->IsRunningPhase();
	Button->SetIsEnabled(bReady);
	StyleButton(Button, bReady ? TDTokens::CoreAccent() : TDTokens::SemDisabled());
	SetLabel(Label, Txt(TEXT("UI_NextPhase")), TDTokens::SizeBody, bReady ? TDTokens::CorePrimary() : TDTokens::CoreText());
}

// UTDPowerPanelWidget

void UTDPowerPanelWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SlotAir->System = ETDSystem::Air;
	SlotGreenhouse->System = ETDSystem::Greenhouse;
	SlotShelter->System = ETDSystem::Shelter;
	if (PanelBorder)
	{
		PanelBorder->SetBrushColor(TDTokens::CorePrimary());
	}
}

UTDSystemSlotWidget* UTDPowerPanelWidget::GetSlot(ETDSystem System) const
{
	switch (System)
	{
	case ETDSystem::Air: return SlotAir;
	case ETDSystem::Greenhouse: return SlotGreenhouse;
	default: return SlotShelter;
	}
}

void UTDPowerPanelWidget::RefreshFromState(const FTDMissionState& State)
{
	CellsBox->ClearChildren();
	const int32 Placed = UTDMissionRules::GetCellsPlaced(State);
	for (int32 Cell = 0; Cell < State.CellsTotal; ++Cell)
	{
		const bool bUsed = Cell < Placed;
		if (PowerCellClass)
		{
			UTDPowerCellWidget* CellWidget = CreateWidget<UTDPowerCellWidget>(this, PowerCellClass);
			if (UHorizontalBoxSlot* BoxSlot = CellsBox->AddChildToHorizontalBox(CellWidget))
			{
				BoxSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
				BoxSlot->SetVerticalAlignment(VAlign_Center);
			}
			CellWidget->SetUsed(bUsed);
		}
		else
		{
			AddSquare(CellsBox, 64.f, bUsed ? TDTokens::SemDisabled() : TDTokens::CoreAccent(), TDTokens::CoreSecondary(), 8.f);
		}
	}
	for (UTDSystemSlotWidget* SlotWidget : { SlotAir.Get(), SlotGreenhouse.Get(), SlotShelter.Get() })
	{
		SlotWidget->RefreshFromState(State);
	}
	NextPhase->RefreshFromState(State);
}

// UTDPhaseStripWidget

void UTDPhaseStripWidget::RefreshFromState(const FTDMissionState& State)
{
	const UTDMissionSubsystem* Mission = GetMission();
	if (!Mission)
	{
		return;
	}
	const TArray<int32> Phases = Mission->GetPlayablePhases();
	const int32 Current = Phases.IndexOfByKey(State.PhaseIndex);
	DotBox->ClearChildren();
	for (int32 Position = 0; Position < Phases.Num(); ++Position)
	{
		const bool bPast = Position < Current;
		const bool bNow = Position == Current;
		AddSquare(DotBox, bNow ? 36.f : 24.f, (bPast || bNow) ? TDTokens::CoreAccent() : TDTokens::CorePrimary(), bNow ? TDTokens::CoreText() : TDTokens::CoreAccent(), 10.f);
	}

	SetLabel(PhaseNameText, PhaseName(Mission, State.PhaseIndex), TDTokens::SizeT2, TDTokens::CoreText());
	int32 SolStart = 0;
	int32 SolEnd = 0;
	Mission->GetPhaseSols(State.PhaseIndex, SolStart, SolEnd);
	SetLabel(SolText, FText::Format(Txt(TEXT("Phase_SolRange")), FText::AsNumber(SolStart), FText::AsNumber(SolEnd)), TDTokens::SizeCaption, TDTokens::CoreText());
}

// UTDEventCardWidget

void UTDEventCardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ChoiceAButton->OnClicked.AddDynamic(this, &UTDEventCardWidget::HandleChoiceA);
	ChoiceBButton->OnClicked.AddDynamic(this, &UTDEventCardWidget::HandleChoiceB);
	CloseButton->OnClicked.AddDynamic(this, &UTDEventCardWidget::HandleClose);
	CardRoot->SetBrushColor(TDTokens::CorePrimary());
	SetVisibility(ESlateVisibility::Collapsed);
}

void UTDEventCardWidget::ShowEvent(FName InEventId)
{
	EventId = InEventId;
	CardRoot->SetRenderScale(FVector2D(1.f, 1.f));
	ShowSide(false);
	RefreshTexts();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UTDEventCardWidget::HideCard()
{
	bFlipping = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FlipTimer);
	}
	SetVisibility(ESlateVisibility::Collapsed);
}

bool UTDEventCardWidget::IsOpen() const
{
	return GetVisibility() != ESlateVisibility::Collapsed;
}

bool UTDEventCardWidget::IsShowingBack() const
{
	return IsOpen() && IsShown(BackBox);
}

void UTDEventCardWidget::RefreshTexts()
{
	const UTDMissionSubsystem* Mission = GetMission();
	const FTDEventRow* Row = Mission ? Mission->GetEvent(EventId) : nullptr;
	if (!Row)
	{
		return;
	}
	SetLabel(QuestionText, BandTxt(Row->QuestionKey), TDTokens::SizeBody, TDTokens::CoreText());
	SetLabel(ChoiceALabel, Txt(Row->ChoiceAKey), TDTokens::SizeBody, TDTokens::CorePrimary());
	SetLabel(ChoiceBLabel, Txt(Row->ChoiceBKey), TDTokens::SizeBody, TDTokens::CorePrimary());
	StyleButton(ChoiceAButton, TDTokens::CoreAccent());
	StyleButton(ChoiceBButton, TDTokens::CoreAccent());
	SetLabel(PriorityText, Txt(TEXT("UI_PriorityRule")), TDTokens::SizeCaption, TDTokens::CoreText());

	SetLabel(BackTitleText, Txt(TEXT("UI_WhatReallyHappened")), TDTokens::SizeBody, TDTokens::CoreAccent());
	SetLabel(RevealText, BandTxt(Row->RevealKey), TDTokens::SizeBody, TDTokens::CoreText());
	const FText Label = Txt(FName(TEXT("Label_") + StaticEnum<ETDCardLabel>()->GetNameStringByValue(static_cast<int64>(Row->Label))));
	SetLabel(FactsText, FText::Format(FText::FromString(TEXT("{0} · {1} · {2}")), FText::FromString(Row->EventDate), FText::FromString(Row->Instrument), Label),
		TDTokens::SizeCaption, TDTokens::CoreText());
	SetLabel(MeasureText, Txt(Row->MeasurementKey), TDTokens::SizeCaption, TDTokens::CoreText());

	TArray<FString> Sources;
	for (const FName& SourceId : Row->SourceIds)
	{
		const FTDSourceRow* Source = Mission->GetSource(SourceId);
		Sources.Add(Source ? Source->Title : SourceId.ToString());
	}
	SetLabel(SourceText, FText::Format(Txt(TEXT("UI_SourceLine")), FText::FromString(FString::Join(Sources, TEXT("; ")))), TDTokens::SizeCaption, TDTokens::CoreText());
	SetLabel(CloseLabel, Txt(TEXT("UI_Close")), TDTokens::SizeBody, TDTokens::CorePrimary());
	StyleButton(CloseButton, TDTokens::CoreAccent());
}

void UTDEventCardWidget::HandleChoiceA() { Choose(true); }
void UTDEventCardWidget::HandleChoiceB() { Choose(false); }

void UTDEventCardWidget::Choose(bool bA)
{
	// The HUD flips the card when the state shows a choice (so keyboard or script choices flip it too).
	if (UTDMissionSubsystem* Mission = GetMission())
	{
		Mission->ChooseEventOption(bA);
	}
}

void UTDEventCardWidget::FlipToBack()
{
	const UTDMissionSubsystem* Mission = GetMission();
	if (bFlipping || IsShowingBack())
	{
		return;
	}
	if (!Mission || Mission->IsReduceMotion())
	{
		ShowSide(true);
		return;
	}
	// Motion_CardFlip: 450 ms; scale X to 0, swap sides, back to 1.
	bFlipping = true;
	FlipElapsed = 0.f;
	bSwapped = false;
	GetWorld()->GetTimerManager().SetTimer(FlipTimer, FTimerDelegate::CreateUObject(this, &UTDEventCardWidget::StepFlip), 1.f / 60.f, true);
}

void UTDEventCardWidget::StepFlip()
{
	constexpr float Duration = 0.45f;
	FlipElapsed += 1.f / 60.f;
	const float Alpha = FMath::Clamp(FlipElapsed / Duration, 0.f, 1.f);
	if (Alpha >= 0.5f && !bSwapped)
	{
		bSwapped = true;
		ShowSide(true);
	}
	CardRoot->SetRenderScale(FVector2D(FMath::Max(0.02f, FMath::Abs(1.f - 2.f * Alpha)), 1.f));
	if (Alpha >= 1.f)
	{
		GetWorld()->GetTimerManager().ClearTimer(FlipTimer);
		CardRoot->SetRenderScale(FVector2D(1.f, 1.f));
		bFlipping = false;
	}
}

void UTDEventCardWidget::ShowSide(bool bBack)
{
	SetShown(FrontBox, !bBack);
	SetShown(BackBox, bBack);
}

void UTDEventCardWidget::HandleClose()
{
	if (UTDMissionSubsystem* Mission = GetMission()) { Mission->PlaySfx(TEXT("UI_Back")); }
	HideCard();
}

// UTDCapcomWidget

void UTDCapcomWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Portrait->SetBrushColor(TDTokens::Hex(TEXT("C9D3E0")));
	SetLabel(PortraitText, FText::FromString(TEXT("CAPCOM")), TDTokens::SizeCaption, TDTokens::CorePrimary());
}

void UTDCapcomWidget::SetLine(const FText& Line)
{
	SetLabel(LineText, Line, TDTokens::SizeBody, TDTokens::CoreText());
}

// UTDMessageTrackWidget

void UTDMessageTrackWidget::RefreshFromState(const FTDMissionState& State)
{
	const UTDMissionSubsystem* Mission = GetMission();
	if (!Mission)
	{
		return;
	}
	TrackBar->SetFillColorAndOpacity(TDTokens::CoreAccent());
	TrackBar->SetVisibility(State.PendingMessages.Num() > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	if (State.PendingMessages.Num() == 0)
	{
		TrackBar->SetPercent(0.f);
		SetLabel(StampText, FText::GetEmpty(), TDTokens::SizeCaption, TDTokens::CoreText());
		return;
	}
	const FTDPendingMessage& Message = State.PendingMessages.Last();
	const TArray<int32> Phases = Mission->GetPlayablePhases();
	const int32 Sent = Phases.IndexOfByKey(Message.SentPhase);
	const int32 Arrive = Phases.Contains(Message.ArrivePhase) ? Phases.IndexOfByKey(Message.ArrivePhase) : Phases.Num();
	const int32 Now = Phases.IndexOfByKey(State.PhaseIndex);
	const float Percent = Arrive > Sent ? FMath::Clamp(float(Now - Sent + 0.5f) / float(Arrive - Sent + 0.5f), 0.1f, 0.95f) : 1.f;
	TrackBar->SetPercent(Percent);
	SetLabel(StampText, FText::Format(Txt(TEXT("UI_SentIn")), PhaseName(Mission, Message.SentPhase)), TDTokens::SizeCaption, TDTokens::CoreText());
}

// UTDMissionReportWidget

void UTDMissionReportWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	FlyAgainButton->OnClicked.AddDynamic(this, &UTDMissionReportWidget::HandleFlyAgain);
	NewMissionButton->OnClicked.AddDynamic(this, &UTDMissionReportWidget::HandleNewMission);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UTDMissionReportWidget::Stamp(UTextBlock* Text, FName Key, bool bOn)
{
	SetLabel(Text, FText::Format(FText::FromString(bOn ? TEXT("[+] {0}") : TEXT("[ ] {0}")), Txt(Key)), TDTokens::SizeBody,
		bOn ? TDTokens::SemSuccess() : TDTokens::CoreText());
}

void UTDMissionReportWidget::ShowReport()
{
	const UTDMissionSubsystem* Mission = GetMission();
	if (!Mission)
	{
		return;
	}
	const FTDMissionReport Report = Mission->GetReport();
	SetLabel(TitleText, Txt(TEXT("Report_Title")), TDTokens::SizeT1, TDTokens::CoreText());
	Stamp(CrewText, TEXT("Report_Crew"), Report.bCrew);
	Stamp(BaseText, TEXT("Report_Base"), Report.bBase);
	Stamp(EmergencyText, TEXT("Report_Emergency"), Report.bEmergency);
	Stamp(BonusText, TEXT("Report_Bonus"), Report.bBonus);
	SetLabel(ResultText, Txt(Report.Result == ETDMissionResult::Full ? TEXT("Report_Full") : TEXT("Report_Minimum")), TDTokens::SizeT2, TDTokens::CoreAccent());
	SetLabel(PatchText, FText::Format(Txt(TEXT("Report_Patch")), Txt(Mission->GetPatchKey())), TDTokens::SizeBody, TDTokens::CoreText());
	SetLabel(SeedText, FText::Format(Txt(TEXT("Report_Seed")), FText::AsNumber(Mission->GetState().Seed, &FNumberFormattingOptions::DefaultNoGrouping())), TDTokens::SizeCaption, TDTokens::CoreText());
	FTDMissionReport Previous;
	const bool bHasPrevious = Mission->GetPreviousReport(Previous);
	SetLabel(CompareText, bHasPrevious ? FText::Format(Txt(TEXT("Report_LastTime")), FText::AsNumber(StampCount(Previous)), FText::AsNumber(StampCount(Report))) : FText::GetEmpty(),
		TDTokens::SizeCaption, TDTokens::CoreText());
	SetLabel(MessageText, Report.bMessageStillOnItsWay ? Txt(TEXT("UI_MessageOnItsWay")) : FText::GetEmpty(), TDTokens::SizeCaption, TDTokens::CoreText());
	SetLabel(FlyAgainLabel, Txt(TEXT("UI_FlyAgain")), TDTokens::SizeBody, TDTokens::CorePrimary());
	SetLabel(NewMissionLabel, Txt(TEXT("UI_NewMission")), TDTokens::SizeBody, TDTokens::CorePrimary());
	StyleButton(FlyAgainButton, TDTokens::CoreAccent());
	StyleButton(NewMissionButton, TDTokens::CoreAccent());
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UTDMissionReportWidget::HandleFlyAgain()
{
	SetVisibility(ESlateVisibility::Collapsed);
	if (UTDMissionSubsystem* Mission = GetMission())
	{
		Mission->RestartSameSeed();
	}
}

void UTDMissionReportWidget::HandleNewMission()
{
	SetVisibility(ESlateVisibility::Collapsed);
	if (UTDMissionSubsystem* Mission = GetMission())
	{
		Mission->StartNewMission();
	}
}

// UTDBackToLanderWidget

void UTDBackToLanderWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	RetryButton->OnClicked.AddDynamic(this, &UTDBackToLanderWidget::HandleRetry);
	NewButton->OnClicked.AddDynamic(this, &UTDBackToLanderWidget::HandleNew);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UTDBackToLanderWidget::ShowLost()
{
	const UTDMissionSubsystem* Mission = GetMission();
	if (!Mission)
	{
		return;
	}
	const FTDMissionReport Report = Mission->GetReport();
	SetLabel(LineText, FText::Format(Txt(TEXT("Capcom_LostAir")), PhaseName(Mission, Report.LostInPhase)), TDTokens::SizeBody, TDTokens::CoreText());
	SetLabel(RetryLabel, Txt(TEXT("UI_ChangeOneThing")), TDTokens::SizeBody, TDTokens::CorePrimary());
	SetLabel(NewLabel, Txt(TEXT("UI_NewMission")), TDTokens::SizeBody, TDTokens::CorePrimary());
	StyleButton(RetryButton, TDTokens::CoreAccent());
	StyleButton(NewButton, TDTokens::CoreAccent());
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UTDBackToLanderWidget::HandleRetry()
{
	SetVisibility(ESlateVisibility::Collapsed);
	if (UTDMissionSubsystem* Mission = GetMission())
	{
		Mission->RestartSameSeed();
	}
}

void UTDBackToLanderWidget::HandleNew()
{
	SetVisibility(ESlateVisibility::Collapsed);
	if (UTDMissionSubsystem* Mission = GetMission())
	{
		Mission->StartNewMission();
	}
}

// UTDSunTodayWidget

void UTDSunTodayWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BadgeButton->OnClicked.AddDynamic(this, &UTDSunTodayWidget::HandleBadge);
	SunCloseButton->OnClicked.AddDynamic(this, &UTDSunTodayWidget::HandleClose);
	SetShown(SunPanel, false);
	SetLabel(BadgeText, Txt(TEXT("Sun_Badge")), TDTokens::SizeCaption, TDTokens::CorePrimary());
	StyleButton(BadgeButton, TDTokens::CoreAccent());
}

void UTDSunTodayWidget::ShowWeather(const FTDSpaceWeather& Weather)
{
	Current = Weather;
	bHasWeather = true;
	const bool bLive = Weather.Status == ETDDataStatus::Live;
	// Static data is never shown as live: LIVE only with this session's fetch time.
	SetLabel(BadgeText, FText::Format(FText::FromString(TEXT("{0}  {1}")), Txt(TEXT("Sun_Badge")), Txt(bLive ? TEXT("UI_Live") : TEXT("UI_Archive"))),
		TDTokens::SizeCaption, TDTokens::CorePrimary());
	SetLabel(SunTitleText, Txt(TEXT("Sun_Title")), TDTokens::SizeT2, TDTokens::CoreText());
	SetLabel(StatusText, bLive ? FText::Format(Txt(TEXT("Sun_LiveStatus")), FText::FromString(Weather.FetchedAtUTC.ToString(TEXT("%Y-%m-%d %H:%M")))) : Txt(TEXT("Sun_ArchiveStatus")),
		TDTokens::SizeBody, bLive ? TDTokens::SemSuccess() : TDTokens::Hex(TEXT("C9D3E0")));
	SetLabel(SummaryText, FText::Format(Txt(bLive ? TEXT("Sun_SummaryLive") : TEXT("Sun_SummaryArchive")),
		FText::AsNumber(Weather.FlareCount), FText::FromString(Weather.StrongestClass.IsEmpty() ? TEXT("-") : Weather.StrongestClass),
		FText::AsNumber(Weather.CMECount), FText::AsNumber(Weather.StormCount)), TDTokens::SizeBody, TDTokens::CoreText());
	TArray<FString> Lines;
	for (const FTDDonkiRow& Row : Weather.Rows)
	{
		if (Row.EventType != TEXT("CME") && Lines.Num() < 6)
		{
			Lines.Add(FString::Printf(TEXT("%s  %s  %s"), *Row.EventType, *Row.TimeUTC, *Row.ClassType));
		}
	}
	SetLabel(RowsText, FText::FromString(FString::Join(Lines, TEXT("\n"))), TDTokens::SizeCaption, TDTokens::CoreText());
	SetLabel(DisclaimerText, Txt(TEXT("UI_DonkiDisclaimer")), TDTokens::SizeCaption, TDTokens::CoreText());
	SetLabel(SunCloseLabel, Txt(TEXT("UI_Close")), TDTokens::SizeBody, TDTokens::CorePrimary());
	StyleButton(SunCloseButton, TDTokens::CoreAccent());
}

void UTDSunTodayWidget::HandleBadge()
{
	SetShown(SunPanel, !IsShown(SunPanel));
}

void UTDSunTodayWidget::OpenPanel()
{
	SetShown(SunPanel, true);
}

void UTDSunTodayWidget::HandleClose()
{
	ClosePanel();
}

void UTDSunTodayWidget::ClosePanel()
{
	SetShown(SunPanel, false);
}

bool UTDSunTodayWidget::IsPanelOpen() const
{
	return IsShown(SunPanel);
}

// UTDSourcePanelWidget

void UTDSourcePanelWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SourcesCloseButton->OnClicked.AddDynamic(this, &UTDSourcePanelWidget::HandleClose);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UTDSourcePanelWidget::Open()
{
	const UTDMissionSubsystem* Mission = GetMission();
	const UDataTable* Constants = Mission ? Mission->GetConstantsTable() : nullptr;
	if (!Constants)
	{
		return;
	}
	TArray<FString> Lines;
	Lines.Add(Txt(TEXT("UI_NotEndorsed")).ToString());
	Lines.Add(Txt(TEXT("UI_TimeCompression")).ToString());
	Lines.Add(FString());
	for (const TPair<FName, uint8*>& Pair : Constants->GetRowMap())
	{
		const FTDConstantRow& Row = *reinterpret_cast<const FTDConstantRow*>(Pair.Value);
		FString Line = FString::Printf(TEXT("%s: %s %s"), *Pair.Key.ToString(), *FString::SanitizeFloat(Row.Value, 0), *Row.Unit);
		if (Row.bIsDesignerValue)
		{
			Line += TEXT("  -  ") + Txt(TEXT("UI_DesignerValue")).ToString();
		}
		else if (const FTDSourceRow* Source = Mission->GetSource(Row.SourceId))
		{
			Line += FString::Printf(TEXT("  -  %s (%s)  %s  [%s, %s]"), *Source->Title, *Row.SourceDetail, *Source->URL,
				*Txt(TEXT("UI_NasaMeasurement")).ToString(), *FText::Format(Txt(TEXT("UI_Retrieved")), FText::FromString(Source->RetrievedOn)).ToString());
		}
		Lines.Add(Line);
	}
	SetLabel(SourcesTitleText, Txt(TEXT("UI_WhereFrom")), TDTokens::SizeT2, TDTokens::CoreText());
	SetLabel(ListText, FText::FromString(FString::Join(Lines, TEXT("\n"))), TDTokens::SizeCaption, TDTokens::CoreText());
	SetLabel(SourcesCloseLabel, Txt(TEXT("UI_Close")), TDTokens::SizeBody, TDTokens::CorePrimary());
	StyleButton(SourcesCloseButton, TDTokens::CoreAccent());
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UTDSourcePanelWidget::Close()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UTDSourcePanelWidget::HandleClose()
{
	Close();
}

// UTDSettingsWidget

void UTDSettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ResumeButton->OnClicked.AddDynamic(this, &UTDSettingsWidget::HandleResume);
	RestartButton->OnClicked.AddDynamic(this, &UTDSettingsWidget::HandleRestart);
	NewMissionButton->OnClicked.AddDynamic(this, &UTDSettingsWidget::HandleNewMission);
	BandButton->OnClicked.AddDynamic(this, &UTDSettingsWidget::HandleBand);
	LanguageButton->OnClicked.AddDynamic(this, &UTDSettingsWidget::HandleLanguage);
	SoundButton->OnClicked.AddDynamic(this, &UTDSettingsWidget::HandleSound);
	MotionButton->OnClicked.AddDynamic(this, &UTDSettingsWidget::HandleMotion);
	ScreenButton->OnClicked.AddDynamic(this, &UTDSettingsWidget::HandleScreen);
	QuitButton->OnClicked.AddDynamic(this, &UTDSettingsWidget::HandleQuit);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UTDSettingsWidget::Open()
{
	RefreshTexts();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UTDSettingsWidget::Close()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UTDSettingsWidget::RefreshTexts()
{
	const UTDMissionSubsystem* Mission = GetMission();
	if (!Mission)
	{
		return;
	}
	auto Button = [this](UButton* Target, UTextBlock* Label, const FText& Value)
	{
		StyleButton(Target, TDTokens::CoreSecondary());
		SetLabel(Label, Value, TDTokens::SizeBody, TDTokens::CoreText());
	};
	auto OnOff = [this](FName Key, bool bOn) { return FText::Format(FText::FromString(TEXT("{0}: {1}")), Txt(Key), Txt(bOn ? TEXT("UI_On") : TEXT("UI_Off"))); };
	SetLabel(SettingsTitleText, Txt(TEXT("UI_Settings")), TDTokens::SizeT2, TDTokens::CoreText());
	Button(ResumeButton, ResumeLabel, Txt(TEXT("UI_Resume")));
	Button(RestartButton, RestartLabel, Txt(TEXT("UI_RestartSame")));
	Button(NewMissionButton, NewMissionLabel, Txt(TEXT("UI_NewMission")));
	Button(BandButton, BandLabel, FText::Format(FText::FromString(TEXT("{0}: {1}")), Txt(TEXT("UI_AgeBand")),
		Txt(Mission->GetPreferredBand() == ETDAgeBand::Cadet ? TEXT("Band_Cadet") : TEXT("Band_Commander"))));
	Button(LanguageButton, LanguageLabel, FText::FromString(Mission->GetLanguage() == ETDLanguage::EN ? TEXT("Language: English") : TEXT("Dil: Türkçe")));
	Button(SoundButton, SoundLabel, OnOff(TEXT("UI_Sound"), Mission->IsSoundOn()));
	Button(MotionButton, MotionLabel, OnOff(TEXT("UI_ReduceMotion"), Mission->IsReduceMotion()));
	Button(ScreenButton, ScreenLabel, Txt(Mission->IsFullscreen() ? TEXT("UI_Fullscreen") : TEXT("UI_Window")));
	Button(QuitButton, QuitLabel, Txt(TEXT("UI_Quit")));
}

void UTDSettingsWidget::HandleResume() { Close(); }

void UTDSettingsWidget::HandleRestart()
{
	Close();
	if (UTDMissionSubsystem* Mission = GetMission()) { Mission->RestartSameSeed(); }
}

void UTDSettingsWidget::HandleNewMission()
{
	Close();
	if (UTDMissionSubsystem* Mission = GetMission()) { Mission->StartNewMission(); }
}

void UTDSettingsWidget::HandleBand()
{
	if (UTDMissionSubsystem* Mission = GetMission())
	{
		Mission->SetPreferredBand(Mission->GetPreferredBand() == ETDAgeBand::Cadet ? ETDAgeBand::Commander : ETDAgeBand::Cadet);
	}
	RefreshTexts();
}

void UTDSettingsWidget::HandleLanguage()
{
	if (UTDMissionSubsystem* Mission = GetMission())
	{
		Mission->SetLanguage(Mission->GetLanguage() == ETDLanguage::EN ? ETDLanguage::TR : ETDLanguage::EN);
	}
	RefreshTexts();
}

void UTDSettingsWidget::HandleSound()
{
	if (UTDMissionSubsystem* Mission = GetMission()) { Mission->SetSoundOn(!Mission->IsSoundOn()); }
	RefreshTexts();
}

void UTDSettingsWidget::HandleMotion()
{
	if (UTDMissionSubsystem* Mission = GetMission()) { Mission->SetReduceMotion(!Mission->IsReduceMotion()); }
	RefreshTexts();
}

void UTDSettingsWidget::HandleScreen()
{
	if (UTDMissionSubsystem* Mission = GetMission()) { Mission->SetFullscreen(!Mission->IsFullscreen()); }
	RefreshTexts();
}

void UTDSettingsWidget::HandleQuit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

// UTDHudWidget

void UTDHudWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UTDMissionSubsystem* Mission = GetMission())
	{
		Mission->OnAllocationChanged.AddDynamic(this, &UTDHudWidget::HandleAllocation);
		Mission->OnLevelsChanged.AddDynamic(this, &UTDHudWidget::HandleLevels);
		Mission->OnPhaseStarted.AddDynamic(this, &UTDHudWidget::HandlePhaseStarted);
		Mission->OnEventDrawn.AddDynamic(this, &UTDHudWidget::HandleEventDrawn);
		Mission->OnPhaseResolved.AddDynamic(this, &UTDHudWidget::HandlePhaseResolved);
		Mission->OnMessageArrived.AddDynamic(this, &UTDHudWidget::HandleMessage);
		Mission->OnMissionEnded.AddDynamic(this, &UTDHudWidget::HandleEnded);
		Mission->OnSpaceWeatherReady.AddDynamic(this, &UTDHudWidget::HandleWeather);
		Mission->OnSettingsChanged.AddDynamic(this, &UTDHudWidget::HandleSettingsChanged);
		if (Mission->HasSpaceWeather())
		{
			HandleWeather(Mission->GetSpaceWeather());
		}
	}
	if (CallHomeButton) { CallHomeButton->OnClicked.AddDynamic(this, &UTDHudWidget::HandleCallHome); }
	if (SettingsButton) { SettingsButton->OnClicked.AddDynamic(this, &UTDHudWidget::HandleSettingsButton); }
	if (LanguageButton) { LanguageButton->OnClicked.AddDynamic(this, &UTDHudWidget::HandleLanguageButton); }
	if (MuteButton) { MuteButton->OnClicked.AddDynamic(this, &UTDHudWidget::HandleMuteButton); }
	if (SourcesButton) { SourcesButton->OnClicked.AddDynamic(this, &UTDHudWidget::HandleSourcesButton); }
	if (UTDShotDirector* Director = UTDShotDirector::Get(this))
	{
		Director->OnShotStarted.AddDynamic(this, &UTDHudWidget::HandleShotStarted);
		Director->OnShotEnded.AddDynamic(this, &UTDHudWidget::HandleShotEnded);
		Director->OnShotFinished.AddDynamic(this, &UTDHudWidget::HandleShotFinished);
	}
	// The debug block belongs to the test map only.
	if (DebugText && !GetWorld()->GetMapName().Contains(TEXT("L_TD_Test")))
	{
		SetShown(DebugText->GetParent() ? static_cast<UWidget*>(DebugText->GetParent()) : static_cast<UWidget*>(DebugText.Get()), false);
		DebugText = nullptr;
	}
	RefreshCorner();
	RefreshNow();
	ApplyConsoleMode();
}

void UTDHudWidget::NativeDestruct()
{
	if (UTDMissionSubsystem* Mission = GetMission())
	{
		Mission->OnAllocationChanged.RemoveAll(this);
		Mission->OnLevelsChanged.RemoveAll(this);
		Mission->OnPhaseStarted.RemoveAll(this);
		Mission->OnEventDrawn.RemoveAll(this);
		Mission->OnPhaseResolved.RemoveAll(this);
		Mission->OnMessageArrived.RemoveAll(this);
		Mission->OnMissionEnded.RemoveAll(this);
		Mission->OnSpaceWeatherReady.RemoveAll(this);
		Mission->OnSettingsChanged.RemoveAll(this);
	}
	if (UTDShotDirector* Director = UTDShotDirector::Get(this))
	{
		Director->OnShotStarted.RemoveAll(this);
		Director->OnShotEnded.RemoveAll(this);
		Director->OnShotFinished.RemoveAll(this);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(IdleTimer);
		World->GetTimerManager().ClearTimer(OpeningLineTimer);
	}
	Super::NativeDestruct();
}

void UTDHudWidget::HandleAllocation(const FTDMissionState& State)
{
	AdvanceOpening(State);
	RestartIdleTimer();
	RefreshFromState(State);
}

void UTDHudWidget::HandleLevels(const FTDMissionState& State, const TArray<ETDSystem>& Changed)
{
	RefreshFromState(State);
}

void UTDHudWidget::HandlePhaseStarted(ETDPhase Phase)
{
	UTDMissionSubsystem* Mission = GetMission();
	if (!Mission)
	{
		return;
	}
	if (MissionReport) { MissionReport->SetVisibility(ESlateVisibility::Collapsed); }
	if (BackToLander) { BackToLander->SetVisibility(ESlateVisibility::Collapsed); }
	if (EventCard && Mission->GetState().CurrentEventId.IsNone()) { EventCard->HideCard(); }

	const FTDMissionState& State = Mission->GetState();
	const bool bFreshStart = State.PhaseIndex == Mission->GetPlayablePhases()[0] && State.SolsElapsed == 0;
	if (bFreshStart)
	{
		if (ResultText) { ResultText->SetText(FText::GetEmpty()); ShowPanelOf(ResultText, false); }
		if (!Mission->bOpeningShown)
		{
			// Opening minute (gdd.md §1 Walkthrough): dark base, one cell to AIR shown by the ghost hand.
			OpeningStep = 0;
			SayCapcom(TEXT("Capcom_Opening_1"));
			TWeakObjectPtr<UTDHudWidget> WeakThis(this);
			GetWorld()->GetTimerManager().SetTimer(OpeningLineTimer, FTimerDelegate::CreateLambda([WeakThis]()
			{
				if (WeakThis.IsValid() && WeakThis->OpeningStep == 0)
				{
					WeakThis->SayCapcom(TEXT("Capcom_Opening_2"));
				}
			}), 3.f, false);
			for (ETDSystem System : { ETDSystem::Greenhouse, ETDSystem::Shelter })
			{
				PowerPanel->GetSlot(System)->SetOpeningBlocked(true);
			}
			PowerPanel->GetSlot(ETDSystem::Air)->SetHint(true);
		}
		else
		{
			SayCapcom(TEXT("Capcom_Restart"));
		}
	}
	RestartIdleTimer();
	RefreshNow();
}

void UTDHudWidget::AdvanceOpening(const FTDMissionState& State)
{
	UTDMissionSubsystem* Mission = GetMission();
	if (OpeningStep < 0 || !Mission)
	{
		return;
	}
	const int32 Placed = UTDMissionRules::GetCellsPlaced(State);
	if (OpeningStep == 0 && State.Allocation[static_cast<int32>(ETDSystem::Air)] > 0)
	{
		OpeningStep = 1;
		ClearHints();
		for (ETDSystem System : { ETDSystem::Greenhouse, ETDSystem::Shelter })
		{
			PowerPanel->GetSlot(System)->SetOpeningBlocked(false);
		}
		SayCapcom(TEXT("Capcom_Opening_3"));
	}
	if (OpeningStep == 1 && Placed >= State.CellsTotal)
	{
		OpeningStep = -1;
		Mission->bOpeningShown = true;
		SayCapcom(TEXT("Capcom_Opening_4"));
	}
}

void UTDHudWidget::HandleEventDrawn(FName EventId)
{
	ShotRequestedFor = NAME_None;
	bAwaitingShot = false;
	if (EventCard)
	{
		EventCard->ShowEvent(EventId);
	}
	ApplyConsoleMode(); // walking: the card waits for the console
}

void UTDHudWidget::HandlePhaseResolved(const FText& Cause)
{
	SetLabel(ResultText, Cause, TDTokens::SizeBody, TDTokens::CoreText());
	ShowPanelOf(ResultText, !Cause.IsEmpty());
}

void UTDHudWidget::ShowPanelOf(UTextBlock* Text, bool bShown)
{
	if (Text)
	{
		SetShown(Text->GetParent() ? static_cast<UWidget*>(Text->GetParent()) : static_cast<UWidget*>(Text), bShown);
	}
}

void UTDHudWidget::SayCapcom(FName Key)
{
	if (UTDMissionSubsystem* Mission = GetMission()) { Mission->LastCapcomKey = Key; Mission->PushComms(Txt(Key)); }
	if (Capcom) { Capcom->SetLine(Txt(Key)); }
}

void UTDHudWidget::HandleMessage(const FText& Message)
{
	if (UTDMissionSubsystem* Mission = GetMission()) { Mission->PushComms(Message); }
	if (Capcom)
	{
		Capcom->SetLine(Message);
	}
}

void UTDHudWidget::HandleEnded(ETDMissionResult Result)
{
	ClearHints();
	ShowPanelOf(ResultText, false);
	if (EventCard) { EventCard->HideCard(); }
	if (Result == ETDMissionResult::LostAir)
	{
		ATDPlayerController* PC = Cast<ATDPlayerController>(GetOwningPlayer());
		const UTDMissionSubsystem* EndMission = GetMission();
		if (PC && EndMission && EndMission->HasFlownDesign())
		{
			PC->ShowDebrief(); // the debrief explains what broke and offers the redesign
		}
		else if (BackToLander) { BackToLander->ShowLost(); }
	}
	else if (MissionReport)
	{
		// Departure Outcome Shot (S07) first; the report follows on OnShotFinished.
		UTDShotDirector* Director = UTDShotDirector::Get(this);
		UTDMissionSubsystem* Mission = GetMission();
		if (Director && Mission)
		{
			static const FName Departure(TEXT("Departure"));
			bPendingReport = true;
			const FName Key = Director->GetResultTextKey(Departure, 0);
			if (!Key.IsNone())
			{
				Mission->LastCauseKey = Key;
				SetLabel(ResultText, Director->GetResultText(Departure, 0), TDTokens::SizeBody, TDTokens::CoreText());
				ShowPanelOf(ResultText, true);
			}
			Director->PlayShot(Departure, 0);
		}
		else
		{
			ShowReportNow();
		}
	}
	RefreshNow();
}

void UTDHudWidget::ShowReportNow()
{
	bPendingReport = false;
	ShowPanelOf(ResultText, false);
	ATDPlayerController* PC = Cast<ATDPlayerController>(GetOwningPlayer());
	const UTDMissionSubsystem* EndMission = GetMission();
	if (PC && EndMission && EndMission->HasFlownDesign())
	{
		PC->ShowDebrief();
		return;
	}
	if (MissionReport)
	{
		MissionReport->ShowReport();
		if (UTDMissionSubsystem* Mission = GetMission()) { Mission->PlaySfx(TEXT("Stamp")); }
	}
}

void UTDHudWidget::SetConsoleMode(bool bOpen)
{
	bConsoleMode = bOpen;
	ApplyConsoleMode();
}

void UTDHudWidget::ApplyConsoleMode()
{
	// The decision UI moved to the Mission Control console (STDBaseComputer) and the helmet HUD (STDSurfaceHud).
	// The card stays "open" but invisible so the outcome shot still plays after a choice.
	if (PowerPanel) { PowerPanel->SetVisibility(ESlateVisibility::Collapsed); }
	if (EventCard && EventCard->IsOpen()) { EventCard->SetVisibility(ESlateVisibility::Hidden); }
	for (UWidget* Corner : TArray<UWidget*>{ SettingsButton, LanguageButton, MuteButton, SourcesButton, CallHomeButton })
	{
		if (Corner) { Corner->SetVisibility(ESlateVisibility::Collapsed); }
	}
	for (UWidget* Old : TArray<UWidget*>{ PhaseStrip, Capcom, MessageTrack, SunToday })
	{
		if (Old) { Old->SetVisibility(ESlateVisibility::Collapsed); }
	}
	// Their backdrop panels too (StripPanel, CapcomPanel in WBP_TD_HUD).
	if (PhaseStrip && PhaseStrip->GetParent()) { PhaseStrip->GetParent()->SetVisibility(ESlateVisibility::Collapsed); }
	if (Capcom && Capcom->GetParent() && Capcom->GetParent()->GetParent()) { Capcom->GetParent()->GetParent()->SetVisibility(ESlateVisibility::Collapsed); }
	if (DoseText) { ShowPanelOf(DoseText, false); }
}

void UTDHudWidget::RequestOutcomeShot(const FTDMissionState& State)
{
	ShotRequestedFor = State.CurrentEventId;
	UTDShotDirector* Director = UTDShotDirector::Get(this);
	UTDMissionSubsystem* Mission = GetMission();
	if (!Director || !Mission)
	{
		EventCard->FlipToBack();
		return;
	}
	// The one-sentence result shows at once, not after the shot.
	const FName Key = Director->GetResultTextKey(State.CurrentEventId, State.EventChoice);
	if (!Key.IsNone())
	{
		Mission->LastCauseKey = Key;
		SetLabel(ResultText, Director->GetResultText(State.CurrentEventId, State.EventChoice), TDTokens::SizeBody, TDTokens::CoreText());
		ShowPanelOf(ResultText, true);
	}
	bAwaitingShot = true;
	Director->PlayShot(State.CurrentEventId, State.EventChoice);
}

void UTDHudWidget::HandleShotStarted(FName ShotId, FName CameraTag, int32 OutcomeState)
{
	// The 2D layer stays on screen, dimmed and click-through, so any left click reaches the shot skip.
	if (!bShotDimmed)
	{
		bShotDimmed = true;
		VisibilityBeforeShot = GetVisibility();
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
	// Panel dimmed but opaque; the card steps aside so the 3D outcome is seen, then flips on OnShotFinished.
	if (PowerPanel) { PowerPanel->SetColorAndOpacity(FLinearColor(0.45f, 0.45f, 0.45f, 1.f)); }
	if (EventCard) { EventCard->SetRenderOpacity(0.f); }
}

void UTDHudWidget::HandleShotEnded(FName ShotId)
{
	if (bShotDimmed)
	{
		bShotDimmed = false;
		SetVisibility(VisibilityBeforeShot);
	}
	if (PowerPanel) { PowerPanel->SetColorAndOpacity(FLinearColor::White); }
	if (EventCard) { EventCard->SetRenderOpacity(1.f); }
}

void UTDHudWidget::HandleShotFinished(FName EventId)
{
	if (bPendingReport && EventId == FName(TEXT("Departure")))
	{
		ShowReportNow();
		return;
	}
	if (bAwaitingShot && EventId == ShotRequestedFor)
	{
		bAwaitingShot = false;
		if (EventCard && EventCard->IsOpen() && !EventCard->IsShowingBack())
		{
			EventCard->FlipToBack();
		}
	}
}

void UTDHudWidget::HandleWeather(const FTDSpaceWeather& Weather)
{
	if (SunToday)
	{
		SunToday->ShowWeather(Weather);
	}
	ApplyConsoleMode();
}

void UTDHudWidget::HandleSettingsChanged()
{
	RefreshCorner();
	if (const UTDMissionSubsystem* Mission = GetMission())
	{
		// Redraw the lines on screen in the new language.
		if (Capcom && !Mission->LastCapcomKey.IsNone()) { Capcom->SetLine(Txt(Mission->LastCapcomKey)); }
		if (ResultText && !Mission->LastCauseKey.IsNone() && !ResultText->GetText().IsEmpty()) { ResultText->SetText(Txt(Mission->LastCauseKey)); }
	}
	if (EventCard && EventCard->IsOpen()) { EventCard->RefreshTexts(); }
	if (SettingsPanel && IsShown(SettingsPanel)) { SettingsPanel->RefreshTexts(); }
	if (const UTDMissionSubsystem* Mission = GetMission(); Mission && Mission->HasSpaceWeather() && SunToday) { SunToday->ShowWeather(Mission->GetSpaceWeather()); }
	RefreshNow();
}

void UTDHudWidget::HandleCallHome()
{
	if (UTDMissionSubsystem* Mission = GetMission())
	{
		Mission->CallHome();
	}
}

void UTDHudWidget::HandleSettingsButton() { OpenSettings(); }
void UTDHudWidget::HandleSourcesButton() { OpenSources(); }

void UTDHudWidget::HandleLanguageButton()
{
	if (UTDMissionSubsystem* Mission = GetMission())
	{
		Mission->SetLanguage(Mission->GetLanguage() == ETDLanguage::EN ? ETDLanguage::TR : ETDLanguage::EN);
	}
}

void UTDHudWidget::HandleMuteButton()
{
	if (UTDMissionSubsystem* Mission = GetMission())
	{
		Mission->SetSoundOn(!Mission->IsSoundOn());
	}
}

void UTDHudWidget::OpenSettings()
{
	if (SettingsPanel) { SettingsPanel->Open(); }
}

void UTDHudWidget::OpenSources()
{
	if (SourcePanel) { SourcePanel->Open(); }
}

void UTDHudWidget::OpenSunPanel()
{
	if (SunToday) { SunToday->OpenPanel(); }
}

void UTDHudWidget::HandleBack()
{
	if (EventCard && EventCard->IsShowingBack()) { EventCard->HideCard(); return; }
	if (SourcePanel && IsShown(SourcePanel)) { SourcePanel->Close(); return; }
	if (SunToday && SunToday->IsPanelOpen()) { SunToday->ClosePanel(); return; }
	if (SettingsPanel && IsShown(SettingsPanel)) { SettingsPanel->Close(); return; }
	OpenSettings();
}

void UTDHudWidget::RefreshCorner()
{
	const UTDMissionSubsystem* Mission = GetMission();
	if (!Mission)
	{
		return;
	}
	auto Corner = [this](UButton* Button, UTextBlock* Label, const FText& Value)
	{
		StyleButton(Button, TDTokens::CoreSecondary());
		SetLabel(Label, Value, TDTokens::SizeCaption, TDTokens::CoreText());
	};
	Corner(SettingsButton, SettingsLabel, Txt(TEXT("UI_Settings")));
	Corner(LanguageButton, LanguageLabel, FText::FromString(Mission->GetLanguage() == ETDLanguage::EN ? TEXT("TR") : TEXT("EN")));
	Corner(MuteButton, MuteLabel, Txt(Mission->IsSoundOn() ? TEXT("UI_Mute") : TEXT("UI_Unmute")));
	Corner(SourcesButton, SourcesLabel, Txt(TEXT("UI_WhereFrom")));
}

void UTDHudWidget::RestartIdleTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(IdleTimer, FTimerDelegate::CreateUObject(this, &UTDHudWidget::ShowIdleHint), IdleHintSeconds, false);
	}
	if (OpeningStep != 0)
	{
		ClearHints();
	}
}

void UTDHudWidget::ShowIdleHint()
{
	const UTDMissionSubsystem* Mission = GetMission();
	if (!Mission || !Mission->IsMissionRunning() || OpeningStep == 0)
	{
		return;
	}
	const FTDMissionState& State = Mission->GetState();
	if (UTDMissionRules::GetCellsPlaced(State) >= State.CellsTotal)
	{
		return;
	}
	// Ghost hand over the next valid move: the open system with the biggest unmet demand, lowest level first.
	const TArray<int32> Demand = Mission->GetDemand();
	int32 Best = INDEX_NONE;
	for (int32 Index = 0; Index < TD::NumSystems; ++Index)
	{
		if (Mission->IsSystemLocked(static_cast<ETDSystem>(Index)))
		{
			continue;
		}
		const int32 Gap = Demand[Index] - State.Allocation[Index];
		if (Best == INDEX_NONE || Gap > Demand[Best] - State.Allocation[Best] || (Gap == Demand[Best] - State.Allocation[Best] && State.Levels[Index] < State.Levels[Best]))
		{
			Best = Index;
		}
	}
	if (Best != INDEX_NONE)
	{
		PowerPanel->GetSlot(static_cast<ETDSystem>(Best))->SetHint(true);
	}
}

void UTDHudWidget::ClearHints()
{
	for (ETDSystem System : { ETDSystem::Air, ETDSystem::Greenhouse, ETDSystem::Shelter })
	{
		PowerPanel->GetSlot(System)->SetHint(false);
	}
}

void UTDHudWidget::RefreshNow()
{
	if (const UTDMissionSubsystem* Mission = GetMission())
	{
		RefreshFromState(Mission->GetState());
	}
}

void UTDHudWidget::RefreshFromState(const FTDMissionState& State)
{
	const UTDMissionSubsystem* Mission = GetMission();
	if (!Mission || !Mission->HasMission())
	{
		return;
	}
	PhaseStrip->RefreshFromState(State);
	PowerPanel->RefreshFromState(State);
	if (MessageTrack) { MessageTrack->RefreshFromState(State); }
	if (EventCard && EventCard->IsOpen() && !EventCard->IsShowingBack() && State.EventChoice >= 0 && ShotRequestedFor != State.CurrentEventId)
	{
		RequestOutcomeShot(State);
	}

	if (CallHomeButton)
	{
		const FTDPhaseRow* Phase = Mission->GetPhaseRow(State.PhaseIndex);
		const bool bBlackout = Phase && Phase->bIsBlackout;
		const bool bCan = Mission->CanCallHome();
		// The Call home button appears in the opening only after the first allocation (walkthrough step 4).
		SetShownInteractive(CallHomeButton, OpeningStep < 0);
		CallHomeButton->SetIsEnabled(bCan);
		StyleButton(CallHomeButton, bCan ? TDTokens::Hex(TEXT("C9D3E0")) : TDTokens::SemDisabled());
		SetLabel(CallHomeLabel, Txt(bBlackout ? TEXT("UI_NoSignal") : TEXT("UI_CallHome")), TDTokens::SizeBody, TDTokens::CorePrimary());
	}

	if (DoseText)
	{
		const bool bCommanderRadiation = State.Band == ETDAgeBand::Commander && State.StormExposures > 0;
		SetLabel(DoseText, bCommanderRadiation ? FText::Format(Txt(TEXT("UI_DoseBadge")), FText::AsNumber(State.StormExposures)) : FText::GetEmpty(),
			TDTokens::SizeCaption, TDTokens::Hex(TEXT("F07F2E")));
		ShowPanelOf(DoseText, bCommanderRadiation);
	}

	ApplyConsoleMode();
	if (DebugText)
	{
		DebugText->SetText(FText::FromString(Mission->GetDebugString()));
		StyleText(DebugText, TDTokens::SizeCaption, TDTokens::CoreText());
	}
}
