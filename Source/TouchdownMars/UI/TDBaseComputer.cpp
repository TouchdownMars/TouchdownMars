// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "UI/TDBaseComputer.h"

#include "Engine/DataTable.h"
#include "Fonts/CompositeFont.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Systems/TDMissionSubsystem.h"
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
	if (InKeyEvent.GetKey() == EKeys::B || InKeyEvent.GetKey() == EKeys::Escape)
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
	Root->SetContent(SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[BuildHeader()]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f)[BuildCrew()]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.72f).Padding(0.f, 0.f, 12.f, 0.f)[BuildBoard()]
			+ SHorizontalBox::Slot().FillWidth(0.28f)[BuildSide()]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)[BuildFooter()]);
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
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f)
		[
			Button(Txt(TEXT("Ops_Auto")), FOnClicked::CreateLambda([this]() { if (Mission.IsValid()) { Mission->AutoPlanTasks(); } return FReply::Handled(); }),
				!M->IsRunningPhase(), Pal::Accent())
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f)
		[
			Button(Txt(TEXT("Ops_Clear")), FOnClicked::CreateLambda([this]() { if (Mission.IsValid()) { Mission->ClearPlan(); } return FReply::Handled(); }),
				!M->IsRunningPhase(), Pal::Line())
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
	int32 SolStart = 0;
	int32 SolEnd = 0;
	M->GetPhaseSols(M->GetState().PhaseIndex, SolStart, SolEnd);
	const bool bReady = M->CanEndPhase() && !M->IsRunningPhase();
	return Box(SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			Label(bReady ? Txt(TEXT("Ops_RunHint")) : Txt(TEXT("Ops_RunBlocked")), 15, bReady ? Pal::Text() : Pal::Caution(), true)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			Button(FText::Format(Txt(TEXT("Ops_Run")), SolStart, SolEnd), FOnClicked::CreateLambda([this]()
			{
				OnRun.ExecuteIfBound();
				return FReply::Handled();
			}), bReady, Pal::Red(), 20)
		], Pal::Panel(), 14.f);
}
