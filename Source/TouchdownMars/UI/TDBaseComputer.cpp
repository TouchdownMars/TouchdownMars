// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "UI/TDBaseComputer.h"

#include "Engine/DataTable.h"
#include "Fonts/CompositeFont.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Model/TDMissionRules.h"
#include "Systems/TDMissionSubsystem.h"
#include "Systems/TDShotDirector.h"
#include "UI/TDDesignTokens.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FSlateBrush* White() { return FCoreStyle::Get().GetBrush("GenericWhiteBox"); }

	/** Same NASA-punk palette as the design room (docs/design.md "Mission screens"). */
	namespace Pal
	{
		inline FLinearColor Hex(const TCHAR* V, float A = 1.f) { FLinearColor C = FLinearColor(FColor::FromHex(V)); C.A = A; return C; }
		inline FLinearColor Dim() { return Hex(TEXT("080A0D"), 0.55f); }
		inline FLinearColor Bg() { return Hex(TEXT("E2DFD8")); }
		inline FLinearColor Panel() { return Hex(TEXT("EFEDE8"), 0.97f); }
		inline FLinearColor Raised() { return Hex(TEXT("FAF9F6")); }
		inline FLinearColor Line() { return Hex(TEXT("C8C4BA")); }
		inline FLinearColor Track() { return Hex(TEXT("D9D5CC")); }
		inline FLinearColor Text() { return Hex(TEXT("15181C")); }
		inline FLinearColor Text2() { return Hex(TEXT("5C636E")); }
		inline FLinearColor Accent() { return Hex(TEXT("22272E")); }
		inline FLinearColor Red() { return Hex(TEXT("E03C31")); }
		inline FLinearColor Go() { return Hex(TEXT("1E8E4E")); }
		inline FLinearColor Caution() { return Hex(TEXT("B7791F")); }
		inline FLinearColor NoGo() { return Hex(TEXT("D7261E")); }
		inline FLinearColor White() { return Hex(TEXT("FFFFFF")); }
	}

	TSharedPtr<const FCompositeFont> TeamFont()
	{
		static TSharedPtr<FCompositeFont> Cached;
		static bool bTried = false;
		if (!bTried)
		{
			bTried = true;
			const FString Dir = FPaths::ProjectContentDir() / TEXT("TouchdownMars/UI/Fonts");
			const FString Regular = Dir / TEXT("TDFont_Regular.ttf");
			const FString Bold = Dir / TEXT("TDFont_Bold.ttf");
			if (FPaths::FileExists(Regular) && FPaths::FileExists(Bold))
			{
				Cached = MakeShared<FCompositeFont>();
				Cached->DefaultTypeface.AppendFont(TEXT("Regular"), Regular, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
				Cached->DefaultTypeface.AppendFont(TEXT("Bold"), Bold, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
				const FString Mono = Dir / TEXT("TDFont_Mono.ttf");
				if (FPaths::FileExists(Mono))
				{
					Cached->DefaultTypeface.AppendFont(TEXT("Mono"), Mono, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
				}
			}
		}
		return Cached;
	}

	FSlateFontInfo Font(int32 Size, bool bBold = false)
	{
		if (TSharedPtr<const FCompositeFont> Team = TeamFont())
		{
			return FSlateFontInfo(Team, Size, bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular")));
		}
		return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
	}

	FSlateFontInfo HeadFont(int32 Size)
	{
		FSlateFontInfo Info = Font(Size, true);
		Info.LetterSpacing = 90;
		return Info;
	}

	FSlateFontInfo NumFont(int32 Size)
	{
		if (TSharedPtr<const FCompositeFont> Team = TeamFont())
		{
			return FSlateFontInfo(Team, Size, FName(TEXT("Mono")));
		}
		return FCoreStyle::GetDefaultFontStyle("Mono", Size);
	}

	TSharedRef<STextBlock> Label(const FText& Text, int32 Size, const FLinearColor& Color, bool bBold = false, bool bWrap = true)
	{
		return SNew(STextBlock).Text(Text).Font(Font(Size, bBold)).ColorAndOpacity(Color).AutoWrapText(bWrap);
	}

	TSharedRef<SWidget> Box(const TSharedRef<SWidget>& Content, const FLinearColor& Color, float Pad = 14.f)
	{
		return SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Color).Padding(Pad)[Content];
	}

	const FButtonStyle* FlatStyle()
	{
		static FSlateColorBrush Normal(FLinearColor::White);
		static FSlateColorBrush Hovered(FLinearColor(0.82f, 0.82f, 0.82f, 1.f));
		static FSlateColorBrush Pressed(FLinearColor(0.66f, 0.66f, 0.66f, 1.f));
		static FSlateColorBrush Disabled(FLinearColor::White);
		static const FButtonStyle Style = FButtonStyle().SetNormal(Normal).SetHovered(Hovered).SetPressed(Pressed).SetDisabled(Disabled)
			.SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f));
		return &Style;
	}

	TSharedRef<SWidget> Button(const FText& Text, FOnClicked OnClicked, bool bEnabled, const FLinearColor& Color, int32 Size = 15)
	{
		return SNew(SButton)
			.ButtonStyle(FlatStyle())
			.IsEnabled(bEnabled)
			.ButtonColorAndOpacity(bEnabled ? Color : Pal::Track())
			.ContentPadding(FMargin(14.f, 7.f))
			.HAlign(HAlign_Center)
			.OnClicked(OnClicked)
			[
				SNew(STextBlock).Text(Text).Font(Font(Size, true))
					.ColorAndOpacity(!bEnabled ? Pal::Text2() : Color.GetLuminance() < 0.45f ? Pal::White() : Pal::Text())
			];
	}

	/** Hours bar: planned of available, with an over-the-line colour. */
	TSharedRef<SWidget> HoursBar(float Planned, float Available, const FLinearColor& Fill)
	{
		const float Ratio = Available > 0.f ? FMath::Clamp(Planned / Available, 0.f, 1.f) : 0.f;
		return SNew(SBox).HeightOverride(10.f)
		[
			SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Pal::Track()).Padding(0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(FMath::Max(0.001f, Ratio))[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Fill)]
				+ SHorizontalBox::Slot().FillWidth(FMath::Max(0.001f, 1.f - Ratio))[SNullWidget::NullWidget]
			]
		];
	}

	FText Hours(float H)
	{
		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = 0;
		Options.MaximumFractionalDigits = 1;
		return FText::AsNumber(H, &Options);
	}

	FLinearColor RoleColor(ETDRole Role)
	{
		switch (Role)
		{
		case ETDRole::Planner: return TDTokens::Hex(TEXT("B58CF0"));
		case ETDRole::Engineer: return TDTokens::Hex(TEXT("F07F2E"));
		case ETDRole::Science: return TDTokens::Hex(TEXT("3FB27F"));
		default: return TDTokens::Hex(TEXT("3A8DDE"));
		}
	}

	const TCHAR* RoleKey(ETDRole Role)
	{
		switch (Role)
		{
		case ETDRole::Planner: return TEXT("Role_Planner");
		case ETDRole::Engineer: return TEXT("Role_Engineer");
		case ETDRole::Science: return TEXT("Role_Science");
		default: return TEXT("Role_Comms");
		}
	}

	/** Setup checklist (research: the habitat computer opens on a checklist that doubles as onboarding). */
	const TCHAR* ChecklistIds[] = { TEXT("InspectCargo"), TEXT("SuitCheck"), TEXT("OffloadRover"), TEXT("OffloadPayload"), TEXT("SetUpGreenhouse"),
		TEXT("SurveyClays") };
}

void STDBaseComputer::Construct(const FArguments& InArgs)
{
	Mission = InArgs._Mission;
	OnClose = InArgs._OnClose;
	OnRun = InArgs._OnRun;
	SetVisibility(TAttribute<EVisibility>::CreateLambda([this]()
	{
		const UTDShotDirector* Director = Mission.IsValid() ? UTDShotDirector::Get(Mission.Get()) : nullptr;
		return Director && Director->IsPlayingShot() ? EVisibility::Collapsed : EVisibility::Visible;
	}));
	ChildSlot
	[
		SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Pal::Dim()).Padding(0.f)
		[
			SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
			[
				SNew(SBox).WidthOverride(1920.f).HeightOverride(1080.f)
				[
					SAssignNew(Root, SBorder).BorderImage(White()).BorderBackgroundColor(FLinearColor::Transparent).Padding(32.f)
				]
			]
		]
	];
	if (Mission.IsValid())
	{
		OpsChangedHandle = Mission->OnOpsChanged.AddSP(this, &STDBaseComputer::Rebuild);
	}
	Rebuild();
}

STDBaseComputer::~STDBaseComputer()
{
	if (Mission.IsValid())
	{
		Mission->OnOpsChanged.Remove(OpsChangedHandle);
	}
}

FReply STDBaseComputer::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::B || InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::Tab
		|| InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Top || InKeyEvent.GetKey() == EKeys::Gamepad_Special_Left)
	{
		OnClose.ExecuteIfBound();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FText STDBaseComputer::Txt(const TCHAR* Key) const
{
	return Mission.IsValid() ? Mission->GetText(FName(Key)) : FText::FromString(Key);
}

FText STDBaseComputer::Txt(FName Key) const
{
	return Mission.IsValid() ? Mission->GetText(Key) : FText::FromName(Key);
}

void STDBaseComputer::Rebuild()
{
	if (!Root.IsValid() || !Mission.IsValid())
	{
		return;
	}
	TSharedRef<SVerticalBox> Page = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[BuildHeader()];
	if (Tab == ETab::Crew)
	{
		Page->AddSlot().AutoHeight().Padding(0.f, 12.f)[BuildCrew()];
		Page->AddSlot().FillHeight(1.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.72f).Padding(0.f, 0.f, 12.f, 0.f)[BuildBoard()]
			+ SHorizontalBox::Slot().FillWidth(0.28f)[BuildSide()]
		];
	}
	else
	{
		Page->AddSlot().FillHeight(1.f).Padding(0.f, 12.f, 0.f, 0.f)[Tab == ETab::Power ? BuildPower() : Tab == ETab::Data ? BuildData() : BuildDecision()];
	}
	Page->AddSlot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)[BuildFooter()];
	Root->SetContent(Page);
}

TSharedRef<SWidget> STDBaseComputer::BuildHeader()
{
	UTDMissionSubsystem* M = Mission.Get();
	int32 SolStart = 0;
	int32 SolEnd = 0;
	M->GetPhaseSols(M->GetState().PhaseIndex, SolStart, SolEnd);
	const FTDPhaseRow* Phase = M->GetPhaseRow(M->GetState().PhaseIndex);
	return Box(SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Txt(TEXT("Ops_Title"))).Font(HeadFont(28)).ColorAndOpacity(Pal::Text())]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(FText::Format(Txt(TEXT("Ops_Sols")), SolStart, SolEnd, Phase ? Txt(Phase->NameKey) : FText::GetEmpty()))
					.Font(NumFont(16)).ColorAndOpacity(Pal::Red())
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16.f, 0.f)[BuildTabs()]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f)
		[
			Tab != ETab::Crew ? SNullWidget::NullWidget :
			Button(Txt(TEXT("Ops_Auto")), FOnClicked::CreateLambda([this]() { if (Mission.IsValid()) { Mission->AutoPlanTasks(); } return FReply::Handled(); }),
				!M->IsRunningPhase(), Pal::Accent())
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f)
		[
			Tab != ETab::Crew ? SNullWidget::NullWidget :
			Button(Txt(TEXT("Ops_Clear")), FOnClicked::CreateLambda([this]() { if (Mission.IsValid()) { Mission->ClearPlan(); } return FReply::Handled(); }),
				!M->IsRunningPhase(), Pal::Line())
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f)
		[
			Button(Txt(TEXT("UI_CallHome")), FOnClicked::CreateLambda([this]()
			{
				if (Mission.IsValid()) { Mission->CallHome(); }
				Rebuild();
				return FReply::Handled();
			}), M->CanCallHome(), TDTokens::Hex(TEXT("3A8DDE")))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f, 0.f, 0.f)
		[
			Button(Txt(TEXT("Ops_Close")), FOnClicked::CreateLambda([this]() { OnClose.ExecuteIfBound(); return FReply::Handled(); }), true, Pal::Line())
		], Pal::Panel(), 16.f);
}

TSharedRef<SWidget> STDBaseComputer::BuildCrew()
{
	UTDMissionSubsystem* M = Mission.Get();
	const FTDOpsState& Ops = M->GetOps();
	const float EvaAvailable = M->GetPhaseEvaHours();
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (int32 CrewIndex = 0; CrewIndex < Ops.Crew.Num(); ++CrewIndex)
	{
		const FTDCrewPlan& Crew = Ops.Crew[CrewIndex];
		const float Available = M->GetCrewPhaseHours(CrewIndex);
		const bool bTired = Ops.Fatigue.IsValidIndex(CrewIndex) && Ops.Fatigue[CrewIndex] > 0;
		const bool bBroken = Ops.RestBroken.Contains(CrewIndex);
		const float Planned = M->GetPlannedHours(CrewIndex, false);
		const float Eva = M->GetPlannedHours(CrewIndex, true);
		const bool bSelected = CrewIndex == SelectedCrew;
		TSharedRef<SWrapBox> Chips = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4.f, 4.f));
		for (const FName& Id : Crew.Tasks)
		{
			const FTDTaskRow* Task = M->GetTasksTable() ? M->GetTasksTable()->FindRow<FTDTaskRow>(Id, TEXT("TDBaseComputer"), false) : nullptr;
			Chips->AddSlot()
			[
				SNew(SButton).ButtonStyle(FlatStyle()).ButtonColorAndOpacity(Pal::Bg()).ContentPadding(FMargin(6.f, 2.f))
				.ToolTipText(Txt(TEXT("Ops_RemoveTip")))
				.OnClicked(FOnClicked::CreateLambda([this, Id]() { if (Mission.IsValid()) { Mission->UnplanTask(Id); } return FReply::Handled(); }))
				[
					SNew(STextBlock).Text(FText::Format(FText::FromString(TEXT("{0}  ×")), Task ? Txt(Task->NameKey) : FText::FromName(Id)))
						.Font(Font(11, true)).ColorAndOpacity(Pal::Text())
				]
			];
		}
		Row->AddSlot().FillWidth(1.f).Padding(CrewIndex == 0 ? 0.f : 6.f, 0.f, 0.f, 0.f)
		[
			SNew(SButton).ButtonStyle(FlatStyle()).ButtonColorAndOpacity(bSelected ? RoleColor(Crew.Role) : Pal::Raised()).ContentPadding(FMargin(12.f, 10.f))
			.OnClicked(FOnClicked::CreateLambda([this, CrewIndex]() { SelectedCrew = CrewIndex; Rebuild(); return FReply::Handled(); }))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SBox).WidthOverride(10.f).HeightOverride(20.f)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(bSelected ? Pal::Text() : RoleColor(Crew.Role))]
					]
					+ SHorizontalBox::Slot().FillWidth(1.f)[Label(M->GetCrewName(CrewIndex), 19, Pal::Text(), true, false)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
					[
						bTired || bBroken ? Box(Label(bBroken ? Txt(TEXT("Ops_RestBadge")) : Txt(TEXT("Ops_Tired")), 11, Pal::Text(), true, false), Pal::Caution(), 3.f)
							: SNullWidget::NullWidget
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Label(Txt(RoleKey(Crew.Role)), 12, Pal::Text2(), true, false)]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 2.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f)[Label(Txt(TEXT("Ops_WorkHours")), 11, Pal::Text2(), true, false)]
					+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::Format(Txt(TEXT("Ops_Hours")), Hours(Planned), Hours(Available))).Font(NumFont(12)).ColorAndOpacity(Pal::Text())]
				]
				+ SVerticalBox::Slot().AutoHeight()[HoursBar(Planned, Available, Pal::Go())]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f, 0.f, 2.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f)[Label(Txt(TEXT("Ops_EvaHours")), 11, Pal::Text2(), true, false)]
					+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::Format(Txt(TEXT("Ops_Hours")), Hours(Eva), Hours(EvaAvailable))).Font(NumFont(12)).ColorAndOpacity(Pal::Text())]
				]
				+ SVerticalBox::Slot().AutoHeight()[HoursBar(Eva, EvaAvailable, Eva > EvaAvailable + 0.01f ? Pal::NoGo() : TDTokens::Hex(TEXT("F07F2E")))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
				[
					Crew.Tasks.Num() > 0 ? StaticCastSharedRef<SWidget>(Chips)
						: StaticCastSharedRef<SWidget>(Label(CrewIndex == 0 ? Txt(TEXT("Ops_YouHint")) : Txt(TEXT("Ops_FreeHint")), 12, Pal::Text2()))
				]
			]
		];
	}
	return Row;
}

TSharedRef<SWidget> STDBaseComputer::BuildBoard()
{
	UTDMissionSubsystem* M = Mission.Get();
	const UDataTable* Tasks = M->GetTasksTable();
	const UDataTable* Sites = M->GetSitesTable();
	TSharedRef<SWrapBox> Cards = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(10.f, 10.f));
	if (Tasks)
	{
		// Open tasks first, then planned, then blocked: the board reads as "what can we do now".
		struct FEntry { FName Id; const FTDTaskRow* Row; ETDTaskBlock Block; TArray<int32> Crew; };
		TArray<FEntry> Entries;
		Tasks->ForeachRow<FTDTaskRow>(TEXT("TDBaseComputer"), [&](const FName& Key, const FTDTaskRow& Row)
		{
			FEntry& E = Entries.AddDefaulted_GetRef();
			E.Id = Key;
			E.Row = &Row;
			E.Crew = UTDOps::CrewOn(M->GetOps(), Key);
			E.Block = E.Crew.Num() > 0 ? ETDTaskBlock::Ok : M->GetTaskBlock(Key, SelectedCrew);
		});
		// Hide what is done for good or impossible with this design.
		Entries.RemoveAll([](const FEntry& E) { return E.Crew.Num() == 0 && (E.Block == ETDTaskBlock::AlreadyDone || E.Block == ETDTaskBlock::NotLanded || E.Block == ETDTaskBlock::WrongPhase); });
		Entries.StableSort([](const FEntry& A, const FEntry& B)
		{
			auto Rank = [](const FEntry& E) { return E.Crew.Num() > 0 ? 1 : (E.Block == ETDTaskBlock::Ok ? 0 : 2); };
			return Rank(A) < Rank(B);
		});
		for (const FEntry& E : Entries)
		{
			const FTDTaskRow& Row = *E.Row;
			const FTDSiteRow* Site = Sites ? Sites->FindRow<FTDSiteRow>(Row.Site, TEXT("TDBaseComputer"), false) : nullptr;
			const bool bPlanned = E.Crew.Num() > 0;
			const bool bOpen = !bPlanned && E.Block == ETDTaskBlock::Ok;
			const int32 Team = FMath::Max(Row.Crew, Row.bEva ? 2 : 1);
			const float H = UTDOps::TaskHours(Row, M->GetConstantsTable());
			TArray<FString> Names;
			for (int32 Index : E.Crew) { Names.Add(M->GetCrewName(Index).ToString()); }
			const FName Id = E.Id;

			TSharedRef<SHorizontalBox> Badges = SNew(SHorizontalBox);
			auto Badge = [&Badges](const FText& Text, const FLinearColor& Fill, const FLinearColor& Ink)
			{
				Badges->AddSlot().AutoWidth().Padding(0.f, 0.f, 5.f, 0.f)[Box(SNew(STextBlock).Text(Text).Font(NumFont(11)).ColorAndOpacity(Ink), Fill, 3.f)];
			};
			Badge(FText::Format(Txt(TEXT("Ops_TaskMeta")), Hours(H), Team), Pal::Bg(), Pal::Text());
			Badge(Row.bEva ? Txt(TEXT("Ops_Eva")) : Txt(TEXT("Ops_Inside")), Row.bEva ? TDTokens::Hex(TEXT("F07F2E")) : Pal::Line(), Pal::Text());
			if (Row.Cells > 0)
			{
				Badge(FText::Format(Txt(TEXT("Ops_UsesCells")), Row.Cells), Pal::Caution(), Pal::Text());
			}

			TSharedRef<SWidget> Action = SNullWidget::NullWidget;
			if (bPlanned)
			{
				Action = Button(FText::Format(Txt(TEXT("Ops_PlannedBy")), FText::FromString(FString::Join(Names, TEXT(", ")))),
					FOnClicked::CreateLambda([this, Id]() { if (Mission.IsValid()) { Mission->UnplanTask(Id); } return FReply::Handled(); }), !M->IsRunningPhase(), Pal::Go(), 13);
			}
			else if (bOpen)
			{
				Action = Button(FText::Format(Txt(TEXT("Ops_AssignTo")), M->GetCrewName(SelectedCrew)),
					FOnClicked::CreateLambda([this, Id]() { if (Mission.IsValid()) { Mission->PlanTask(Id, SelectedCrew); } return FReply::Handled(); }), !M->IsRunningPhase(), Pal::Red(), 13);
			}
			else
			{
				const FString BlockName = StaticEnum<ETDTaskBlock>()->GetNameStringByValue(static_cast<int64>(E.Block));
				TSharedRef<SVerticalBox> Blocked = SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[Label(Txt(FName(*FString::Printf(TEXT("Ops_Block_%s"), *BlockName))), 12, Pal::Caution(), true)];
				if (E.Block == ETDTaskBlock::NoEvaHours)
				{
					// The rule can be broken, at a cost (Frostpunk-style overtime): fatigue next phase and a medical flag.
					Blocked->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
					[
						Button(Txt(TEXT("Ops_Override")), FOnClicked::CreateLambda([this, Id]()
						{
							if (Mission.IsValid()) { Mission->PlanTask(Id, SelectedCrew, true); }
							return FReply::Handled();
						}), !M->IsRunningPhase(), Pal::Caution(), 12)
					];
				}
				Action = Blocked;
			}

			Cards->AddSlot()
			[
				SNew(SBox).WidthOverride(400.f)
				[
					SNew(SBorder).BorderImage(White()).BorderBackgroundColor(bPlanned ? Pal::Go() : bOpen ? Pal::Line() : Pal::Track()).Padding(2.f)
					[
						Box(SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()[Label(Txt(Row.NameKey), 17, bOpen || bPlanned ? Pal::Text() : Pal::Text2(), true)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)[Label(Site ? FText::Format(Txt(TEXT("Ops_At")), Txt(Site->NameKey)) : FText::GetEmpty(), 12, Pal::Text2(), false, false)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[Badges]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 6.f)[Label(Txt(Row.InfoKey), 13, Pal::Text())]
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)[Action],
							Pal::Raised(), 10.f)
					]
				]
			];
		}
	}
	return Box(SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(STextBlock).Text(Txt(TEXT("Ops_Board"))).Font(HeadFont(18)).ColorAndOpacity(Pal::Text())]
			+ SHorizontalBox::Slot().AutoWidth()[Label(FText::Format(Txt(TEXT("Ops_BoardHint")), M->GetCrewName(SelectedCrew)), 13, Pal::Text2(), false, false)]
		]
		+ SVerticalBox::Slot().FillHeight(1.f)[SNew(SScrollBox) + SScrollBox::Slot()[Cards]], Pal::Panel(), 16.f);
}

TSharedRef<SWidget> STDBaseComputer::BuildSide()
{
	UTDMissionSubsystem* M = Mission.Get();
	const FTDOpsState& Ops = M->GetOps();
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[SNew(STextBlock).Text(Txt(TEXT("Ops_Checklist"))).Font(HeadFont(18)).ColorAndOpacity(Pal::Text())];
	for (const TCHAR* Id : ChecklistIds)
	{
		const FTDTaskRow* Row = M->GetTasksTable() ? M->GetTasksTable()->FindRow<FTDTaskRow>(FName(Id), TEXT("TDBaseComputer"), false) : nullptr;
		if (!Row)
		{
			continue;
		}
		const bool bDone = UTDOps::IsDone(Ops, FName(Id));
		const bool bPlanned = UTDOps::CrewOn(Ops, FName(Id)).Num() > 0;
		List->AddSlot().AutoHeight().Padding(0.f, 3.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SBox).WidthOverride(16.f).HeightOverride(16.f)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(bDone ? Pal::Go() : bPlanned ? Pal::Caution() : Pal::Track())]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)[Label(Txt(Row->NameKey), 14, bDone ? Pal::Text2() : Pal::Text(), !bDone)]
		];
	}
	// Base status: water, dust on the arrays, weather station.
	List->AddSlot().AutoHeight().Padding(0.f, 16.f, 0.f, 8.f)[SNew(STextBlock).Text(Txt(TEXT("Ops_Status"))).Font(HeadFont(18)).ColorAndOpacity(Pal::Text())];
	if (M->GetState().bDustPhase)
	{
		List->AddSlot().AutoHeight().Padding(0.f, 2.f)[Label(Txt(TEXT("Ops_StormNow")), 14, Pal::NoGo(), true)];
	}
	if (Ops.SolarDustCells > 0)
	{
		List->AddSlot().AutoHeight().Padding(0.f, 2.f)[Label(FText::Format(Txt(TEXT("Ops_Dust")), Ops.SolarDustCells), 14, Pal::Caution(), true)];
	}
	List->AddSlot().AutoHeight().Padding(0.f, 2.f)
	[
		Label(Ops.bWaterFound ? FText::Format(Txt(TEXT("Ops_Water")), Hours(Ops.WaterKg)) : Txt(TEXT("Ops_NoWaterYet")), 14, Pal::Text())
	];
	List->AddSlot().AutoHeight().Padding(0.f, 2.f)[Label(Ops.bWeatherStation ? Txt(TEXT("Ops_WeatherOn")) : Txt(TEXT("Ops_WeatherOff")), 14, Pal::Text())];
	if (Ops.RoverChargingPhase == M->GetState().PhaseIndex)
	{
		List->AddSlot().AutoHeight().Padding(0.f, 2.f)[Label(Txt(TEXT("Ops_RoverCharging")), 14, Pal::Caution(), true)];
	}
	if (Ops.RestBreaks > 0)
	{
		List->AddSlot().AutoHeight().Padding(0.f, 2.f)[Label(FText::Format(Txt(TEXT("Ops_RestBreaks")), Ops.RestBreaks), 14, Pal::NoGo(), true)];
	}
	List->AddSlot().AutoHeight().Padding(0.f, 2.f)[Label(Txt(TEXT("Ops_EvaRule")), 13, Pal::Text2())];

	const TArray<FText>& Report = M->GetOpsReport();
	if (Report.Num() > 0)
	{
		List->AddSlot().AutoHeight().Padding(0.f, 16.f, 0.f, 8.f)[SNew(STextBlock).Text(Txt(TEXT("Ops_Report"))).Font(HeadFont(18)).ColorAndOpacity(Pal::Text())];
		for (const FText& Line : Report)
		{
			List->AddSlot().AutoHeight().Padding(0.f, 2.f)[Label(Line, 13, Pal::Text())];
		}
	}
	return Box(SNew(SScrollBox) + SScrollBox::Slot()[List], Pal::Panel(), 16.f);
}

TSharedRef<SWidget> STDBaseComputer::BuildFooter()
{
	UTDMissionSubsystem* M = Mission.Get();
	const FTDMissionState& S = M->GetState();
	int32 SolStart = 0;
	int32 SolEnd = 0;
	M->GetPhaseSols(S.PhaseIndex, SolStart, SolEnd);
	const int32 Placed = UTDMissionRules::GetCellsPlaced(S);
	const bool bPower = Placed >= S.CellsTotal;
	const bool bDecision = S.CurrentEventId.IsNone() || S.EventChoice >= 0;
	bool bPlanned = false;
	for (const FTDCrewPlan& Crew : M->GetOps().Crew)
	{
		bPlanned |= Crew.Tasks.Num() > 0;
	}
	const int32 OwnLeft = M->GetOps().Crew.Num() > 0 ? M->GetOps().Crew[0].Tasks.Num() : 0;
	const bool bReady = M->CanEndPhase() && !M->IsRunningPhase();

	TSharedRef<SHorizontalBox> Rows = SNew(SHorizontalBox);
	auto Row = [this, &Rows](const FText& Text, int32 State, ETab Fix)
	{
		// State: 0 GO, 1 caution (optional), 2 NO-GO.
		const FLinearColor Lamp = State == 0 ? Pal::Go() : State == 1 ? Pal::Caution() : Pal::NoGo();
		Rows->AddSlot().AutoWidth().Padding(0.f, 0.f, 18.f, 0.f).VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SBox).WidthOverride(16.f).HeightOverride(16.f)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Lamp)]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::Format(FText::FromString(TEXT("{0}  {1}")), State == 0 ? Txt(TEXT("MC_Go")) : State == 1 ? Txt(TEXT("MC_Caution")) : Txt(TEXT("MC_NoGo")), Text))
					.Font(Font(15, true)).ColorAndOpacity(State == 2 ? Pal::NoGo() : Pal::Text())
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
			[
				State == 0 || Tab == Fix ? SNullWidget::NullWidget
					: Button(Txt(TEXT("MC_Fix")), FOnClicked::CreateLambda([this, Fix]() { Tab = Fix; Rebuild(); return FReply::Handled(); }), true, Pal::Accent(), 12)
			]
		];
	};
	Row(FText::Format(Txt(TEXT("MC_CheckPower")), Placed, S.CellsTotal), bPower ? 0 : 2, ETab::Power);
	Row(Txt(TEXT("MC_CheckDecision")), bDecision ? 0 : 2, ETab::Decision);
	Row(Txt(TEXT("MC_CheckPlan")), bPlanned ? 0 : 1, ETab::Crew);
	if (OwnLeft > 0)
	{
		Row(FText::Format(Txt(TEXT("MC_CheckOwn")), OwnLeft), 1, ETab::Crew);
	}
	const int32 Blockers = (bPower ? 0 : 1) + (bDecision ? 0 : 1);
	const ETab FirstBlocker = !bDecision ? ETab::Decision : ETab::Power;
	return Box(SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[Rows]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			Button(bReady ? FText::Format(Txt(TEXT("Ops_Run")), SolStart, SolEnd) : FText::Format(Txt(TEXT("MC_RunNoGo")), Blockers),
				FOnClicked::CreateLambda([this, bReady, FirstBlocker]()
				{
					if (bReady)
					{
						OnRun.ExecuteIfBound();
					}
					else
					{
						// The run button is the checklist: it takes you to the first NO-GO.
						Tab = FirstBlocker;
						if (Mission.IsValid()) { Mission->PlaySfx(TEXT("UI_Locked")); }
						Rebuild();
					}
					return FReply::Handled();
				}), !M->IsRunningPhase(), bReady ? Pal::Red() : Pal::Caution(), 20)
		], Pal::Panel(), 14.f);
}

// Mission Control: tabs, power grid, decision, GO / NO-GO -----------------------------------------------------------

void STDBaseComputer::PickTab()
{
	// Open on what needs you (NMS Beyond: hints highlight the relevant menu item).
	const UTDMissionSubsystem* M = Mission.Get();
	if (!M)
	{
		return;
	}
	const FTDMissionState& S = M->GetState();
	if (!S.CurrentEventId.IsNone() && S.EventChoice < 0)
	{
		Tab = ETab::Decision;
	}
	else if (UTDMissionRules::GetCellsPlaced(S) < S.CellsTotal)
	{
		Tab = ETab::Power;
	}
	else
	{
		Tab = ETab::Crew;
	}
}

TSharedRef<SWidget> STDBaseComputer::BuildTabs()
{
	UTDMissionSubsystem* M = Mission.Get();
	const FTDMissionState& S = M->GetState();
	const bool bPowerNeeds = UTDMissionRules::GetCellsPlaced(S) < S.CellsTotal;
	const bool bDecisionNeeds = !S.CurrentEventId.IsNone() && S.EventChoice < 0;
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	auto Add = [this, &Row](ETab Which, const TCHAR* Key, bool bNeeds)
	{
		const bool bOn = Tab == Which;
		Row->AddSlot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
		[
			SNew(SButton).ButtonStyle(FlatStyle()).ButtonColorAndOpacity(bOn ? Pal::Accent() : Pal::Raised()).ContentPadding(FMargin(18.f, 10.f))
			.OnClicked(FOnClicked::CreateLambda([this, Which]()
			{
				Tab = Which;
				if (Mission.IsValid()) { Mission->PlaySfx(TEXT("UI_Hover")); }
				Rebuild();
				return FReply::Handled();
			}))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(Txt(Key)).Font(HeadFont(16)).ColorAndOpacity(bOn ? Pal::White() : Pal::Text())]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
				[
					// "Needs you" badge, blinking.
					bNeeds ? StaticCastSharedRef<SWidget>(SNew(SBox).WidthOverride(12.f).HeightOverride(12.f)
					[
						SNew(SBorder).BorderImage(White()).BorderBackgroundColor_Lambda([]()
						{
							const float B = 0.5f + 0.5f * FMath::Sin(float(FPlatformTime::Seconds()) * 6.f);
							return FSlateColor(FLinearColor::LerpUsingHSV(Pal::Red(), TDTokens::Hex(TEXT("FFB199")), B));
						})
					]) : SNullWidget::NullWidget
				]
			]
		];
	};
	Add(ETab::Power, TEXT("MC_TabPower"), bPowerNeeds);
	Add(ETab::Crew, TEXT("MC_TabCrew"), false);
	Add(ETab::Decision, TEXT("MC_TabDecision"), bDecisionNeeds);
	Add(ETab::Data, TEXT("MC_TabData"), false);
	return Row;
}

namespace
{
	/** Moving current: dots run along the line, faster with more cells (Astroneer power glow). */
	TSharedRef<SWidget> FlowLine(int32 Cells, const FLinearColor& Color)
	{
		TSharedRef<SHorizontalBox> Line = SNew(SHorizontalBox);
		for (int32 Dot = 0; Dot < 10; ++Dot)
		{
			Line->AddSlot().FillWidth(1.f).VAlign(VAlign_Center).Padding(3.f, 0.f)
			[
				SNew(SBox).HeightOverride(Cells > 0 ? 8.f : 4.f)
				[
					SNew(SBorder).BorderImage(White()).BorderBackgroundColor_Lambda([Dot, Cells, Color]()
					{
						if (Cells <= 0)
						{
							return FSlateColor(Pal::Track());
						}
						const float T = float(FPlatformTime::Seconds()) * (1.2f + 0.6f * Cells);
						const float Wave = FMath::Frac(T - Dot * 0.1f);
						FLinearColor C = Color;
						C.A = 0.25f + 0.75f * FMath::Pow(Wave, 3.f);
						return FSlateColor(C);
					})
				]
			];
		}
		return Line;
	}
}

TSharedRef<SWidget> STDBaseComputer::BuildPower()
{
	UTDMissionSubsystem* M = Mission.Get();
	const FTDMissionState& S = M->GetState();
	const int32 Placed = UTDMissionRules::GetCellsPlaced(S);
	const TArray<int32> Demand = M->GetDemand();
	const bool bCanEdit = M->IsMissionRunning() && !M->IsRunningPhase();

	// Source: every cell as a big token; free ones pulse.
	TSharedRef<SWrapBox> Tokens = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(10.f, 10.f));
	for (int32 Cell = 0; Cell < S.CellsTotal; ++Cell)
	{
		const bool bUsed = Cell < Placed;
		Tokens->AddSlot()
		[
			SNew(SBox).WidthOverride(58.f).HeightOverride(58.f)
			[
				SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Pal::Accent()).Padding(4.f)
				[
					SNew(SBorder).BorderImage(White()).Padding(0.f).BorderBackgroundColor_Lambda([bUsed, Cell]()
					{
						if (bUsed) { return FSlateColor(Pal::Line()); }
						const float B = 0.5f + 0.5f * FMath::Sin(float(FPlatformTime::Seconds()) * 4.f + Cell * 0.7f);
						return FSlateColor(FLinearColor::LerpUsingHSV(TDTokens::Hex(TEXT("F07F2E")), TDTokens::Hex(TEXT("FFC07A")), B));
					})
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(bUsed ? TEXT("") : TEXT("+"))).Font(Font(24, true)).ColorAndOpacity(Pal::Text())
					]
				]
			]
		];
	}
	const FTDOpsState& Ops = M->GetOps();
	TSharedRef<SVerticalBox> Source = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Txt(TEXT("MC_Source"))).Font(HeadFont(18)).ColorAndOpacity(Pal::Text())]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f)
		[
			SNew(STextBlock).Text(FText::Format(Txt(TEXT("MC_CellsPlaced")), Placed, S.CellsTotal)).Font(NumFont(30))
				.ColorAndOpacity(Placed < S.CellsTotal ? Pal::Red() : Pal::Go())
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)[Tokens]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 4.f)[Label(Txt(TEXT("MC_PowerHow")), 15, Pal::Text())]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[Label(Txt(TEXT("UI_PriorityRule")), 15, Pal::Text2(), true)];
	{
		int32 HeatStart = 0;
		int32 HeatEnd = 0;
		M->GetPhaseSols(S.PhaseIndex, HeatStart, HeatEnd);
		double MeanAir = 0.0;
		const double Heat = M->GetHeatingKw(HeatStart, HeatEnd, &MeanAir);
		if (Heat > 0.0)
		{
			FNumberFormattingOptions One;
			One.MaximumFractionalDigits = 1;
			Source->AddSlot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
			[
				Box(Label(FText::Format(Txt(TEXT("Heat_Phase")), FText::AsNumber(MeanAir, &One), FText::AsNumber(Heat, &One),
					FText::AsNumber(Heat / FMath::Max(0.1, M->GetConstant(TEXT("CellKw"))) * 100.0, &One)), 14, Pal::Text(), true), TDTokens::Hex(TEXT("DCEBFA")), 10.f)
			];
		}
	}
	if (Ops.SolarDustCells > 0)
	{
		Source->AddSlot().AutoHeight().Padding(0.f, 8.f)[Label(FText::Format(Txt(TEXT("Ops_Dust")), Ops.SolarDustCells), 14, Pal::Caution(), true)];
	}
	if (S.bDustPhase)
	{
		Source->AddSlot().AutoHeight().Padding(0.f, 4.f)[Label(Txt(TEXT("Ops_StormNow")), 14, Pal::NoGo(), true)];
	}

	// Loads: one card per system with its gauge, demand slots, the forecast and +/- breakers.
	TSharedRef<SVerticalBox> Loads = SNew(SVerticalBox);
	for (ETDSystem System : { ETDSystem::Air, ETDSystem::Greenhouse, ETDSystem::Shelter })
	{
		const int32 Index = static_cast<int32>(System);
		const int32 Level = S.Levels.IsValidIndex(Index) ? S.Levels[Index] : 0;
		const int32 Has = S.Allocation.IsValidIndex(Index) ? S.Allocation[Index] : 0;
		const int32 Need = Demand.IsValidIndex(Index) ? Demand[Index] : 0;
		const bool bLocked = M->IsSystemLocked(System);
		const FLinearColor SysColor = TDTokens::System(System);
		const int32 Change = UTDMissionRules::GetLevelChange(Has, Need);

		TSharedRef<SHorizontalBox> Gauge = SNew(SHorizontalBox);
		for (int32 Seg = 0; Seg < 4; ++Seg)
		{
			const bool bOn = Seg < Level;
			const bool bNext = !bLocked && ((Change > 0 && Seg == Level) || (Change < 0 && Seg == Level - 1));
			Gauge->AddSlot().AutoWidth().Padding(0.f, 0.f, 5.f, 0.f)
			[
				SNew(SBox).WidthOverride(46.f).HeightOverride(22.f)
				[
					SNew(SBorder).BorderImage(White()).BorderBackgroundColor_Lambda([bOn, bNext, Change, System, Level]()
					{
						FLinearColor C = bOn ? TDTokens::LevelColor(System, Level) : Pal::Track();
						if (bNext)
						{
							// Forecast segment blinks: green where it will rise, red where it will drop.
							const float B = 0.5f + 0.5f * FMath::Sin(float(FPlatformTime::Seconds()) * 5.f);
							C = FMath::Lerp(C, Change > 0 ? Pal::Go() : Pal::NoGo(), B);
						}
						return FSlateColor(C);
					})
				]
			];
		}
		TSharedRef<SHorizontalBox> Slots = SNew(SHorizontalBox);
		for (int32 Cell = 0; Cell < FMath::Max(Has, Need); ++Cell)
		{
			const bool bFilled = Cell < Has;
			Slots->AddSlot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SBox).WidthOverride(40.f).HeightOverride(40.f)
				[
					SNew(SBorder).BorderImage(White()).BorderBackgroundColor(bFilled ? SysColor : (Cell < Need ? Pal::NoGo() : Pal::Line())).Padding(bFilled ? 0.f : 3.f)
					[
						SNew(SBorder).BorderImage(White()).BorderBackgroundColor(bFilled ? SysColor : Pal::Raised())
					]
				]
			];
		}
		const FText Forecast = bLocked ? Txt(TEXT("UI_LockedGood"))
			: Change > 0 ? Txt(TEXT("MC_Rises")) : Change < 0 ? Txt(TEXT("MC_Drops")) : Txt(TEXT("MC_Holds"));
		const FLinearColor ForecastColor = bLocked ? Pal::Text2() : Change > 0 ? Pal::Go() : Change < 0 ? Pal::NoGo() : Pal::Text2();
		const bool bGrows = System == ETDSystem::Greenhouse && Has > 0 && !bLocked;

		Loads->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.28f).VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)[FlowLine(Has, SysColor)]
			+ SHorizontalBox::Slot().FillWidth(0.72f)
			[
				SNew(SBorder).BorderImage(White()).BorderBackgroundColor(SysColor).Padding(FMargin(8.f, 0.f, 0.f, 0.f))
				[
					Box(SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(Txt(TDTokens::SystemTextKey(System))).Font(HeadFont(22)).ColorAndOpacity(Pal::Text())]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14.f, 0.f)[Gauge]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Label(Txt(FName(*FString::Printf(TEXT("Level_%d"), Level))), 15, Pal::Text2(), true, false)]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 6.f)[Slots]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::Format(Txt(TEXT("MC_NeedHas")), Need, Has)).Font(NumFont(15)).ColorAndOpacity(Pal::Text())]
								+ SHorizontalBox::Slot().AutoWidth().Padding(16.f, 0.f)[Label(Forecast, 15, ForecastColor, true, false)]
								+ SHorizontalBox::Slot().AutoWidth()[Label(bGrows ? Txt(TEXT("UI_GrowsNextPhase")) : FText::GetEmpty(), 14, Pal::Text2(), false, false)]
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f)
						[
							Button(Txt(TEXT("MC_Remove")), FOnClicked::CreateLambda([this, System]()
							{
								if (Mission.IsValid() && Mission->RemoveCell(System)) { Mission->PlaySfxPitched(TEXT("SystemDown"), 1.3f, 0.6f); }
								Rebuild();
								return FReply::Handled();
							}), bCanEdit && Has > 0, Pal::Line(), 18)
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							Button(Txt(TEXT("MC_Add")), FOnClicked::CreateLambda([this, System]()
							{
								if (Mission.IsValid() && Mission->PlaceCell(System)) { Mission->PlaySfxPitched(TEXT("SystemUp"), 1.4f, 0.7f); }
								Rebuild();
								return FReply::Handled();
							}), bCanEdit && !bLocked && Placed < S.CellsTotal, Pal::Red(), 18)
						], Pal::Raised(), 16.f)
				]
			]
		];
	}

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.3f).Padding(0.f, 0.f, 12.f, 0.f)[Box(Source, Pal::Panel(), 20.f)]
		+ SHorizontalBox::Slot().FillWidth(0.7f)
		[
			Box(SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(STextBlock).Text(Txt(TEXT("MC_Bus"))).Font(HeadFont(18)).ColorAndOpacity(Pal::Text())]
					+ SHorizontalBox::Slot().AutoWidth()[Label(Txt(TEXT("MC_BusHint")), 14, Pal::Text2(), false, false)]
				]
				+ SVerticalBox::Slot().FillHeight(1.f)[SNew(SScrollBox) + SScrollBox::Slot()[Loads]], Pal::Panel(), 20.f)
		];
}

TSharedRef<SWidget> STDBaseComputer::BuildDecision()
{
	UTDMissionSubsystem* M = Mission.Get();
	const FTDMissionState& S = M->GetState();
	const FTDEventRow* Row = M->GetCurrentEvent();
	if (!Row)
	{
		return Box(SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Txt(TEXT("MC_TabDecision"))).Font(HeadFont(18)).ColorAndOpacity(Pal::Text())]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 20.f)[Label(Txt(TEXT("MC_NoEvent")), 22, Pal::Text2(), true)], Pal::Panel(), 24.f);
	}
	const bool bChosen = S.EventChoice >= 0;
	auto Costs = [this](int32 Air, int32 Green, int32 Shelter)
	{
		TSharedRef<SHorizontalBox> Chips = SNew(SHorizontalBox);
		const int32 Values[3] = { Air, Green, Shelter };
		const ETDSystem Systems[3] = { ETDSystem::Air, ETDSystem::Greenhouse, ETDSystem::Shelter };
		bool bAny = false;
		for (int32 I = 0; I < 3; ++I)
		{
			if (Values[I] == 0)
			{
				continue;
			}
			bAny = true;
			Chips->AddSlot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
			[
				Box(SNew(STextBlock).Text(FText::Format(Txt(TEXT("MC_CostChip")), FText::FromString(Values[I] > 0 ? FString::Printf(TEXT("+%d"), Values[I]) : FString::FromInt(Values[I])),
					Txt(TDTokens::SystemTextKey(Systems[I])))).Font(NumFont(15)).ColorAndOpacity(Pal::Text()), TDTokens::System(Systems[I]), 6.f)
			];
		}
		if (!bAny)
		{
			Chips->AddSlot().AutoWidth()[Label(Txt(TEXT("MC_NoCost")), 15, Pal::Text2(), true, false)];
		}
		return Chips;
	};
	auto Option = [&](bool bA)
	{
		const bool bThis = bChosen && (S.EventChoice == 0) == bA;
		const FName Key = bA ? Row->ChoiceAKey : Row->ChoiceBKey;
		return SNew(SBorder).BorderImage(White()).BorderBackgroundColor(bThis ? Pal::Go() : Pal::Line()).Padding(3.f)
		[
			Box(SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(bA ? TEXT("A") : TEXT("B"))).Font(HeadFont(28)).ColorAndOpacity(bThis ? Pal::Go() : Pal::Red())]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)[Label(Txt(Key), 20, bChosen && !bThis ? Pal::Text2() : Pal::Text(), true)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f)[Label(Txt(TEXT("MC_CostTitle")), 13, Pal::Text2(), true, false)]
				+ SVerticalBox::Slot().AutoHeight()[bA ? Costs(Row->ChoiceADemandAir, Row->ChoiceADemandGreenhouse, Row->ChoiceADemandShelter)
					: Costs(Row->ChoiceBDemandAir, Row->ChoiceBDemandGreenhouse, Row->ChoiceBDemandShelter)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f).HAlign(HAlign_Left)
				[
					bChosen ? StaticCastSharedRef<SWidget>(Label(bThis ? Txt(TEXT("MC_Chosen")) : FText::GetEmpty(), 16, Pal::Go(), true, false))
						: Button(Txt(TEXT("MC_Choose")), FOnClicked::CreateLambda([this, bA]()
						{
							if (Mission.IsValid()) { Mission->ChooseEventOption(bA); }
							Rebuild();
							return FReply::Handled();
						}), true, Pal::Red(), 18)
				], Pal::Raised(), 18.f)
		];
	};

	TSharedRef<SVerticalBox> Body = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(STextBlock).Text(Txt(TEXT("MC_Incoming"))).Font(HeadFont(18)).ColorAndOpacity(Pal::Red())]
			+ SHorizontalBox::Slot().AutoWidth()[Label(Txt(TEXT("UI_PriorityRule")), 15, Pal::Text2(), true, false)]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 18.f)[Label(M->GetBandText(Row->QuestionKey), 26, Pal::Text(), true)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 14.f)
		[
			// Radiation cards: what each choice means in real RAD numbers (event day, 11 Sep 2017).
			Row->Family == ETDEventFamily::Radiation
				? StaticCastSharedRef<SWidget>(Box(Label(FText::Format(Txt(TEXT("Dose_Forecast")),
					FText::AsNumber(FMath::RoundToInt(M->ForecastEventDayDose(true))), FText::AsNumber(FMath::RoundToInt(M->ForecastEventDayDose(false))),
					FText::AsNumber(M->GetConstant(TEXT("EvaHoursPerEvaSol")))), 16, Pal::Text(), true), TDTokens::Hex(TEXT("FCE9C8")), 12.f))
				: SNullWidget::NullWidget
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 8.f, 0.f)[Option(true)]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f, 0.f, 0.f)[Option(false)]
		];
	if (bChosen)
	{
		// The back of the card: what really happened, with its date, instrument, label and source.
		TArray<FString> Sources;
		for (const FName& SourceId : Row->SourceIds)
		{
			const FTDSourceRow* Source = M->GetSource(SourceId);
			Sources.Add(Source ? Source->Title : SourceId.ToString());
		}
		const FText LabelText = Txt(FName(TEXT("Label_") + StaticEnum<ETDCardLabel>()->GetNameStringByValue(static_cast<int64>(Row->Label))));
		Body->AddSlot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)
		[
			Box(SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Txt(TEXT("UI_WhatReallyHappened"))).Font(HeadFont(16)).ColorAndOpacity(Pal::Red())]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f)[Label(M->GetBandText(Row->RevealKey), 18, Pal::Text())]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::Format(FText::FromString(TEXT("{0} · {1} · {2}")), FText::FromString(Row->EventDate),
					FText::FromString(Row->Instrument), LabelText)).Font(NumFont(13)).ColorAndOpacity(Pal::Text2())]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[Label(Txt(Row->MeasurementKey), 14, Pal::Text2())]
				+ SVerticalBox::Slot().AutoHeight()[Label(FText::Format(Txt(TEXT("UI_SourceLine")), FText::FromString(FString::Join(Sources, TEXT("; ")))), 13, Pal::Text2())],
				Pal::Raised(), 18.f)
		];
	}
	return Box(SNew(SScrollBox) + SScrollBox::Slot()[Body], Pal::Panel(), 24.f);
}

TSharedRef<SWidget> STDBaseComputer::BuildData()
{
	UTDMissionSubsystem* M = Mission.Get();

	// Today's Sun: LIVE only with this session's fetch time; otherwise ARCHIVE (never stored data shown as live).
	TSharedRef<SVerticalBox> Sun = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Txt(TEXT("Sun_Title"))).Font(HeadFont(18)).ColorAndOpacity(Pal::Text())];
	if (M->HasSpaceWeather())
	{
		const FTDSpaceWeather& W = M->GetSpaceWeather();
		const bool bLive = W.Status == ETDDataStatus::Live;
		Sun->AddSlot().AutoHeight().Padding(0.f, 10.f).HAlign(HAlign_Left)
		[
			Box(SNew(STextBlock).Text(Txt(bLive ? TEXT("UI_Live") : TEXT("UI_Archive"))).Font(HeadFont(16)).ColorAndOpacity(Pal::White()), bLive ? Pal::Go() : Pal::Text2(), 6.f)
		];
		Sun->AddSlot().AutoHeight()[Label(bLive ? FText::Format(Txt(TEXT("Sun_LiveStatus")), FText::FromString(W.FetchedAtUTC.ToString(TEXT("%Y-%m-%d %H:%M"))))
			: Txt(TEXT("Sun_ArchiveStatus")), 15, Pal::Text(), true)];
		Sun->AddSlot().AutoHeight().Padding(0.f, 8.f)
		[
			Label(FText::Format(Txt(bLive ? TEXT("Sun_SummaryLive") : TEXT("Sun_SummaryArchive")), FText::AsNumber(W.FlareCount),
				FText::FromString(W.StrongestClass.IsEmpty() ? TEXT("-") : W.StrongestClass), FText::AsNumber(W.CMECount), FText::AsNumber(W.StormCount)), 16, Pal::Text())
		];
		int32 Shown = 0;
		for (const FTDDonkiRow& Row : W.Rows)
		{
			if (Row.EventType == TEXT("CME") || Shown >= 8)
			{
				continue;
			}
			++Shown;
			Sun->AddSlot().AutoHeight().Padding(0.f, 2.f)
			[
				SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%s  %s  %s"), *Row.EventType, *Row.TimeUTC, *Row.ClassType))).Font(NumFont(13)).ColorAndOpacity(Pal::Text())
			];
		}
	}
	else
	{
		Sun->AddSlot().AutoHeight().Padding(0.f, 10.f)[Label(Txt(TEXT("MC_SunWaiting")), 15, Pal::Text2(), true)];
	}
	Sun->AddSlot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)[Label(Txt(TEXT("UI_DonkiDisclaimer")), 13, Pal::Text2())];

	// Crew dose (Curiosity RAD) and the InSight records the crew studied.
	Sun->AddSlot().AutoHeight().Padding(0.f, 22.f, 0.f, 6.f)[SNew(STextBlock).Text(Txt(TEXT("Dose_Title"))).Font(HeadFont(18)).ColorAndOpacity(Pal::Text())];
	Sun->AddSlot().AutoHeight().Padding(0.f, 2.f, 0.f, 6.f)[Label(Txt(TEXT("Dose_How")), 13, Pal::Text2())];
	for (int32 Crew = 0; Crew < FMath::Max(4, M->GetOps().Crew.Num()); ++Crew)
	{
		Sun->AddSlot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)[SNew(SBox).WidthOverride(70.f)[Label(M->GetCrewName(Crew), 14, Pal::Text(), true, false)]]
			+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(STextBlock).Text(M->GetDoseLine(Crew)).Font(NumFont(12)).ColorAndOpacity(Pal::Text()).AutoWrapText(true)]
		];
	}
	Sun->AddSlot().AutoHeight().Padding(0.f, 22.f, 0.f, 6.f)[SNew(STextBlock).Text(Txt(TEXT("Quake_Title"))).Font(HeadFont(18)).ColorAndOpacity(Pal::Text())];
	if (M->GetState().QuakeRecordsStudied <= 0)
	{
		Sun->AddSlot().AutoHeight()[Label(Txt(TEXT("Quake_None")), 13, Pal::Text2())];
	}
	for (int32 Index = 0; Index < M->GetState().QuakeRecordsStudied; ++Index)
	{
		Sun->AddSlot().AutoHeight().Padding(0.f, 3.f)[Label(M->GetQuakeRecordText(Index), 13, Pal::Text())];
	}
	{
		FNumberFormattingOptions One;
		One.MaximumFractionalDigits = 1;
		Sun->AddSlot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)
		[
			Label(FText::Format(Txt(TEXT("Light_Line")), FText::AsNumber(M->GetConstant(TEXT("LightTimeNow")), &One)), 14, Pal::Text(), true)
		];
	}

	// Where every number comes from (the Source Panel).
	TSharedRef<SVerticalBox> Sources = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Txt(TEXT("UI_WhereFrom"))).Font(HeadFont(18)).ColorAndOpacity(Pal::Text())]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 2.f)[Label(Txt(TEXT("UI_NotEndorsed")), 15, Pal::Red(), true)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 10.f)[Label(Txt(TEXT("UI_TimeCompression")), 14, Pal::Text())];
	if (const UDataTable* Constants = M->GetConstantsTable())
	{
		for (const TPair<FName, uint8*>& Pair : Constants->GetRowMap())
		{
			const FTDConstantRow& Row = *reinterpret_cast<const FTDConstantRow*>(Pair.Value);
			FString Line = FString::Printf(TEXT("%s: %s %s"), *Pair.Key.ToString(), *FString::SanitizeFloat(Row.Value, 0), *Row.Unit);
			FLinearColor Ink = Pal::Text2();
			if (Row.bIsDesignerValue)
			{
				Line += TEXT("  ·  ") + Txt(TEXT("UI_DesignerValue")).ToString();
			}
			else if (const FTDSourceRow* Source = M->GetSource(Row.SourceId))
			{
				Ink = Pal::Text();
				Line += FString::Printf(TEXT("  ·  %s (%s)  %s  [%s, %s]"), *Source->Title, *Row.SourceDetail, *Source->URL,
					*Txt(TEXT("UI_NasaMeasurement")).ToString(), *FText::Format(Txt(TEXT("UI_Retrieved")), FText::FromString(Source->RetrievedOn)).ToString());
			}
			Sources->AddSlot().AutoHeight().Padding(0.f, 2.f)[Label(FText::FromString(Line), 13, Ink)];
		}
	}

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.34f).Padding(0.f, 0.f, 12.f, 0.f)[Box(SNew(SScrollBox) + SScrollBox::Slot()[Sun], Pal::Panel(), 20.f)]
		+ SHorizontalBox::Slot().FillWidth(0.66f)[Box(SNew(SScrollBox) + SScrollBox::Slot()[Sources], Pal::Panel(), 20.f)];
}
