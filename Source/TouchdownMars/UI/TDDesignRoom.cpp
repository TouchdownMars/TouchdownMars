// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "UI/TDDesignRoom.h"

#include "Engine/DataTable.h"
#include "Brushes/SlateColorBrush.h"
#include "Styling/CoreStyle.h"
#include "Systems/TDCutsceneSubsystem.h"
#include "Systems/TDMissionSubsystem.h"
#include "UI/TDDesignTokens.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Engine/Texture2D.h"
#include "Engine/FontFace.h"
#include "Fonts/CompositeFont.h"
#include "Misc/Paths.h"

namespace
{
	const FSlateBrush* White() { return FCoreStyle::Get().GetBrush("GenericWhiteBox"); }

	/** Design-room palette (NASA-punk: warm light panels, charcoal, one NASA red). */
	namespace Pal
	{
		inline FLinearColor Hex(const TCHAR* V, float A = 1.f) { FLinearColor C = FLinearColor(FColor::FromHex(V)); C.A = A; return C; }
		inline FLinearColor Dim() { return Hex(TEXT("080A0D"), 0.55f); }       // scene dim behind the panels
		inline FLinearColor Bg() { return Hex(TEXT("E2DFD8")); }               // chips, wells
		inline FLinearColor Panel() { return Hex(TEXT("EFEDE8"), 0.97f); }     // panels
		inline FLinearColor Raised() { return Hex(TEXT("FAF9F6")); }           // cards, guide strip
		inline FLinearColor Line() { return Hex(TEXT("C8C4BA")); }             // rules, idle borders, secondary buttons
		inline FLinearColor Track() { return Hex(TEXT("D9D5CC")); }            // empty bar
		inline FLinearColor Text() { return Hex(TEXT("15181C")); }             // main text
		inline FLinearColor Text2() { return Hex(TEXT("5C636E")); }            // secondary text
		inline FLinearColor Accent() { return Hex(TEXT("22272E")); }           // ordinary buttons (charcoal)
		inline FLinearColor Red() { return Hex(TEXT("E03C31")); }              // NASA red: primary actions, guide highlight
		inline FLinearColor Fill() { return Hex(TEXT("3A4049")); }
		inline FLinearColor Go() { return Hex(TEXT("1E8E4E")); }
		inline FLinearColor Caution() { return Hex(TEXT("B7791F")); }
		inline FLinearColor NoGo() { return Hex(TEXT("D7261E")); }
		inline FLinearColor Ink() { return Hex(TEXT("15181C")); }
		inline FLinearColor White() { return Hex(TEXT("FFFFFF")); }
	}

	/** Team UI font: Barlow Semi Condensed + IBM Plex Mono (SIL OFL 1.1), raw TTF files in Content/TouchdownMars/UI/Fonts
	 *  (staged as non-UFS); engine font if missing. */
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

	/** Industrial heading: semibold with wide tracking (no case conversion, so Turkish İ/I stays correct). */
	FSlateFontInfo HeadFont(int32 Size)
	{
		FSlateFontInfo Info = Font(Size, true);
		Info.LetterSpacing = 90;
		return Info;
	}

	/** Numbers: IBM Plex Mono (fixed-width digits, NASA-STD-3001 App. F). */
	FSlateFontInfo NumFont(int32 Size)
	{
		if (TSharedPtr<const FCompositeFont> Team = TeamFont())
		{
			return FSlateFontInfo(Team, Size, FName(TEXT("Mono")));
		}
		return FCoreStyle::GetDefaultFontStyle("Mono", Size);
	}

	/** UI picture brush (Content/TouchdownMars/UI/Art/<Name>), or null. */
	const FSlateBrush* ArtBrush(const FString& Name)
	{
		static TMap<FString, TSharedPtr<FSlateBrush>> Cache;
		if (TSharedPtr<FSlateBrush>* Found = Cache.Find(Name))
		{
			return Found->IsValid() ? Found->Get() : nullptr;
		}
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/TouchdownMars/UI/Art/%s.%s"), *Name, *Name));
		TSharedPtr<FSlateBrush> Brush;
		if (Texture)
		{
			Texture->AddToRoot();
			Brush = MakeShared<FSlateBrush>();
			Brush->SetResourceObject(Texture);
			Brush->ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
			Brush->DrawAs = ESlateBrushDrawType::Image;
		}
		Cache.Add(Name, Brush);
		return Brush.IsValid() ? Brush.Get() : nullptr;
	}

	/** Option picture T_TD_Opt_<OptionId> (Gemini illustration in the shared card style; credit tag on the card). */
	const FSlateBrush* OptionArt(FName OptionId) { return ArtBrush(FString::Printf(TEXT("T_TD_Opt_%s"), *OptionId.ToString())); }

	/** Bar that eases toward its value; the displayed value survives screen rebuilds (keyed by Id). */
	class STDAnimBar : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(STDAnimBar) : _Ratio(0.f), _Fill(FLinearColor::White), _Marker(-1.f) {}
			SLATE_ARGUMENT(FName, Id)
			SLATE_ARGUMENT(float, Ratio)
			SLATE_ARGUMENT(FLinearColor, Fill)
			SLATE_ARGUMENT(float, Marker)
		SLATE_END_ARGS()

		static TMap<FName, float>& Shown() { static TMap<FName, float> Map; return Map; }

		void Construct(const FArguments& InArgs)
		{
			Id = InArgs._Id;
			Target = InArgs._Ratio;
			Fill = InArgs._Fill;
			Marker = InArgs._Marker;
			if (!Shown().Contains(Id))
			{
				Shown().Add(Id, Target);
			}
		}

		virtual void Tick(const FGeometry& G, const double Now, const float Dt) override
		{
			float& Value = Shown().FindOrAdd(Id);
			Value += (Target - Value) * FMath::Min(1.f, Dt * 7.f); // about 300 ms to settle
		}

		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(100.f, 12.f); }

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer,
			const FWidgetStyle& Style, bool bParentEnabled) const override
		{
			const FVector2D Size = G.GetLocalSize();
			const float* ValuePtr = Shown().Find(Id);
			const float Value = FMath::Clamp(ValuePtr ? *ValuePtr : Target, 0.f, 1.f);
			FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(Size, FSlateLayoutTransform()), White(), ESlateDrawEffect::None, Pal::Track());
			FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(FVector2D(Size.X * Value, Size.Y), FSlateLayoutTransform()), White(), ESlateDrawEffect::None, Fill);
			if (Marker >= 0.f && Marker <= 1.f)
			{
				FSlateDrawElement::MakeBox(Out, Layer + 2, G.ToPaintGeometry(FVector2D(3.f, Size.Y + 6.f), FSlateLayoutTransform(FVector2D(Size.X * Marker - 1.5f, -3.f))),
					White(), ESlateDrawEffect::None, Pal::Ink());
			}
			return Layer + 3;
		}

	private:
		FName Id;
		float Target = 0.f;
		FLinearColor Fill;
		float Marker = -1.f;
	};

	/** Pulsing NASA-red outline (0.8 Hz, NASA-STD-3001 App. F) that points at the next thing to do. */
	TSharedRef<SWidget> Glow(const TSharedRef<SWidget>& Content, bool bOn)
	{
		if (!bOn)
		{
			return Content;
		}
		return SNew(SBorder).BorderImage(White()).Padding(3.f)
			.BorderBackgroundColor(TAttribute<FSlateColor>::CreateLambda([]()
			{
				const float A = 0.45f + 0.55f * (0.5f + 0.5f * FMath::Sin(FPlatformTime::Seconds() * 2.0 * PI * 0.8));
				FLinearColor C = Pal::Red();
				C.A = A;
				return FSlateColor(C);
			}))
			[
				// Outline only: the content keeps its panel behind it.
				SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Pal::Panel()).Padding(4.f)[Content]
			];
	}

	FLinearColor Panel() { return Pal::Panel(); }
	FLinearColor PanelDark() { return Pal::Raised(); }
	FLinearColor Muted() { return Pal::Text2(); }

	TSharedRef<STextBlock> Label(const FText& Text, int32 Size, const FLinearColor& Color, bool bBold = false, bool bWrap = true)
	{
		return SNew(STextBlock).Text(Text).Font(Font(Size, bBold)).ColorAndOpacity(Color).AutoWrapText(bWrap);
	}

	TSharedRef<SWidget> Box(const TSharedRef<SWidget>& Content, const FLinearColor& Color, float Pad = 14.f)
	{
		return SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Color).Padding(Pad)[Content];
	}

	/** Flat button: the tint is the exact fill color (white brushes), darker on hover and press. */
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

	TSharedRef<SWidget> Button(const FText& Text, FOnClicked OnClicked, bool bEnabled, const FLinearColor& Color, int32 Size = 16)
	{
		return SNew(SButton)
			.ButtonStyle(FlatStyle())
			.IsEnabled(bEnabled)
			.ButtonColorAndOpacity(bEnabled ? Color : Pal::Track())
			.ContentPadding(FMargin(16.f, 8.f))
			.HAlign(HAlign_Center)
			.OnClicked(OnClicked)
			[
				SNew(STextBlock).Text(Text).Font(Font(Size, true))
					.ColorAndOpacity(!bEnabled ? Pal::Text2() : Color.GetLuminance() < 0.45f ? Pal::White() : Pal::Ink())
			];
	}

	/** Flat bar: dark track, solid fill (the default progress bar style is too faint on the panels). */
	TSharedRef<SWidget> Bar(float Ratio, const FLinearColor& Fill)
	{
		return SNew(SBorder).BorderImage(White()).BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.55f)).Padding(0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(FMath::Max(0.001f, Ratio))[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Fill)]
			+ SHorizontalBox::Slot().FillWidth(FMath::Max(0.001f, 1.f - Ratio))[SNullWidget::NullWidget]
		];
	}

	TSharedRef<SWidget> Chip(const FText& Text, const FLinearColor& Color)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(4.f)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Color)]]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Pal::Bg()).Padding(FMargin(6.f, 2.f))
				[
					SNew(STextBlock).Text(Text).Font(NumFont(12)).ColorAndOpacity(Pal::Text())
				]
			];
	}

	FText Num(double Value, int32 Digits = 0)
	{
		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = Digits;
		Options.MaximumFractionalDigits = Digits;
		return FText::AsNumber(Value, &Options);
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

	ETDRole OwnerOf(ETDDesignSlot Slot)
	{
		switch (Slot)
		{
		case ETDDesignSlot::Launcher: return ETDRole::Planner;
		case ETDDesignSlot::Instrument: return ETDRole::Science;
		case ETDDesignSlot::Comms: return ETDRole::Comms;
		default: return ETDRole::Engineer;
		}
	}

	constexpr double TransitSeconds = 8.0;
	constexpr double StormPreviewSeconds = 6.5;
	constexpr double PollStepSeconds = 0.75;

	/** One power cell token: the same square in the dashboard, the storm preview and the cold open. */
	TSharedRef<SWidget> Token(const TAttribute<FSlateColor>& Color, float Size = 18.f)
	{
		return SNew(SBox).WidthOverride(Size).HeightOverride(Size)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Color)];
	}

	/** Colour of token Index when Shown cells are on and Need are required. */
	FLinearColor TokenColor(int32 Index, float Shown, int32 Need, const FLinearColor& On)
	{
		if (Index < FMath::FloorToInt(Shown + 0.001f))
		{
			return On;
		}
		if (Index < Need)
		{
			FLinearColor Missing = Pal::NoGo();
			Missing.A = 0.28f;
			return Missing;
		}
		return Pal::Track();
	}

	/** A row of cell tokens with a charcoal marker after the required count. Shown is a function so it can animate. */
	TSharedRef<SWidget> TokenRow(TFunction<float()> Shown, int32 Need, int32 Slots, const FLinearColor& On, float Size = 18.f)
	{
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		for (int32 Index = 0; Index < Slots; ++Index)
		{
			if (Index == Need)
			{
				Row->AddSlot().AutoWidth().Padding(2.f, -3.f)[SNew(SBox).WidthOverride(3.f).HeightOverride(Size + 6.f)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Pal::Accent())]];
			}
			Row->AddSlot().AutoWidth().Padding(2.f, 0.f)
			[
				Token(TAttribute<FSlateColor>::CreateLambda([Shown, Index, Need, On]() { return FSlateColor(TokenColor(Index, Shown(), Need, On)); }), Size)
			];
		}
		return Row;
	}

	FLinearColor ChainColor(FName Node)
	{
		static const TSet<FName> Science = { TEXT("Objective"), TEXT("Instrument"), TEXT("Data"), TEXT("Link") };
		static const TSet<FName> Cargo = { TEXT("Payload"), TEXT("Mass"), TEXT("Rockets"), TEXT("Budget") };
		static const TSet<FName> Power = { TEXT("Power"), TEXT("Cells"), TEXT("Dust") };
		if (Science.Contains(Node)) return TDTokens::Hex(TEXT("3A8DDE"));
		if (Cargo.Contains(Node)) return TDTokens::Hex(TEXT("B58CF0"));
		if (Power.Contains(Node)) return TDTokens::Hex(TEXT("F07F2E"));
		return Pal::Red();
	}

	ETDDesignSlot DecisionSlot(FName Decision)
	{
		if (Decision == TEXT("Power")) return ETDDesignSlot::Power;
		if (Decision == TEXT("Shelter")) return ETDDesignSlot::Shelter;
		if (Decision == TEXT("Link")) return ETDDesignSlot::Comms;
		if (Decision == TEXT("Rockets")) return ETDDesignSlot::Launcher;
		return ETDDesignSlot::Instrument;
	}
}

void STDDesignRoom::Construct(const FArguments& InArgs)
{
	Mission = InArgs._Mission;
	OnLaunched = InArgs._OnLaunched;
	OnTransitDone = InArgs._OnTransitDone;
	// A fixed 1920 x 1080 canvas scaled to the window, so text wraps and cards flow the same on every screen.
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
	Rebuild();
}

void STDDesignRoom::Show(ETDDesignScreen InScreen)
{
	Screen = InScreen;
	if (Screen == ETDDesignScreen::Transit)
	{
		TransitStart = FPlatformTime::Seconds();
		bTransitDone = false;
	}
	if (Screen == ETDDesignScreen::Design)
	{
		ActiveRole = ETDRole::Planner;
		StormPreviewStart = -1.0;
		PollStart = -1.0;
		if (Mission.IsValid() && Mission->FirstFlight.FocusSlot >= 0)
		{
			// Redesign after the debrief: open the tab that owns the decision the player picked.
			ActiveRole = OwnerOf(static_cast<ETDDesignSlot>(Mission->FirstFlight.FocusSlot));
		}
	}
	if (Screen == ETDDesignScreen::Debrief && Mission.IsValid() && DebriefFlight != Mission->GetFlights().Num())
	{
		DebriefFlight = Mission->GetFlights().Num();
		DebriefPick = NAME_None;
	}
	Rebuild();
}

void STDDesignRoom::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (Screen == ETDDesignScreen::Transit && !bTransitDone && FPlatformTime::Seconds() - TransitStart > TransitSeconds)
	{
		bTransitDone = true;
		OnTransitDone.ExecuteIfBound();
	}
	if (Screen == ETDDesignScreen::Design && Mission.IsValid())
	{
		const double Now = FPlatformTime::Seconds();
		if (StormPreviewStart > 0.0 && Now - StormPreviewStart > StormPreviewSeconds)
		{
			// The storm preview ran the player's own design through the dust: name the cause, then unlock the answer.
			StormPreviewStart = -1.0;
			const FTDDesignBudget B = Mission->GetDesignBudget();
			const int32 MinCells = FMath::RoundToInt(Mission->GetConstant(TEXT("MinCells")));
			const bool bHolds = B.DustCells >= MinCells;
			Mission->LogOnboarding(TEXT("StormPreview"), FString::Printf(TEXT("guess=%d dust=%d need=%d"), Mission->FirstFlight.StormGuess, B.DustCells, MinCells));
			if (!bHolds)
			{
				Mission->FirstFlight.bStormFailed = true;
				Mission->FirstFlight.bFissionUnlocked = true;
				SetBeat(EBeat::Storm, TEXT("Flight_StormFail"));
			}
			else
			{
				SetBeat(EBeat::Poll, TEXT("Flight_StormPass"));
			}
			Rebuild();
		}
		if (PollStart > 0.0 && Now - PollStart > PollStepSeconds * (TD::NumRoles + 0.6))
		{
			PollStart = -1.0;
			Mission->SetAllRolesGo();
			Mission->LogOnboarding(TEXT("FirstGo"));
			Rebuild();
		}
	}
}

void STDDesignRoom::Rebuild()
{
	if (!Root.IsValid() || !Mission.IsValid())
	{
		return;
	}
	switch (Screen)
	{
	case ETDDesignScreen::Objectives:
		Root->SetContent(Mission->IsFirstFlight() && !Mission->FirstFlight.bColdOpenSeen ? BuildColdOpen() : BuildObjectives());
		break;
	case ETDDesignScreen::Design:
		AdvanceBeats(Mission->GetDesignBudget());
		Root->SetContent(BuildDesign());
		break;
	case ETDDesignScreen::Transit: Root->SetContent(BuildTransit()); break;
	case ETDDesignScreen::Debrief: Root->SetContent(BuildDebrief()); break;
	}
}

FText STDDesignRoom::Txt(const TCHAR* Key) const
{
	return Mission.IsValid() ? Mission->GetText(FName(Key)) : FText::FromString(Key);
}

FText STDDesignRoom::Txt(FName Key) const
{
	return Mission.IsValid() ? Mission->GetText(Key) : FText::FromName(Key);
}

FText STDDesignRoom::RoleName(ETDRole Role) const
{
	return Txt(RoleKey(Role));
}

FText STDDesignRoom::SlotName(ETDDesignSlot Slot) const
{
	return Txt(FName(*FString::Printf(TEXT("Slot_%s"), *StaticEnum<ETDDesignSlot>()->GetNameStringByValue(static_cast<int64>(Slot)))));
}

bool STDDesignRoom::IsNasaValue(FName ConstantKey) const
{
	const FTDConstantRow* Row = Mission.IsValid() ? Mission->GetConstantRow(ConstantKey) : nullptr;
	return Row && !Row->bIsDesignerValue;
}

FText STDDesignRoom::IssueText(const FTDDesignIssueLine& Issue) const
{
	const FString Name = StaticEnum<ETDDesignIssue>()->GetNameStringByValue(static_cast<int64>(Issue.Issue));
	if (Issue.Issue == ETDDesignIssue::MissingSlot)
	{
		return FText::Format(Txt(TEXT("Design_Issue_MissingSlot")), SlotName(Issue.Slot));
	}
	if (Issue.Issue == ETDDesignIssue::RoleGoalMissed)
	{
		return Txt(FName(*FString::Printf(TEXT("Goal_%s_Missed"), *StaticEnum<ETDRole>()->GetNameStringByValue(static_cast<int64>(Issue.Role)))));
	}
	const bool bTonnes = Issue.Issue == ETDDesignIssue::OverMass || Issue.Issue == ETDDesignIssue::ItemTooBig;
	const int32 Digits = bTonnes ? 1 : 0;
	FText Third = Num(Issue.ValueC, Digits);
	if (Issue.Issue == ETDDesignIssue::ItemTooBig)
	{
		const FTDDesignOptionRow* Row = Mission->GetOptionsTable() ? Mission->GetOptionsTable()->FindRow<FTDDesignOptionRow>(Issue.OptionId, TEXT("TDDesignRoom"), false) : nullptr;
		Third = Row ? Txt(Row->NameKey) : Txt(FName(*FString::Printf(TEXT("Opt_%s"), *Issue.OptionId.ToString())));
	}
	return FText::Format(Txt(FName(*FString::Printf(TEXT("Design_Issue_%s"), *Name))), Num(Issue.ValueA, Digits), Num(Issue.ValueB, Digits), Third);
}

FText STDDesignRoom::StatsLine(const FTDDesignOptionRow& Row) const
{
	const UTDMissionSubsystem* M = Mission.Get();
	TArray<FString> Parts;
	auto Add = [&Parts](const FText& Text) { Parts.Add(Text.ToString()); };
	if (!Row.CapacityKey.IsNone())
	{
		Add(FText::Format(Txt(TEXT("Stat_Capacity")), Num(M->GetConstant(Row.CapacityKey) / 1000.0, 2)));
	}
	if (!Row.MassKey.IsNone())
	{
		const double Kg = M->GetConstant(Row.MassKey);
		Add(Kg >= 100.0 ? FText::Format(Txt(TEXT("Stat_MassT")), Num(Kg / 1000.0, 1)) : FText::Format(Txt(TEXT("Stat_MassKg")), Num(Kg, 1)));
	}
	if (!Row.PowerOutKey.IsNone())
	{
		Add(FText::Format(Txt(TEXT("Stat_PowerOut")), Num(M->GetConstant(Row.PowerOutKey), 0)));
	}
	if (!Row.PowerDrawKey.IsNone())
	{
		const double Kw = M->GetConstant(Row.PowerDrawKey);
		Add(Kw >= 1.0 ? FText::Format(Txt(TEXT("Stat_PowerDrawKw")), Num(Kw, 1)) : FText::Format(Txt(TEXT("Stat_PowerDrawW")), Num(Kw * 1000.0, 1)));
	}
	if (!Row.CostKey.IsNone())
	{
		Add(FText::Format(Txt(TEXT("Stat_Cost")), Num(M->GetConstant(Row.CostKey), 0)));
	}
	if (!Row.DataKey.IsNone())
	{
		Add(FText::Format(Txt(TEXT("Stat_Data")), Num(M->GetConstant(Row.DataKey), 0)));
	}
	if (!Row.ScienceKey.IsNone())
	{
		Add(FText::Format(Txt(TEXT("Stat_Science")), Num(M->GetConstant(Row.ScienceKey), 0)));
	}
	return FText::FromString(FString::Join(Parts, TEXT("  ·  ")));
}

// Objectives

TSharedRef<SWidget> STDDesignRoom::BuildObjectives()
{
	UTDMissionSubsystem* M = Mission.Get();
	TSharedRef<SWrapBox> Cards = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(16.f, 16.f));
	if (const UDataTable* Objectives = M->GetObjectivesTable())
	{
		Objectives->ForeachRow<FTDObjectiveRow>(TEXT("TDDesignRoom"), [this, M, &Cards](const FName& Key, const FTDObjectiveRow& Row)
		{
			TSharedRef<SVerticalBox> Requirements = SNew(SVerticalBox);
			for (const FName& Req : Row.RequirementKeys)
			{
				Requirements->AddSlot().AutoHeight().Padding(0.f, 2.f)[Label(FText::Format(FText::FromString(TEXT("• {0}")), Txt(Req)), 14, Pal::Text())];
			}
			const FText Budget = Row.bSandbox ? Txt(TEXT("Obj_NoBudget"))
				: FText::Format(Txt(TEXT("Obj_Budget")), Num(M->GetConstant(Row.BudgetKey), 0));
			const FText Weights = FText::Format(Txt(TEXT("Obj_Weights")), Row.WeightSuccess, Row.WeightScience, Row.WeightEfficiency, Row.WeightDecisions);
			const FName Id = Key;
			const bool bLocked = M->IsFirstFlight() && Key != FName(TEXT("FirstLight"));
			const bool bNew = !M->FirstFlight.bSkipped && M->GetFlights().Num() == 1 && Key != FName(TEXT("FirstLight"));
			Cards->AddSlot()
			[
				SNew(SBox).WidthOverride(400.f)
				[
					Box(SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
						[
							ArtBrush(FString::Printf(TEXT("T_TD_Obj_%s"), *Id.ToString()))
								? StaticCastSharedRef<SWidget>(SNew(SBox).HeightOverride(200.f)
								[
									SNew(SOverlay)
									+ SOverlay::Slot()[SNew(SImage).Image(ArtBrush(FString::Printf(TEXT("T_TD_Obj_%s"), *Id.ToString())))]
									+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(4.f)
									[
										Box(Label(FText::FromString(Row.ImageCredit), 9, Pal::White(), false, false), FLinearColor(0.f, 0.f, 0.f, 0.55f), 3.f)
									]
								])
								: SNullWidget::NullWidget
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							Key == FName(TEXT("FirstLight"))
								? Box(Label(Txt(TEXT("Obj_FirstFlight")), 12, Pal::White(), true, false), Pal::Go(), 4.f)
								: bNew ? Box(Label(Txt(TEXT("Lock_New")), 12, Pal::White(), true, false), Pal::Red(), 4.f)
								: SNullWidget::NullWidget
						]
						+ SVerticalBox::Slot().AutoHeight()[Label(Txt(Row.NameKey), 26, Pal::Accent(), true)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)[Label(Txt(Row.InfoKey), 15, Pal::Text())]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 2.f)[Label(Txt(TEXT("Obj_Requirements")), 13, Muted(), true)]
						+ SVerticalBox::Slot().AutoHeight()[Requirements]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f)[Label(Budget, 14, Pal::Text())]
						+ SVerticalBox::Slot().AutoHeight()[Label(Weights, 13, Muted())]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
						[
							Button(bLocked ? Txt(TEXT("Lock_AfterDebrief")) : Txt(TEXT("Obj_Choose")), FOnClicked::CreateLambda([this, Id]()
							{
								if (Mission.IsValid() && Mission->BeginDesign(Id, false))
								{
									if (Mission->IsFirstFlight())
									{
										// First flight: start from one solar field, no rocket and no instrument, so the player causes each red line.
										Mission->ApplyFirstFlightStart();
										Mission->FirstFlight.bFissionUnlocked = false;
										Mission->FirstFlight.bWorkedExample = false;
										NamedKey = NAME_None;
										SetBeat(EBeat::Instrument);
									}
									Show(ETDDesignScreen::Design);
									if (Mission->IsFirstFlight())
									{
										ActiveRole = ETDRole::Science;
										Rebuild();
									}
								}
								return FReply::Handled();
							}), !bLocked, Pal::Red())
						], Panel(), 18.f)
				]
			];
		});
	}
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 18.f)
		[
			Box(SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Txt(TEXT("Design_Title"))).Font(HeadFont(32)).ColorAndOpacity(Pal::Text())]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[Label(Txt(TEXT("Obj_Intro")), 17, Pal::Text2())], Panel(), 18.f)
		]
		+ SVerticalBox::Slot().FillHeight(1.f)[SNew(SScrollBox) + SScrollBox::Slot()[Cards]];
}

// Design room

TSharedRef<SWidget> STDDesignRoom::BuildDesign()
{
	const FTDDesignBudget Budget = Mission->GetDesignBudget();

	TSharedRef<SVerticalBox> Slots = SNew(SVerticalBox);
	for (int32 SlotIndex = 0; SlotIndex < TD::NumDesignSlots; ++SlotIndex)
	{
		const ETDDesignSlot Slot = static_cast<ETDDesignSlot>(SlotIndex);
		if (OwnerOf(Slot) == ActiveRole)
		{
			Slots->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 16.f)[BuildSlot(Slot, Budget)];
		}
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[BuildHeader(Budget)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[IsFirstFlight() ? BuildBeatStrip(Budget) : BuildGuide(Budget)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f)[BuildRoleBar(Budget)]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.58f).Padding(0.f, 0.f, 14.f, 0.f)
			[
				Box(SNew(SScrollBox) + SScrollBox::Slot()[Slots], PanelDark(), 16.f)
			]
			+ SHorizontalBox::Slot().FillWidth(0.42f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot().Padding(0.f, 0.f, 0.f, 12.f)[BuildDashboard(Budget)]
				+ SScrollBox::Slot().Padding(0.f, 0.f, 0.f, 12.f)[BuildIssues(Budget)]
				+ SScrollBox::Slot()[BuildForecast()]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)[BuildPoll(Budget)];
}

STDDesignRoom::EGuide STDDesignRoom::ComputeGuide(const FTDDesignBudget& B) const
{
	auto Has = [&B](ETDDesignIssue I) { return B.Issues.ContainsByPredicate([I](const FTDDesignIssueLine& L) { return L.Issue == I; }); };
	if (Has(ETDDesignIssue::OverMass) || Has(ETDDesignIssue::ItemTooBig)) return EGuide::Mass;
	if (Has(ETDDesignIssue::PowerShort)) return EGuide::Power;
	if (Has(ETDDesignIssue::TooFewInstruments)) return EGuide::Instruments;
	if (Has(ETDDesignIssue::OverBudget) || Has(ETDDesignIssue::MissingSlot)) return EGuide::None;
	if (Has(ETDDesignIssue::DataShort) && !bDataStepSkipped) return EGuide::Data;
	return B.ConceptMaturity >= 4 ? EGuide::Launch : (B.bLaunchable ? EGuide::Poll : EGuide::None);
}

TSharedRef<SWidget> STDDesignRoom::BuildGuide(const FTDDesignBudget& B)
{
	if (!bGuide)
	{
		return SNullWidget::NullWidget;
	}
	const EGuide Step = ComputeGuide(B);
	if (Step == EGuide::None || Step == EGuide::Poll || Step == EGuide::Launch)
	{
		return SNullWidget::NullWidget; // later flights: the guide speaks only when a line is red
	}
	static const TCHAR* Keys[] = { TEXT("Guide_Free"), TEXT("Guide_Mass"), TEXT("Guide_Power"), TEXT("Guide_Instruments"), TEXT("Guide_Data"),
		TEXT("Guide_Poll"), TEXT("Guide_Launch") };
	const int32 Index = static_cast<int32>(Step);
	// Five dots: mass, power, science, data, GO/launch.
	TSharedRef<SHorizontalBox> Dots = SNew(SHorizontalBox);
	const int32 Done = Step == EGuide::None ? 0 : FMath::Min(Index - 1, 4);
	for (int32 Dot = 0; Dot < 5; ++Dot)
	{
		Dots->AddSlot().AutoWidth().Padding(3.f, 0.f)
		[
			SNew(SBox).WidthOverride(14.f).HeightOverride(14.f)
			[
				SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Dot < Done ? Pal::Go() : Dot == Done ? Pal::Red() : Pal::Line())
			]
		];
	}
	TSharedRef<SWidget> Action = SNullWidget::NullWidget;
	if (Step == EGuide::Data)
	{
		Action = Button(Txt(TEXT("Guide_KeepDirect")), FOnClicked::CreateLambda([this]() { bDataStepSkipped = true; Rebuild(); return FReply::Handled(); }),
			true, Pal::Text(), 13);
	}
	return Box(SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 14.f, 0.f)
		[
			SNew(SBox).WidthOverride(46.f).HeightOverride(46.f)
			[
				SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Pal::Red()).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					Label(FText::AsNumber(FMath::Max(1, Done + 1)), 24, Pal::White(), true, false)
				]
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[Label(Txt(Keys[Index]), 19, Pal::Text(), true)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[Dots]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f)[Action]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			Button(Txt(TEXT("Guide_Hide")), FOnClicked::CreateLambda([this]() { bGuide = false; Rebuild(); return FReply::Handled(); }), true, Pal::Line(), 12)
		], Pal::Raised(), 12.f);
}

TSharedRef<SWidget> STDDesignRoom::BuildHeader(const FTDDesignBudget& Budget)
{
	const FTDObjectiveRow* Objective = Mission->GetObjective();
	TSharedRef<SHorizontalBox> Cml = SNew(SHorizontalBox);
	for (int32 Level = 1; Level <= 4; ++Level)
	{
		const bool bReached = Budget.ConceptMaturity >= Level;
		Cml->AddSlot().AutoWidth().Padding(3.f, 0.f)
		[
			Box(Label(Txt(FName(*FString::Printf(TEXT("Cml_%d"), Level))), 12, bReached ? Pal::Ink() : Muted(), true, false),
				bReached ? Pal::Go() : Pal::Line(), 6.f)
		];
	}
	const bool bRealistic = Mission->GetDesign().bRealisticModel;
	return Box(SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[Label(Txt(TEXT("Design_Title")), 26, Pal::Text(), true, false)]
			+ SVerticalBox::Slot().AutoHeight()[Label(Objective ? Txt(Objective->NameKey) : FText::GetEmpty(), 16, Pal::Accent(), true, false)]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[Label(Txt(TEXT("Cml_Title")), 12, Muted(), true, false)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[Cml]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 0.f, 0.f)
		[
			Button(IsFirstFlight() ? Txt(TEXT("Btn_Example")) : Txt(TEXT("Btn_Recommended")), FOnClicked::CreateLambda([this]()
			{
				Mission->UseRecommendedDesign();
				if (IsFirstFlight())
				{
					// Worked example (Sweller): the design loads with its reasons; the player still polls the room.
					Mission->FirstFlight.bWorkedExample = true;
					Mission->FirstFlight.bFissionUnlocked = true;
					SetBeat(EBeat::Poll, TEXT("Flight_Example"));
				}
				Rebuild();
				return FReply::Handled();
			}), true, Pal::Text(), 13)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f, 0.f, 0.f)
		[
			bGuide ? SNullWidget::NullWidget : Button(Txt(TEXT("Guide_Show")), FOnClicked::CreateLambda([this]() { bGuide = true; Rebuild(); return FReply::Handled(); }),
				true, Pal::Line(), 13)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 0.f, 0.f)
		[
			IsFirstFlight() ? SNullWidget::NullWidget : StaticCastSharedRef<SWidget>(SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
			[
				Button(Txt(TEXT("Model_Simplified")), FOnClicked::CreateLambda([this]()
				{
					Mission->SetRealisticModel(false);
					Rebuild();
					return FReply::Handled();
				}), true, bRealistic ? Pal::Line() : Pal::Accent(), 13)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
			[
				Button(Txt(TEXT("Model_Realistic")), FOnClicked::CreateLambda([this]()
				{
					Mission->SetRealisticModel(true);
					Rebuild();
					return FReply::Handled();
				}), true, bRealistic ? Pal::Accent() : Pal::Line(), 13)
			])
		], Panel(), 12.f);
}

namespace
{
	int32 RoleBlockers(const FTDDesignBudget& Budget, ETDRole Role)
	{
		int32 Count = 0;
		for (const FTDDesignIssueLine& Line : Budget.Issues)
		{
			Count += Line.bBlocking && Line.Role == Role ? 1 : 0;
		}
		return Count;
	}
}

TSharedRef<SWidget> STDDesignRoom::BuildRoleBar(const FTDDesignBudget& Budget)
{
	const TArray<bool>& Go = Mission->GetDesign().SignedOff;
	TSharedRef<SHorizontalBox> Bar = SNew(SHorizontalBox);
	for (int32 RoleIndex = 0; RoleIndex < TD::NumRoles; ++RoleIndex)
	{
		const ETDRole Role = static_cast<ETDRole>(RoleIndex);
		const bool bActive = Role == ActiveRole;
		const bool bGoal = Mission->IsRoleGoalMet(Role);
		const bool bGo = Go.IsValidIndex(RoleIndex) && Go[RoleIndex];
		const FText State = FText::Format(Txt(TEXT("Role_State")), bGoal ? Txt(TEXT("Role_GoalMet")) : Txt(TEXT("Role_GoalOpen")),
			bGo ? Txt(TEXT("Poll_Go")) : Txt(TEXT("Poll_Waiting")));
		Bar->AddSlot().FillWidth(1.f).Padding(4.f, 0.f)
		[
			SNew(SButton)
			.ButtonStyle(FlatStyle())
			.ButtonColorAndOpacity(bActive ? RoleColor(Role) : Pal::Raised())
			.ContentPadding(FMargin(12.f, 8.f))
			.OnClicked(FOnClicked::CreateLambda([this, Role]()
			{
				ActiveRole = Role;
				Rebuild();
				return FReply::Handled();
			}))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SBox).WidthOverride(10.f).HeightOverride(18.f)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(bActive ? Pal::Ink() : RoleColor(Role))]
					]
					+ SHorizontalBox::Slot().FillWidth(1.f)[Label(RoleName(Role), 17, Pal::Ink(), true, false)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						RoleBlockers(Budget, Role) > 0
							? Box(Label(FText::Format(Txt(TEXT("Check_RoleBadge")), RoleBlockers(Budget, Role)), 12, Pal::White(), true, false), Pal::NoGo(), 4.f)
							: SNullWidget::NullWidget
					]
				]
				+ SVerticalBox::Slot().AutoHeight()[Label(Txt(FName(*FString::Printf(TEXT("Goal_%s"), *StaticEnum<ETDRole>()->GetNameStringByValue(RoleIndex)))), 12,
					bActive ? Pal::Ink() : Pal::Text2())]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0.f, 4.f, 0.f, 0.f)
				[
					Box(Label(State, 11, bGoal ? Pal::White() : Pal::Ink(), true, false), bGoal ? Pal::Go() : Pal::Caution(), 3.f)
				]
			]
		];
	}
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[Bar]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 6.f, 0.f, 0.f)
		[
			Label(FText::Format(Txt(TEXT("Role_Turn")), RoleName(ActiveRole)), 13, Pal::Raised())
		];
}

TSharedRef<SWidget> STDDesignRoom::BuildSlot(ETDDesignSlot Slot, const FTDDesignBudget& Budget)
{
	TSharedRef<SWrapBox> Cards = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(10.f, 10.f));
	if (const UDataTable* Options = Mission->GetOptionsTable())
	{
		Options->ForeachRow<FTDDesignOptionRow>(TEXT("TDDesignRoom"), [this, Slot, &Cards, &Budget](const FName& Key, const FTDDesignOptionRow& Row)
		{
			if (Row.Slot == Slot)
			{
				Cards->AddSlot()[BuildOptionCard(Key, Row, Budget)];
			}
		});
	}
	const FText Hint = Txt(FName(*FString::Printf(TEXT("Slot_%s_Hint"), *StaticEnum<ETDDesignSlot>()->GetNameStringByValue(static_cast<int64>(Slot)))));
	bool bFocus = Mission->FirstFlight.FocusSlot == static_cast<int32>(Slot);
	if (IsFirstFlight())
	{
		const EBeat Beat = GetBeat();
		bFocus = (Beat == EBeat::Instrument && Slot == ETDDesignSlot::Instrument)
			|| (Beat == EBeat::DataLink && Slot == ETDDesignSlot::Comms)
			|| (Beat == EBeat::Rockets && Slot == ETDDesignSlot::Launcher)
			|| (Beat == EBeat::Power && Slot == ETDDesignSlot::Power && !HasMassIssue(Budget))
			|| ((Beat == EBeat::Power || Beat == EBeat::Storm) && Slot == ETDDesignSlot::Launcher && HasMassIssue(Budget));
	}
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)[Glow(SNew(STextBlock).Text(SlotName(Slot)).Font(HeadFont(18)).ColorAndOpacity(Pal::Red()), bFocus)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 8.f)[Label(Hint, 13, Muted())]
		+ SVerticalBox::Slot().AutoHeight()[Cards];
}

TSharedRef<SWidget> STDDesignRoom::BuildOptionCard(FName OptionId, const FTDDesignOptionRow& Row, const FTDDesignBudget& Budget)
{
	const UTDMissionSubsystem* M = Mission.Get();
	const int32 Count = UTDDesignRules::GetPickCount(M->GetDesign(), OptionId);
	const bool bPicked = Count > 0;
	const bool bCounted = Row.MaxCount > 1;
	const bool bNasa = IsNasaValue(Row.MassKey) || IsNasaValue(Row.CapacityKey) || IsNasaValue(Row.PowerOutKey) || IsNasaValue(Row.PowerDrawKey);

	// Hover text holds the detail; the card itself shows the picture and the numbers.
	const FText Tip = FText::FromString(FString::Printf(TEXT("%s\n\n%s\n\n%s\n%s"), *Txt(Row.InfoKey).ToString(),
		*FText::Format(Txt(TEXT("Card_Why")), Txt(Row.WhyKey)).ToString(), *FText::Format(Txt(TEXT("Card_BasedOn")), FText::FromString(Row.BasedOn)).ToString(),
		*(bNasa ? Txt(TEXT("Card_NasaValues")) : Txt(TEXT("Card_GameValues"))).ToString()));

	TSharedRef<SWrapBox> Chips = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(5.f, 5.f));
	auto AddChip = [&Chips](const FText& Text, const FLinearColor& Color) { Chips->AddSlot()[Chip(Text, Color)]; };
	if (!Row.CapacityKey.IsNone()) { AddChip(FText::Format(Txt(TEXT("Stat_Capacity")), Num(M->GetConstant(Row.CapacityKey) / 1000.0, 1)), TDTokens::Hex(TEXT("B58CF0"))); }
	if (!Row.MassKey.IsNone())
	{
		const double Kg = M->GetConstant(Row.MassKey);
		AddChip(Kg >= 100.0 ? FText::Format(Txt(TEXT("Stat_MassT")), Num(Kg / 1000.0, 1)) : FText::Format(Txt(TEXT("Stat_MassKg")), Num(Kg, 1)), TDTokens::Hex(TEXT("B58CF0")));
	}
	if (!Row.PowerOutKey.IsNone()) { AddChip(FText::Format(Txt(TEXT("Stat_PowerOut")), Num(M->GetConstant(Row.PowerOutKey), 0)), TDTokens::Hex(TEXT("F07F2E"))); }
	if (!Row.PowerDrawKey.IsNone())
	{
		const double Kw = M->GetConstant(Row.PowerDrawKey);
		AddChip(Kw >= 1.0 ? FText::Format(Txt(TEXT("Stat_PowerDrawKw")), Num(Kw, 1)) : FText::Format(Txt(TEXT("Stat_PowerDrawW")), Num(Kw * 1000.0, 0)), TDTokens::Hex(TEXT("F07F2E")));
	}
	if (!Row.CostKey.IsNone()) { AddChip(FText::Format(Txt(TEXT("Stat_Cost")), Num(M->GetConstant(Row.CostKey), 0)), TDTokens::Hex(TEXT("F2C14E"))); }
	if (!Row.DataKey.IsNone()) { AddChip(FText::Format(Txt(TEXT("Stat_Data")), Num(M->GetConstant(Row.DataKey), 0)), TDTokens::Hex(TEXT("3A8DDE"))); }
	if (!Row.ScienceKey.IsNone()) { AddChip(FText::Format(Txt(TEXT("Stat_Science")), Num(M->GetConstant(Row.ScienceKey), 0)), TDTokens::Hex(TEXT("3FB27F"))); }

	FText LockReason;
	const bool bLocked = IsLocked(OptionId, &LockReason);
	const bool bNew = !bLocked && Mission->GetFlights().Num() == 1 && !Mission->FirstFlight.bSkipped && WasLockedOnFirstFlight(OptionId);
	TSharedRef<SWidget> Controls = SNullWidget::NullWidget;
	if (bLocked)
	{
		Controls = Button(LockReason, FOnClicked::CreateLambda([]() { return FReply::Handled(); }), false, Pal::Track(), 13);
	}
	else if (bCounted)
	{
		Controls = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				Button(FText::FromString(TEXT("-")), FOnClicked::CreateLambda([this, OptionId, Count]()
				{
					Mission->SetDesignPick(OptionId, Count - 1);
					Rebuild();
					return FReply::Handled();
				}), Count > 0, Pal::Accent(), 20)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				Label(FText::Format(Txt(TEXT("Card_Count")), Count, Row.MaxCount), 20, Pal::Text(), true, false)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				Button(FText::FromString(TEXT("+")), FOnClicked::CreateLambda([this, OptionId, Count]()
				{
					Mission->SetDesignPick(OptionId, Count + 1);
					Rebuild();
					return FReply::Handled();
				}), Count < Row.MaxCount, Pal::Accent(), 20)
			];
	}
	else
	{
		const bool bMulti = UTDDesignRules::IsMultiPick(Row.Slot);
		Controls = Button(bPicked ? (bMulti ? Txt(TEXT("Card_Remove")) : Txt(TEXT("Card_Picked"))) : Txt(TEXT("Card_Pick")),
			FOnClicked::CreateLambda([this, OptionId, bPicked, bMulti]()
			{
				Mission->SetDesignPick(OptionId, bPicked && bMulti ? 0 : 1);
				Rebuild();
				return FReply::Handled();
			}), !bPicked || bMulti, bPicked ? Pal::Go() : Pal::Accent(), 15);
	}

	// Picture: Gemini illustration with its AI credit tag, or a code tile.
	const FSlateBrush* Art = OptionArt(OptionId);
	TSharedRef<SOverlay> Picture = SNew(SOverlay);
	if (Art)
	{
		Picture->AddSlot()[SNew(SImage).Image(Art)];
		Picture->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(4.f)
		[
			Box(Label(FText::FromString(Row.ImageCredit), 9, Pal::White(), false, false), FLinearColor(0.f, 0.f, 0.f, 0.55f), 3.f)
		];
	}
	else
	{
		Picture->AddSlot()[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Pal::Line())
			.HAlign(HAlign_Center).VAlign(VAlign_Center)[Label(FText::FromString(Row.Code), 40, Muted(), true, false)]];
	}
	if (bLocked)
	{
		Picture->AddSlot()[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(FLinearColor(0.88f, 0.87f, 0.84f, 0.72f))];
		Picture->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Center)[Box(Label(Txt(TEXT("Lock_Tag")), 14, Pal::White(), true, false), Pal::Accent(), 6.f)];
	}
	if (bNew)
	{
		Picture->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(6.f)[Box(Label(Txt(TEXT("Lock_New")), 12, Pal::White(), true, false), Pal::Red(), 4.f)];
	}
	if (bPicked)
	{
		Picture->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(6.f)
		[
			Box(Label(bCounted ? FText::Format(FText::FromString(TEXT("x{0}")), Count) : FText::FromString(TEXT("OK")), 14, Pal::Ink(), true, false),
				Pal::Go(), 5.f)
		];
	}

	const bool bCardFocus = IsFirstFlight() && GetBeat() == EBeat::Storm && OptionId == FName(TEXT("Kilopower"));
	return SNew(SBox).WidthOverride(268.f)
	[
		Glow(SNew(SBorder).BorderImage(White()).BorderBackgroundColor(bPicked ? Pal::Go() : Pal::Line()).Padding(2.f).ToolTipText(bNew ? FText::FromString(Txt(TEXT("Lock_NewWhy")).ToString() + TEXT("\n\n") + Tip.ToString()) : Tip)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(134.f)[Picture]]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Box(SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[Label(Txt(Row.NameKey), 17, Pal::Text(), true)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)[Chips]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[Controls],
					PanelDark(), 10.f)
			]
		], bCardFocus)
	];
}

TSharedRef<SWidget> STDDesignRoom::BuildMeter(const FText& LabelText, double Value, double Max, const FText& ValueText, bool bOverIsBad, const FText& Detail,
	FName BarId, bool bHighlight)
{
	const double Ratio = Max > 0.0 ? Value / Max : (bOverIsBad && Value > 0.0 ? 2.0 : 0.0);
	const FLinearColor Fill = bOverIsBad ? (Ratio > 1.0 ? Pal::NoGo() : Ratio > 0.9 ? Pal::Caution() : Pal::Go())
		: (Ratio >= 1.0 ? Pal::Go() : Pal::Caution());
	// Over-limit bars show the limit as a white marker inside the bar (the fill runs to the end, red).
	const float Shown = bOverIsBad && Ratio > 1.0 ? 1.f : static_cast<float>(FMath::Clamp(Ratio, 0.0, 1.0));
	const float Marker = bOverIsBad && Ratio > 1.0 && Max > 0.0 ? static_cast<float>(1.0 / Ratio) : -1.f;
	const FName Id = BarId.IsNone() ? FName(*LabelText.ToString()) : BarId;
	TSharedRef<SWidget> Body = SNew(SVerticalBox)
		.ToolTipText(Detail)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)[Label(LabelText, 16, Pal::Text(), true, false)]
			+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(ValueText).Font(NumFont(16)).ColorAndOpacity(Fill)]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f, 0.f, 10.f)
		[
			SNew(SBox).HeightOverride(12.f)[SNew(STDAnimBar).Id(Id).Ratio(Shown).Fill(Fill).Marker(Marker)]
		];
	return Glow(Body, bHighlight);
}

TSharedRef<SWidget> STDDesignRoom::BuildDashboard(const FTDDesignBudget& B)
{
	const UTDMissionSubsystem* M = Mission.Get();
	const FText MassDetail = M->GetDesign().bRealisticModel
		? FText::Format(Txt(TEXT("Dash_MassDetail")), Num(B.HardwareKg / 1000.0, 1), Num(B.ConsumablesKg / 1000.0, 2), Num(B.LandingSystemKg / 1000.0, 1), B.Launches)
		: FText::Format(Txt(TEXT("Dash_MassDetailSimple")), Num(B.HardwareKg / 1000.0, 1), Num(B.ConsumablesKg / 1000.0, 2), B.Launches);
	const int32 MinCells = FMath::RoundToInt(M->GetConstant(TEXT("MinCells")));
	const FText PowerDetail = FText::Format(Txt(TEXT("Dash_PowerDetail")), Num(B.PowerOutKw, 1), Num(B.PowerDrawKw, 2),
		Num(M->GetConstant(TEXT("CellKw")), 0), B.DustCells);
	const FText DataDetail = FText::Format(B.bRelay ? Txt(TEXT("Dash_DataRelay")) : Txt(TEXT("Dash_DataDirect")),
		Num(B.StorageMb, 0), Num(B.DataReturnedMb, 0), Num(B.DataMadeTotalMb, 0));
	TArray<FString> PackageParts;
	for (int32 Index = 0; Index < B.Packages.Num(); ++Index)
	{
		const FTDLaunchPackage& P = B.Packages[Index];
		PackageParts.Add(FString::Printf(TEXT("L%d %s %.1f/%.1f t"), Index + 1, *P.LauncherId.ToString(), P.UsedKg / 1000.0, P.CapacityKg / 1000.0));
	}
	const FText PackagesLine = FText::Format(Txt(TEXT("Dash_Packages")), FText::FromString(FString::Join(PackageParts, TEXT("  ·  "))));
	const bool bSandbox = M->GetObjective() && M->GetObjective()->bSandbox;
	const EGuide Step = HighlightStep(B);

	TSharedRef<SVerticalBox> Box_ = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[SNew(STextBlock).Text(Txt(TEXT("Dash_Title"))).Font(HeadFont(18)).ColorAndOpacity(Pal::Text())]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
		[
			BuildCalendar(IsFirstFlight() && (GetBeat() == EBeat::DataLink || GetBeat() == EBeat::StormPredict))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			BuildMeter(Txt(TEXT("Dash_Mass")), B.MassKg, B.CapacityKg,
				FText::Format(Txt(TEXT("Dash_MassValue")), Num(B.MassKg / 1000.0, 1), Num(B.CapacityKg / 1000.0, 1)), true,
				FText::FromString(MassDetail.ToString() + TEXT("\n") + PackagesLine.ToString()), TEXT("Mass"), Step == EGuide::Mass)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).ToolTipText(PowerDetail)[BuildCells(B, Step == EGuide::Power)]
		];
	if (!bSandbox)
	{
		Box_->AddSlot().AutoHeight()
		[
			BuildMeter(Txt(TEXT("Dash_Budget")), B.CostM, B.BudgetM, FText::Format(Txt(TEXT("Dash_BudgetValue")), Num(B.CostM, 0), Num(B.BudgetM, 0)), true,
				Txt(TEXT("Dash_BudgetDetail")), TEXT("Budget"))
		];
	}
	Box_->AddSlot().AutoHeight()
	[
		BuildMeter(Txt(TEXT("Dash_Data")), B.DataReturnedMb, FMath::Max(1.0, B.DataMadeTotalMb),
			FText::Format(Txt(TEXT("Dash_DataValue")), Num(B.DataMadeMb, 0), Num(B.DataLinkMb, 0)), false, DataDetail, TEXT("Data"), Step == EGuide::Data)
	];
	Box_->AddSlot().AutoHeight()
	[
		BuildMeter(Txt(TEXT("Dash_Science")), B.Science, M->GetConstant(TEXT("ScienceGoal")),
			FText::Format(Txt(TEXT("Dash_ScienceValue")), Num(B.Science, 0), Num(M->GetConstant(TEXT("ScienceGoal")), 0)), false,
			FText::Format(Txt(TEXT("Dash_ScienceDetail")), B.Instruments), TEXT("Science"), Step == EGuide::Instruments)
	];
	return Box(Box_, Panel(), 16.f);
}

TSharedRef<SWidget> STDDesignRoom::FixButton(const FTDDesignIssueLine& Issue, bool bHighlight)
{
	const UTDMissionSubsystem* M = Mission.Get();
	const FTDDesignFix Fix = UTDDesignRules::SuggestFix(Issue, M->GetDesign(), M->GetOptionsTable(), M->GetConstantsTable());
	const FTDDesignOptionRow* Row = Fix.bValid && M->GetOptionsTable() ? M->GetOptionsTable()->FindRow<FTDDesignOptionRow>(Fix.OptionId, TEXT("TDDesignRoom"), false) : nullptr;
	if (!Row)
	{
		return SNullWidget::NullWidget;
	}
	const int32 Was = UTDDesignRules::GetPickCount(M->GetDesign(), Fix.OptionId);
	const FText What = Fix.NewCount > Was ? FText::Format(Txt(TEXT("Fix_Add")), Txt(Row->NameKey)) : FText::Format(Txt(TEXT("Fix_Switch")), Txt(Row->NameKey));
	const FTDDesignIssueLine Copy = Issue;
	return Glow(Button(FText::Format(Txt(TEXT("Fix_Button")), What, StatsLine(*Row)), FOnClicked::CreateLambda([this, Copy]()
	{
		Mission->ApplyFix(Copy);
		Rebuild();
		return FReply::Handled();
	}), true, Pal::Red(), 13), bHighlight);
}

TSharedRef<SWidget> STDDesignRoom::BuildIssues(const FTDDesignBudget& Budget)
{
	const EGuide Step = HighlightStep(Budget);
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[SNew(STextBlock).Text(Txt(TEXT("Review_Title"))).Font(HeadFont(18)).ColorAndOpacity(Pal::Text())];
	if (Budget.Issues.Num() == 0)
	{
		List->AddSlot().AutoHeight()[Label(Txt(TEXT("Review_AllClear")), 15, Pal::Go(), true)];
	}
	const TSet<FName> Met = LitNodes();
	const FName Now = CurrentNode();
	int32 Later = 0;
	for (const FTDDesignIssueLine& Issue : Budget.Issues)
	{
		if (Issue.Issue == ETDDesignIssue::RoleGoalMissed)
		{
			continue; // shown on the role tabs; keeps this list short
		}
		if (IsFirstFlight())
		{
			FName Node;
			switch (Issue.Issue)
			{
			case ETDDesignIssue::OverMass:
			case ETDDesignIssue::ItemTooBig: Node = TEXT("Rockets"); break;
			case ETDDesignIssue::OverBudget: Node = TEXT("Budget"); break;
			case ETDDesignIssue::PowerShort: Node = TEXT("Cells"); break;
			case ETDDesignIssue::DataShort: Node = TEXT("Data"); break;
			case ETDDesignIssue::TooFewInstruments: Node = TEXT("Instrument"); break;
			case ETDDesignIssue::MissingSlot:
				Node = Issue.Slot == ETDDesignSlot::Launcher ? FName(TEXT("Rockets")) : Issue.Slot == ETDDesignSlot::Comms ? FName(TEXT("Data"))
					: Issue.Slot == ETDDesignSlot::Power ? FName(TEXT("Cells")) : NAME_None;
				break;
			default: break;
			}
			const bool bMet = Node.IsNone() || Met.Contains(Node) || Node == Now || (Node == TEXT("Data") && Met.Contains(TEXT("Link")))
				|| (Node == TEXT("Rockets") && Met.Contains(TEXT("Mass")));
			if (!bMet)
			{
				++Later;
				continue;
			}
		}
		const FLinearColor Color = Issue.bBlocking ? Pal::NoGo() : Pal::Caution();
		const bool bHighlight = (Step == EGuide::Mass && (Issue.Issue == ETDDesignIssue::OverMass || Issue.Issue == ETDDesignIssue::ItemTooBig))
			|| (Step == EGuide::Power && Issue.Issue == ETDDesignIssue::PowerShort)
			|| (Step == EGuide::Instruments && Issue.Issue == ETDDesignIssue::TooFewInstruments)
			|| (Step == EGuide::Data && Issue.Issue == ETDDesignIssue::DataShort);
		List->AddSlot().AutoHeight().Padding(0.f, 4.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(SBox).WidthOverride(10.f).HeightOverride(18.f)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Color)]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)[Label(IssueText(Issue), 14, Color)]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(18.f, 2.f, 0.f, 0.f)
			[
				SNew(STextBlock).Text(Txt(FName(*FString::Printf(TEXT("Path_%s"), *StaticEnum<ETDDesignIssue>()->GetNameStringByValue(static_cast<int64>(Issue.Issue))))))
					.Font(NumFont(11)).ColorAndOpacity(Pal::Text2())
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(18.f, 4.f, 0.f, 0.f)
			[
				Issue.Issue == ETDDesignIssue::OverMass || Issue.Issue == ETDDesignIssue::ItemTooBig || Issue.Issue == ETDDesignIssue::PowerShort
					? FixChoices(Issue, bHighlight)
					: (IsFirstFlight() ? SNullWidget::NullWidget : FixButton(Issue, bHighlight))
			]
		];
	}
	if (Later > 0)
	{
		List->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[Label(FText::Format(Txt(TEXT("Review_Later")), Later), 13, Pal::Text2())];
	}
	return Box(List, Panel(), 16.f);
}

TSharedRef<SWidget> STDDesignRoom::BuildForecast()
{
	// Telegraph the known risks before launch, like a real mission's risk list (Into the Breach rule: show it first).
	return Box(SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[SNew(STextBlock).Text(Txt(TEXT("Forecast_Title"))).Font(HeadFont(18)).ColorAndOpacity(Pal::Text())]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)[Label(Txt(TEXT("Forecast_Conjunction")), 13, Pal::Text())]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)[Label(Txt(TEXT("Forecast_Dust")), 13, Pal::Text())]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)[Label(Txt(TEXT("Forecast_Sun")), 13, Pal::Text())]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)[Label(Txt(TEXT("Forecast_Equipment")), 13, Pal::Text())],
		Panel(), 16.f);
}

TSharedRef<SWidget> STDDesignRoom::BuildPoll(const FTDDesignBudget& Budget)
{
	const TArray<bool>& Go = Mission->GetDesign().SignedOff;
	const EGuide Step = HighlightStep(Budget);
	const bool bPollOpen = !IsFirstFlight() || static_cast<int32>(GetBeat()) >= static_cast<int32>(EBeat::Poll);
	TSharedRef<SHorizontalBox> Poll = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Txt(TEXT("Poll_Title"))).Font(HeadFont(16)).ColorAndOpacity(Pal::Text())]
			+ SVerticalBox::Slot().AutoHeight()[Label(Budget.bLaunchable ? Txt(TEXT("Poll_Ready")) : Txt(TEXT("Poll_Blocked")), 12, Muted(), false, false)]
		];
	Poll->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f, 8.f, 0.f)
	[
		Glow(Button(Txt(TEXT("Poll_Room")), FOnClicked::CreateLambda([this]()
		{
			// Each console answers in turn with the line it guards and its value (a recap of the model, not a cutscene).
			PollStart = FPlatformTime::Seconds();
			Rebuild();
			return FReply::Handled();
		}), Budget.bLaunchable && PollStart < 0.0 && bPollOpen, Pal::Go(), 14), Step == EGuide::Poll)
	];
	for (int32 RoleIndex = 0; RoleIndex < TD::NumRoles; ++RoleIndex)
	{
		const ETDRole Role = static_cast<ETDRole>(RoleIndex);
		const bool bGo = Go.IsValidIndex(RoleIndex) && Go[RoleIndex];
		Poll->AddSlot().FillWidth(1.f).VAlign(VAlign_Center).Padding(4.f, 0.f)
		[
			Button(FText::Format(Txt(TEXT("Poll_Role")), RoleName(Role), bGo ? Txt(TEXT("Poll_Go")) : Txt(TEXT("Poll_CallGo"))),
				FOnClicked::CreateLambda([this, Role, bGo]()
				{
					Mission->SetRoleGo(Role, !bGo);
					Rebuild();
					return FReply::Handled();
				}), Budget.bLaunchable && bPollOpen, bGo ? Pal::Go() : RoleColor(Role), 14)
		];
	}
	Poll->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 0.f, 0.f)
	[
		Glow(Button(Txt(TEXT("Poll_Launch")), FOnClicked::CreateLambda([this]()
		{
			if (Mission.IsValid() && Mission->IsFirstFlight())
			{
				Mission->LogOnboarding(TEXT("Launch"), Mission->GetDesignCode());
			}
			if (Mission.IsValid() && Mission->LaunchDesign())
			{
				Mission->FirstFlight.FocusSlot = -1;
				SetBeat(EBeat::Done);
				// Launch cutscene first (rocket lift-off from Earth), then the cruise card.
				TWeakPtr<SWidget> WeakSelf = AsShared();
				UTDCutsceneSubsystem::Play(Mission.Get(), {TEXT("CS_Launch")}, [this, WeakSelf]()
				{
					if (WeakSelf.IsValid())
					{
						Show(ETDDesignScreen::Transit);
						OnLaunched.ExecuteIfBound();
					}
				});
			}
			return FReply::Handled();
		}), Budget.ConceptMaturity >= 4 && bPollOpen, Pal::Red(), 20), Step == EGuide::Launch)
	];
	bool bAllGo = Go.Num() == TD::NumRoles;
	for (bool bOne : Go) { bAllGo &= bOne; }
	return Box(SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[BuildLaunchChecklist(Budget)]
		+ SVerticalBox::Slot().AutoHeight()[Poll]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
		[
			PollStart > 0.0 || (bAllGo && Budget.bLaunchable) ? PollReports(Budget, PollStart > 0.0) : SNullWidget::NullWidget
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
		[
			Label(FText::Format(Txt(TEXT("Design_Code")), FText::FromString(Mission->GetDesignCode())), 12, Muted(), false, false)
		], Panel(), 12.f);
}

// First flight (onboarding). Research: D:\NASA\research_notes\Görev tasarım oyunu GDC dersleri\onboarding_logic.md.
// Rules: beats follow the causal chains, not the panels; the player causes each red line before FLIGHT names it;
// predict, then observe; the first failure is a free storm preview of the player's own design; the debrief asks
// "which decision?" before it tells.

bool STDDesignRoom::IsFirstFlight() const
{
	return Mission.IsValid() && Mission->IsFirstFlight();
}

STDDesignRoom::EBeat STDDesignRoom::GetBeat() const
{
	return Mission.IsValid() ? static_cast<EBeat>(Mission->FirstFlight.Beat) : EBeat::Done;
}

void STDDesignRoom::SetBeat(EBeat NewBeat, const TCHAR* InNamedKey)
{
	if (!Mission.IsValid())
	{
		return;
	}
	if (InNamedKey)
	{
		NamedKey = FName(InNamedKey);
	}
	if (static_cast<EBeat>(Mission->FirstFlight.Beat) == NewBeat)
	{
		return;
	}
	Mission->FirstFlight.Beat = static_cast<uint8>(NewBeat);
	static const TCHAR* Names[] = { TEXT("ColdOpen"), TEXT("Instrument"), TEXT("DataPredict"), TEXT("DataLink"), TEXT("Rockets"), TEXT("Power"),
		TEXT("StormPredict"), TEXT("StormPreview"), TEXT("Storm"), TEXT("Poll"), TEXT("Launch"), TEXT("Done") };
	Mission->LogOnboarding(TEXT("Beat"), Names[static_cast<int32>(NewBeat)]);
	// Each beat opens the tab that owns its decision.
	switch (NewBeat)
	{
	case EBeat::Instrument: ActiveRole = ETDRole::Science; break;
	case EBeat::DataPredict:
	case EBeat::DataLink: ActiveRole = ETDRole::Comms; break;
	case EBeat::Rockets: ActiveRole = ETDRole::Planner; break;
	case EBeat::Power:
	case EBeat::StormPredict:
	case EBeat::Storm: ActiveRole = ETDRole::Engineer; break;
	default: break;
	}
}

bool STDDesignRoom::HasMassIssue(const FTDDesignBudget& B) const
{
	return B.Issues.ContainsByPredicate([](const FTDDesignIssueLine& L) { return L.Issue == ETDDesignIssue::OverMass || L.Issue == ETDDesignIssue::ItemTooBig; });
}

void STDDesignRoom::AdvanceBeats(const FTDDesignBudget& B)
{
	if (!IsFirstFlight())
	{
		return;
	}
	const int32 MinCells = FMath::RoundToInt(Mission->GetConstant(TEXT("MinCells")));
	for (int32 Guard = 0; Guard < 12; ++Guard)
	{
		const EBeat Beat = GetBeat();
		if (Beat == EBeat::Instrument && B.Instruments >= 1)
		{
			SetBeat(EBeat::DataPredict, TEXT("Flight_Instrument"));
			continue;
		}
		if (Beat == EBeat::DataLink && UTDDesignRules::GetPickCount(Mission->GetDesign(), TEXT("RelayOrbiter")) > 0)
		{
			SetBeat(EBeat::Rockets, TEXT("Flight_Data"));
			continue;
		}
		if (Beat == EBeat::Rockets && B.Launches >= 1 && !HasMassIssue(B))
		{
			SetBeat(EBeat::Power, Mission->GetDesign().bRealisticModel ? TEXT("Flight_Rockets") : TEXT("Flight_RocketsSimple"));
			continue;
		}
		if (Beat == EBeat::Power && B.Cells >= MinCells && !HasMassIssue(B))
		{
			SetBeat(EBeat::StormPredict, TEXT("Flight_Power"));
			continue;
		}
		if (Beat == EBeat::Storm && B.DustCells >= MinCells && !HasMassIssue(B))
		{
			SetBeat(EBeat::Poll, TEXT("Flight_Storm"));
			continue;
		}
		if (Beat == EBeat::Poll)
		{
			bool bAllGo = Mission->GetDesign().SignedOff.Num() == TD::NumRoles;
			for (bool bOne : Mission->GetDesign().SignedOff)
			{
				bAllGo &= bOne;
			}
			if (bAllGo && B.bLaunchable)
			{
				SetBeat(EBeat::Launch, TEXT("Flight_Poll"));
				continue;
			}
		}
		break;
	}
}

STDDesignRoom::EGuide STDDesignRoom::HighlightStep(const FTDDesignBudget& B) const
{
	if (!IsFirstFlight())
	{
		// Later flights: warnings only (the Civ VII pattern), no nagging about GO and launch.
		const EGuide Step = bGuide ? ComputeGuide(B) : EGuide::None;
		return Step == EGuide::Poll || Step == EGuide::Launch ? EGuide::None : Step;
	}
	switch (GetBeat())
	{
	case EBeat::Instrument: return EGuide::Instruments;
	case EBeat::DataPredict:
	case EBeat::DataLink: return EGuide::Data;
	case EBeat::Rockets: return EGuide::Mass;
	case EBeat::Power:
	case EBeat::StormPredict:
	case EBeat::StormPreview:
	case EBeat::Storm: return HasMassIssue(B) ? EGuide::Mass : EGuide::Power;
	case EBeat::Poll: return B.bLaunchable ? EGuide::Poll : EGuide::None;
	case EBeat::Launch: return EGuide::Launch;
	default: return EGuide::None;
	}
}

bool STDDesignRoom::WasLockedOnFirstFlight(FName OptionId) const
{
	static const TSet<FName> Later = { TEXT("GreenhouseLarge"), TEXT("ShelterWaterWall"), TEXT("ShelterRegolith"), TEXT("SparesKit"), TEXT("SparesFull"),
		TEXT("Pixl"), TEXT("Sherloc"), TEXT("Meda"), TEXT("Moxie"), TEXT("Rad") };
	return Later.Contains(OptionId);
}

bool STDDesignRoom::IsLocked(FName OptionId, FText* OutReason) const
{
	if (!IsFirstFlight())
	{
		return false;
	}
	if (OptionId == FName(TEXT("Kilopower")))
	{
		// Fission unlocks at the storm preview, when the player needs it (KSP tech tree, Mini Metro weekly unlock).
		if (Mission->FirstFlight.bFissionUnlocked)
		{
			return false;
		}
		if (OutReason) { *OutReason = Txt(TEXT("Lock_Fission")); }
		return true;
	}
	if (!WasLockedOnFirstFlight(OptionId))
	{
		return false;
	}
	if (OutReason)
	{
		*OutReason = OptionId == FName(TEXT("SparesKit")) || OptionId == FName(TEXT("SparesFull")) ? Txt(TEXT("Lock_Spares")) : Txt(TEXT("Lock_Flight2"));
	}
	return true;
}

TArray<FIntPoint> STDDesignRoom::DustPhaseSols() const
{
	TArray<FIntPoint> Out;
	const UDataTable* Events = Mission.IsValid() ? Mission->GetEventsTable() : nullptr;
	const UDataTable* Phases = Mission.IsValid() ? Mission->GetPhasesTable() : nullptr;
	if (!Events || !Phases)
	{
		return Out;
	}
	int32 Mask = 0;
	Events->ForeachRow<FTDEventRow>(TEXT("TDDesignRoom"), [&Mask](const FName&, const FTDEventRow& Row)
	{
		if (Row.bDustStorm)
		{
			Mask |= Row.AllowedPhasesMask;
		}
	});
	Phases->ForeachRow<FTDPhaseRow>(TEXT("TDDesignRoom"), [&Out, Mask](const FName&, const FTDPhaseRow& Row)
	{
		if ((Mask & (1 << static_cast<int32>(Row.Phase))) != 0 && !Row.bIsBlackout)
		{
			Out.Add(FIntPoint(Row.SolStart, Row.SolEnd));
		}
	});
	Out.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X < B.X; });
	return Out;
}

// Causal map: the rules as visible objects (Baba Is You; concept-map studies). Nodes light up the first time the
// player experiences that link; the debrief draws the same map with the failure path in red.

TSet<FName> STDDesignRoom::LitNodes() const
{
	static const TCHAR* All[] = { TEXT("Objective"), TEXT("Instrument"), TEXT("Data"), TEXT("Link"), TEXT("Payload"), TEXT("Mass"), TEXT("Rockets"),
		TEXT("Budget"), TEXT("Power"), TEXT("Cells"), TEXT("Dust"), TEXT("GO") };
	TSet<FName> Lit;
	auto Add = [&Lit](std::initializer_list<const TCHAR*> Names) { for (const TCHAR* Name : Names) { Lit.Add(FName(Name)); } };
	if (!IsFirstFlight() || Mission->FirstFlight.bWorkedExample)
	{
		for (const TCHAR* Name : All) { Lit.Add(FName(Name)); }
		return Lit;
	}
	const int32 Beat = static_cast<int32>(GetBeat());
	if (Beat > static_cast<int32>(EBeat::Instrument)) Add({ TEXT("Objective"), TEXT("Instrument") });
	if (Beat > static_cast<int32>(EBeat::DataLink)) Add({ TEXT("Data"), TEXT("Link") });
	if (Beat > static_cast<int32>(EBeat::Rockets)) Add({ TEXT("Payload"), TEXT("Mass"), TEXT("Rockets"), TEXT("Budget") });
	if (Beat > static_cast<int32>(EBeat::Power)) Add({ TEXT("Power"), TEXT("Cells") });
	if (Beat > static_cast<int32>(EBeat::Storm)) Add({ TEXT("Dust") });
	if (Beat >= static_cast<int32>(EBeat::Launch)) Add({ TEXT("GO") });
	return Lit;
}

FName STDDesignRoom::CurrentNode() const
{
	if (!IsFirstFlight())
	{
		return NAME_None;
	}
	switch (GetBeat())
	{
	case EBeat::Instrument: return TEXT("Instrument");
	case EBeat::DataPredict:
	case EBeat::DataLink: return TEXT("Data");
	case EBeat::Rockets: return TEXT("Rockets");
	case EBeat::Power: return TEXT("Cells");
	case EBeat::StormPredict:
	case EBeat::StormPreview:
	case EBeat::Storm: return TEXT("Dust");
	case EBeat::Poll:
	case EBeat::Launch: return TEXT("GO");
	default: return NAME_None;
	}
}

TSet<FName> STDDesignRoom::FailingNodes(const FTDDesignBudget& B) const
{
	TSet<FName> Failing;
	for (const FTDDesignIssueLine& Issue : B.Issues)
	{
		switch (Issue.Issue)
		{
		case ETDDesignIssue::OverMass:
		case ETDDesignIssue::ItemTooBig: Failing.Add(TEXT("Mass")); Failing.Add(TEXT("Rockets")); break;
		case ETDDesignIssue::OverBudget: Failing.Add(TEXT("Budget")); break;
		case ETDDesignIssue::PowerShort: Failing.Add(TEXT("Cells")); break;
		case ETDDesignIssue::DataShort: Failing.Add(TEXT("Link")); break;
		case ETDDesignIssue::TooFewInstruments: Failing.Add(TEXT("Instrument")); break;
		default: break;
		}
	}
	if (B.DustCells < FMath::RoundToInt(Mission->GetConstant(TEXT("MinCells"))))
	{
		Failing.Add(TEXT("Dust"));
	}
	return Failing;
}

TSharedRef<SWidget> STDDesignRoom::BuildCausalMap(const TSet<FName>& Lit, const TSet<FName>& Failing, FName Current, int32 Size) const
{
	auto Node = [this, &Lit, &Failing, Current, Size](const TCHAR* Id) -> TSharedRef<SWidget>
	{
		const FName Name(Id);
		const bool bLit = Lit.Contains(Name);
		const bool bCurrent = Name == Current;
		const bool bFail = Failing.Contains(Name) && (bLit || bCurrent);
		const FLinearColor Fill = bFail ? Pal::NoGo() : (bLit || bCurrent) ? ChainColor(Name) : Pal::Track();
		const FLinearColor Ink = bFail ? Pal::White() : (bLit || bCurrent) ? Pal::Ink() : Pal::Text2();
		TSharedRef<SWidget> Chip = SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Fill).Padding(FMargin(7.f, 2.f))
			[
				SNew(STextBlock).Text(Txt(FName(*FString::Printf(TEXT("Node_%s"), Id)))).Font(Font(Size, true)).ColorAndOpacity(Ink)
			];
		return bCurrent ? Glow(Chip, true) : Chip;
	};
	auto Arrow = [Size](const TCHAR* Glyph) -> TSharedRef<SWidget>
	{
		return SNew(STextBlock).Text(FText::FromString(Glyph)).Font(NumFont(Size)).ColorAndOpacity(Pal::Text2());
	};
	TSharedRef<SWrapBox> Row = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(0.f, 4.f));
	auto Push = [&Row](const TSharedRef<SWidget>& W, float Left = 3.f) { Row->AddSlot().VAlign(VAlign_Center).Padding(FMargin(Left, 0.f, 3.f, 0.f))[W]; };
	const TCHAR* ScienceChain[] = { TEXT("Objective"), TEXT("Instrument"), TEXT("Data"), TEXT("Link") };
	const TCHAR* CargoChain[] = { TEXT("Payload"), TEXT("Mass"), TEXT("Rockets"), TEXT("Budget") };
	for (int32 Index = 0; Index < 4; ++Index)
	{
		if (Index > 0) { Push(Arrow(TEXT("→"))); }
		Push(Node(ScienceChain[Index]));
	}
	Push(Arrow(TEXT("|")), 10.f);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		if (Index > 0) { Push(Arrow(TEXT("→"))); }
		Push(Node(CargoChain[Index]));
	}
	Push(Arrow(TEXT("|")), 10.f);
	Push(Node(TEXT("Power")));
	Push(Arrow(TEXT("→")));
	Push(Node(TEXT("Cells")));
	Push(Arrow(TEXT("←")));
	Push(Node(TEXT("Dust")));
	Push(Arrow(TEXT("→")), 10.f);
	Push(Node(TEXT("GO")));
	return Row;
}

// Mission calendar: every Act 2 event telegraphed in Act 1 (Into the Breach; Frostpunk foreshadowing).

TSharedRef<SWidget> STDDesignRoom::BuildCalendar(bool bHighlight)
{
	const UDataTable* Phases = Mission->GetPhasesTable();
	const int32 Sols = FMath::Max(1, FMath::RoundToInt(Mission->GetConstant(TEXT("MissionSols"))));
	TArray<FName> Cat;
	Cat.Init(TEXT("Quiet"), Sols + 1);
	TSet<int32> DustSols;
	for (const FIntPoint& Range : DustPhaseSols())
	{
		for (int32 Sol = Range.X; Sol <= Range.Y; ++Sol) { DustSols.Add(Sol); }
	}
	if (Phases)
	{
		Phases->ForeachRow<FTDPhaseRow>(TEXT("TDDesignRoom"), [&Cat, &DustSols, Sols](const FName&, const FTDPhaseRow& Row)
		{
			for (int32 Sol = FMath::Max(1, Row.SolStart); Sol <= FMath::Min(Sols, Row.SolEnd); ++Sol)
			{
				FName C = TEXT("Quiet");
				if (Row.bIsBlackout) C = TEXT("NoLink");
				else if (DustSols.Contains(Sol)) C = Row.EventFamily == ETDEventFamily::Equipment ? TEXT("DustBreaks") : TEXT("Dust");
				else if (Row.EventFamily == ETDEventFamily::Radiation) C = TEXT("Sun");
				else if (Row.EventFamily == ETDEventFamily::Equipment) C = TEXT("Breaks");
				Cat[Sol] = C;
			}
		});
	}
	auto ColorOf = [](FName C)
	{
		if (C == TEXT("NoLink")) return Pal::Accent();
		if (C == TEXT("Dust") || C == TEXT("DustBreaks")) return TDTokens::Hex(TEXT("F07F2E"));
		if (C == TEXT("Sun")) return Pal::Caution();
		if (C == TEXT("Breaks")) return Pal::Text2();
		return Pal::Track();
	};
	TSharedRef<SHorizontalBox> Strip = SNew(SHorizontalBox);
	TSet<FName> Used;
	for (int32 Sol = 1; Sol <= Sols; ++Sol)
	{
		Strip->AddSlot().FillWidth(1.f).Padding(0.5f, 0.f)[SNew(SBox).HeightOverride(16.f)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(ColorOf(Cat[Sol]))]];
		Used.Add(Cat[Sol] == TEXT("DustBreaks") ? FName(TEXT("Dust")) : Cat[Sol]);
	}
	TSharedRef<SWrapBox> Legend = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(14.f, 2.f));
	for (const TCHAR* Key : { TEXT("Dust"), TEXT("Sun"), TEXT("Breaks"), TEXT("NoLink") })
	{
		if (!Used.Contains(FName(Key)))
		{
			continue;
		}
		Legend->AddSlot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 5.f, 0.f)
			[
				SNew(SBox).WidthOverride(12.f).HeightOverride(12.f)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(ColorOf(FName(Key)))]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Txt(FName(*FString::Printf(TEXT("Cal_%s"), Key)))).Font(Font(12, true)).ColorAndOpacity(Pal::Text())
			]
		];
	}
	TSharedRef<SWidget> Body = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)[SNew(STextBlock).Text(FText::Format(Txt(TEXT("Cal_Sol")), 1)).Font(NumFont(10)).ColorAndOpacity(Pal::Text2())]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[Strip]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f, 0.f, 0.f)[SNew(STextBlock).Text(FText::Format(Txt(TEXT("Cal_Sol")), Sols)).Font(NumFont(10)).ColorAndOpacity(Pal::Text2())]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)[Legend];
	return Glow(Body, bHighlight);
}

// Power as cell tokens: what the player will split on the surface, drawn the same way here.

TSharedRef<SWidget> STDDesignRoom::BuildCells(const FTDDesignBudget& B, bool bHighlight)
{
	const int32 Need = FMath::RoundToInt(Mission->GetConstant(TEXT("MinCells")));
	const int32 Clear = B.Cells;
	const int32 Dust = B.DustCells;
	const int32 Slots = FMath::Clamp(FMath::Max3(Clear, Dust, Need), Need, 14);
	const bool bDustTaught = !IsFirstFlight() || static_cast<int32>(GetBeat()) > static_cast<int32>(EBeat::StormPredict);
	auto Line = [this, Need, Slots](const FText& Name, int32 Count, const FLinearColor& On) -> TSharedRef<SWidget>
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(108.f)[Label(Name, 12, Pal::Text2(), true, false)]]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[TokenRow([Count]() { return static_cast<float>(Count); }, Need, Slots, On, 16.f)]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::Format(FText::FromString(TEXT("{0} / {1}")), Count, Need)).Font(NumFont(15)).ColorAndOpacity(Count >= Need ? Pal::Go() : Pal::NoGo())
			];
	};
	TSharedRef<SVerticalBox> Body = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[Label(Txt(TEXT("Dash_Power")), 16, Pal::Text(), true, false)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f, 0.f, 3.f)[Line(Txt(TEXT("Cells_Clear")), Clear, Pal::Go())];
	Body->AddSlot().AutoHeight().Padding(0.f, 3.f)
	[
		bDustTaught ? Line(Txt(TEXT("Cells_Dust")), Dust, Dust >= Need ? Pal::Go() : TDTokens::Hex(TEXT("F07F2E")))
			: StaticCastSharedRef<SWidget>(Label(Txt(TEXT("Cells_DustLater")), 12, Pal::Text2(), false, false))
	];
	Body->AddSlot().AutoHeight().Padding(0.f, 2.f, 0.f, 10.f)
	[
		Label(FText::Format(Txt(TEXT("Cells_Note")), Num(Mission->GetConstant(TEXT("CellKw")), 0), Need,
			Num(Mission->GetConstant(TEXT("BaseLoadKw")), 0), Num(Mission->GetConstant(TEXT("HeatingKw")), 1)), 11, Pal::Text2())
	];
	return Glow(Body, bHighlight);
}

TSharedRef<SWidget> STDDesignRoom::BuildStormPreview(const FTDDesignBudget& B)
{
	const int32 Need = FMath::RoundToInt(Mission->GetConstant(TEXT("MinCells")));
	const int32 From = B.Cells;
	const int32 To = B.DustCells;
	const int32 Slots = FMath::Clamp(FMath::Max(From, Need), Need, 14);
	const double Start = StormPreviewStart;
	TFunction<float()> Shown = [Start, From, To]()
	{
		const float A = FMath::Clamp(static_cast<float>((FPlatformTime::Seconds() - Start - 1.0) / 3.0), 0.f, 1.f);
		return FMath::Lerp(static_cast<float>(From), static_cast<float>(To), FMath::SmoothStep(0.f, 1.f, A));
	};
	const TArray<FIntPoint> Dust = DustPhaseSols();
	const FIntPoint Sols = Dust.Num() > 0 ? Dust[0] : FIntPoint(0, 0);
	const bool bHolds = To >= Need;
	const int32 DustPct = FMath::RoundToInt((1.0 - Mission->GetConstant(TEXT("DustStormSolarShare"))) * 100.0);
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[Label(FText::Format(Txt(TEXT("Storm_Title")), Sols.X, Sols.Y, DustPct), 19, Pal::Text(), true, false)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[TokenRow(Shown, Need, Slots, Pal::Go(), 26.f)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14.f, 0.f)
			[
				SNew(STextBlock).Font(NumFont(22)).ColorAndOpacity(Pal::Text())
					.Text_Lambda([Shown, Need]() { return FText::Format(FText::FromString(TEXT("{0} / {1}")), FMath::RoundToInt(Shown()), Need); })
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Text(FText::Format(bHolds ? Txt(TEXT("Storm_Holds")) : Txt(TEXT("Storm_Fails")), To, Need)).Font(Font(17, true))
				.ColorAndOpacity(bHolds ? Pal::Go() : Pal::NoGo())
				.Visibility_Lambda([Start]() { return FPlatformTime::Seconds() - Start > 4.4 ? EVisibility::Visible : EVisibility::Hidden; })
		];
}

// Two fixes with their trade-offs (Factorio's "two masters"): the reasoning stays with the player.

TSharedRef<SWidget> STDDesignRoom::FixChoices(const FTDDesignIssueLine& Issue, bool bHighlight, bool bStorm)
{
	const UTDMissionSubsystem* M = Mission.Get();
	const FTDMissionDesign& Design = M->GetDesign();
	const bool bMass = !bStorm && (Issue.Issue == ETDDesignIssue::OverMass || Issue.Issue == ETDDesignIssue::ItemTooBig);
	const int32 MinCells = FMath::RoundToInt(M->GetConstant(TEXT("MinCells")));
	TArray<TPair<FName, int32>> Candidates;
	auto Plus = [&Candidates, &Design](const TCHAR* Id, int32 N) { Candidates.Add({ FName(Id), UTDDesignRules::GetPickCount(Design, FName(Id)) + N }); };
	if (bStorm) { Plus(TEXT("Kilopower"), 1); Plus(TEXT("Kilopower"), 2); }
	else if (bMass)
	{
		const bool bHeavy = UTDDesignRules::GetPickCount(Design, TEXT("FalconHeavy")) > 0 || UTDDesignRules::GetPickCount(Design, TEXT("Falcon9")) == 0;
		const TCHAR* Booked = bHeavy ? TEXT("FalconHeavy") : TEXT("Falcon9");
		const TCHAR* Other = bHeavy ? TEXT("Falcon9") : TEXT("FalconHeavy");
		Plus(Booked, 1);
		const FTDDesignOptionRow* OtherRow = M->GetOptionsTable() ? M->GetOptionsTable()->FindRow<FTDDesignOptionRow>(FName(Other), TEXT("TDDesignRoom"), false) : nullptr;
		for (int32 Count = 1; OtherRow && Count <= OtherRow->MaxCount; ++Count)
		{
			FTDMissionDesign Trial = Design;
			UTDDesignRules::SetPick(Trial, M->GetOptionsTable(), FName(Other), Count);
			if (!HasMassIssue(UTDDesignRules::Evaluate(Trial, M->GetOptionsTable(), M->GetObjectivesTable(), M->GetConstantsTable())))
			{
				Candidates.Add({ FName(Other), Count });
				break;
			}
		}
	}
	else if (Issue.Issue == ETDDesignIssue::PowerShort) { Plus(TEXT("SolarField"), 1); Plus(TEXT("Kilopower"), 1); }

	const FTDDesignBudget Now = M->GetDesignBudget();
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	int32 Shown = 0;
	bool bGlowDone = false;
	for (const TPair<FName, int32>& Candidate : Candidates)
	{
		const FTDDesignOptionRow* Option = M->GetOptionsTable() ? M->GetOptionsTable()->FindRow<FTDDesignOptionRow>(Candidate.Key, TEXT("TDDesignRoom"), false) : nullptr;
		if (!Option || Candidate.Value > Option->MaxCount || IsLocked(Candidate.Key))
		{
			continue;
		}
		FTDMissionDesign Trial = Design;
		UTDDesignRules::SetPick(Trial, M->GetOptionsTable(), Candidate.Key, Candidate.Value);
		const FTDDesignBudget After = UTDDesignRules::Evaluate(Trial, M->GetOptionsTable(), M->GetObjectivesTable(), M->GetConstantsTable());
		const bool bClears = bStorm ? After.DustCells >= MinCells : bMass ? !HasMassIssue(After) : After.Cells >= MinCells;
		auto Signed = [](double V, int32 Digits) { return FText::FromString(FString(V < 0.0 ? TEXT("−") : TEXT("+")) + Num(FMath::Abs(V), Digits).ToString()); };
		TArray<FString> Parts;
		Parts.Add(FText::Format(Txt(TEXT("Choice_Cost")), Signed(After.CostM - Now.CostM, 0)).ToString());
		if (bMass)
		{
			Parts.Add(FText::Format(Txt(TEXT("Choice_Cap")), Signed((After.CapacityKg - Now.CapacityKg) / 1000.0, 1)).ToString());
		}
		else
		{
			Parts.Add(FText::Format(Txt(TEXT("Choice_Cells")), Signed(After.Cells - Now.Cells, 0), Signed(After.DustCells - Now.DustCells, 0)).ToString());
			Parts.Add(FText::Format(Txt(TEXT("Choice_Mass")), Signed((After.MassKg - Now.MassKg) / 1000.0, 1)).ToString());
		}
		if (!bClears)
		{
			Parts.Add(Txt(TEXT("Choice_NotEnough")).ToString());
		}
		const int32 Had = UTDDesignRules::GetPickCount(Design, Candidate.Key);
		const FText Head = Had > 0 ? FText::Format(Txt(TEXT("Choice_Add")), Candidate.Value - Had, Txt(Option->NameKey))
			: FText::Format(Txt(TEXT("Choice_Switch")), Candidate.Value, Txt(Option->NameKey));
		const FText Text = FText::FromString(Head.ToString() + TEXT("\n") + FString::Join(Parts, TEXT("  ·  ")));
		const FName Id = Candidate.Key;
		const int32 Count = Candidate.Value;
		TSharedRef<SWidget> Btn = Button(Text, FOnClicked::CreateLambda([this, Id, Count]()
		{
			if (Mission.IsValid())
			{
				if (Mission->IsFirstFlight())
				{
					Mission->LogOnboarding(TEXT("Choice"), FString::Printf(TEXT("%s=%d"), *Id.ToString(), Count));
				}
				Mission->SetDesignPick(Id, Count);
			}
			Rebuild();
			return FReply::Handled();
		}), true, bClears ? Pal::Red() : Pal::Accent(), 13);
		const bool bGlow = bHighlight && bClears && !bGlowDone;
		bGlowDone |= bGlow;
		Row->AddSlot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)[Glow(Btn, bGlow)];
		++Shown;
	}
	if (Shown == 0)
	{
		return IsFirstFlight() ? SNullWidget::NullWidget : FixButton(Issue, bHighlight);
	}
	return Row;
}

TSharedRef<SWidget> STDDesignRoom::BuildColdOpen()
{
	// Beat 1 (about 20 s): see the consequence before any rule (Portal "see yourself", KSP2 concept-first video).
	// Ares-0 is a fictional earlier crew; every number comes from the same game constants the player's design uses.
	if (ColdOpenStart <= 0.0)
	{
		ColdOpenStart = FPlatformTime::Seconds();
		Mission->LogOnboarding(TEXT("ColdOpen"), TEXT("shown"));
	}
	const double Start = ColdOpenStart;
	auto Elapsed = [Start]() { return FPlatformTime::Seconds() - Start; };
	auto After = [Elapsed](double T)
	{
		return TAttribute<EVisibility>::CreateLambda([Elapsed, T]() { return Elapsed() >= T ? EVisibility::Visible : EVisibility::Hidden; });
	};
	const double FieldKw = Mission->GetConstant(TEXT("SolarFieldKwe"));
	const double CellKw = FMath::Max(0.1, Mission->GetConstant(TEXT("CellKw")));
	const double Share = Mission->GetConstant(TEXT("DustStormSolarShare"));
	const int32 Need = FMath::RoundToInt(Mission->GetConstant(TEXT("MinCells")));
	const int32 Clear = FMath::RoundToInt(2.0 * FieldKw / CellKw);
	const int32 Dusty = FMath::RoundToInt(2.0 * FieldKw * Share / CellKw);
	const int32 DustPct = FMath::RoundToInt((1.0 - Share) * 100.0);
	const TArray<FIntPoint> DustSols = DustPhaseSols();
	const int32 StormSol = DustSols.Num() > 0 ? DustSols.Last().X : 1;
	TFunction<float()> Shown = [Elapsed, Clear, Dusty]()
	{
		const float A = FMath::Clamp(static_cast<float>((Elapsed() - 4.0) / 4.0), 0.f, 1.f);
		return FMath::Lerp(static_cast<float>(Clear), static_cast<float>(Dusty), FMath::SmoothStep(0.f, 1.f, A));
	};
	const FLinearColor Light = TDTokens::Hex(TEXT("C9CED6"));
	// Ares-0 is fictional, so its picture is an illustration, never a photo of a real mission.
	const FSlateBrush* Photo = ArtBrush(TEXT("T_TD_ColdOpen_Ares0"));

	TSharedRef<SOverlay> Picture = SNew(SOverlay);
	if (Photo)
	{
		Picture->AddSlot()[SNew(SImage).Image(Photo)];
	}
	Picture->AddSlot()
	[
		SNew(SBorder).BorderImage(White()).BorderBackgroundColor(TAttribute<FSlateColor>::CreateLambda([Elapsed]()
		{
			// Dust closes over the arrays, then the base goes dark.
			const float DustA = FMath::Clamp(static_cast<float>((Elapsed() - 3.5) / 4.0), 0.f, 1.f);
			const float DarkA = FMath::Clamp(static_cast<float>((Elapsed() - 8.0) / 1.5), 0.f, 1.f);
			FLinearColor C = FMath::Lerp(FLinearColor(0.62f, 0.36f, 0.18f, 0.72f * DustA), FLinearColor(0.02f, 0.02f, 0.03f, 0.85f), DarkA);
			return FSlateColor(C);
		}))
	];
	Picture->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(6.f)
	[
		Box(Label(Txt(TEXT("ColdOpen_Credit")), 10, Pal::White(), false, false), FLinearColor(0.f, 0.f, 0.f, 0.55f), 3.f)
	];

	TSharedRef<SWidget> Right = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Font(NumFont(30)).ColorAndOpacity(Pal::White())
				.Text_Lambda([this, Elapsed, StormSol]()
				{
					const int32 Sol = FMath::Clamp(1 + FMath::FloorToInt(static_cast<float>(Elapsed() / 4.0 * (StormSol - 1))), 1, StormSol);
					return FText::Format(Txt(TEXT("Cal_Sol")), Sol);
				})
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 4.f)
		[
			SNew(SBox).Visibility(After(0.6))[Label(Txt(TEXT("ColdOpen_1")), 20, Pal::White(), false)]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
		[
			SNew(SBox).Visibility(After(3.8))[Label(FText::Format(Txt(TEXT("ColdOpen_2")), StormSol, DustPct), 20, TDTokens::Hex(TEXT("F2A35E")), true)]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 4.f)[Label(Txt(TEXT("Cells_Title")), 12, Light, true, false)]
		+ SVerticalBox::Slot().AutoHeight()[TokenRow(Shown, Need, FMath::Clamp(Clear, Need, 14), Pal::Go(), 24.f)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 4.f)
		[
			SNew(SBox).Visibility(After(8.4))[Label(FText::Format(Txt(TEXT("ColdOpen_3")), Dusty, Need), 20, TDTokens::Hex(TEXT("FF6B5E")), true)]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)
		[
			SNew(SBox).Visibility(After(10.0))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 2.f, 10.f, 0.f)[Box(Label(Txt(TEXT("Flight_Tag")), 12, Pal::White(), true, false), Pal::Red(), 4.f)]
				+ SHorizontalBox::Slot().FillWidth(1.f)[Label(Txt(TEXT("ColdOpen_Flight")), 20, Pal::White(), true)]
			]
		];

	return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
	[
		SNew(SBox).WidthOverride(1560.f)
		[
			Box(SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 14.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)[Box(Label(Txt(TEXT("ColdOpen_Replay")), 13, Pal::White(), true, false), Pal::Red(), 5.f)]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[SNew(STextBlock).Text(Txt(TEXT("ColdOpen_Title"))).Font(HeadFont(22)).ColorAndOpacity(Pal::White())]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(840.f).HeightOverride(420.f)[Picture]]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(30.f, 0.f, 0.f, 0.f)[Right]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						Glow(Button(Txt(TEXT("ColdOpen_Take")), FOnClicked::CreateLambda([this, Elapsed]()
						{
							Mission->FirstFlight.bColdOpenSeen = true;
							Mission->FirstFlight.StartSeconds = FPlatformTime::Seconds();
							Mission->LogOnboarding(TEXT("ColdOpen"), FString::Printf(TEXT("take after %.1f s"), Elapsed()));
							Rebuild();
							return FReply::Handled();
						}), true, Pal::Red(), 20), true)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16.f, 0.f)
					[
						Button(Txt(TEXT("ColdOpen_Skip")), FOnClicked::CreateLambda([this]()
						{
							// Flown before: no locks, warnings-only guide (FTL's optional tutorial, Civ VII "only warnings").
							Mission->FirstFlight.bSkipped = true;
							Mission->FirstFlight.bColdOpenSeen = true;
							Mission->LogOnboarding(TEXT("Skip"));
							Rebuild();
							return FReply::Handled();
						}), true, Pal::Fill(), 14)
					]
				], Pal::Accent(), 28.f)
		]
	];
}

TSharedRef<SWidget> STDDesignRoom::BuildBeatStrip(const FTDDesignBudget& B)
{
	const UTDMissionSubsystem* M = Mission.Get();
	const EBeat Beat = GetBeat();
	const int32 MinCells = FMath::RoundToInt(M->GetConstant(TEXT("MinCells")));
	const FTDDesignIssueLine* MassLine = B.Issues.FindByPredicate([](const FTDDesignIssueLine& L) { return L.Issue == ETDDesignIssue::OverMass || L.Issue == ETDDesignIssue::ItemTooBig; });
	const FTDDesignIssueLine* PowerLine = B.Issues.FindByPredicate([](const FTDDesignIssueLine& L) { return L.Issue == ETDDesignIssue::PowerShort; });
	const double DataPct = B.DataMadeTotalMb > 0.0 ? B.DataReturnedMb / B.DataMadeTotalMb * 100.0 : 100.0;
	const FText Guesses[] = { Txt(TEXT("Guess_All")), Txt(TEXT("Guess_Most")), Txt(TEXT("Guess_Half")) };

	int32 Step = 1;
	switch (Beat)
	{
	case EBeat::Instrument: Step = 1; break;
	case EBeat::DataPredict:
	case EBeat::DataLink: Step = 2; break;
	case EBeat::Rockets: Step = 3; break;
	case EBeat::Power: Step = 4; break;
	case EBeat::StormPredict:
	case EBeat::StormPreview:
	case EBeat::Storm: Step = 5; break;
	case EBeat::Poll: Step = 6; break;
	default: Step = 7; break;
	}

	FText Line;
	TSharedRef<SWidget> Actions = SNullWidget::NullWidget;
	TSharedRef<SWidget> Extra = SNullWidget::NullWidget;
	auto GuessButton = [this](const FText& Text, TFunction<void()> OnPick) -> TSharedRef<SWidget>
	{
		return SNew(SBox).Padding(FMargin(0.f, 0.f, 8.f, 0.f))
		[
			Button(Text, FOnClicked::CreateLambda([this, OnPick]() { OnPick(); Rebuild(); return FReply::Handled(); }), true, Pal::Accent(), 15)
		];
	};

	switch (Beat)
	{
	case EBeat::Instrument:
		Line = Txt(TEXT("Beat_Instrument"));
		break;
	case EBeat::DataPredict:
	{
		// Predict, then observe (POE): one tap, never scored, shown next to the real result.
		Line = FText::Format(Txt(TEXT("Beat_DataPredict")), Num(B.DataMadeMb, 0), Num(B.DataLinkMb, 0));
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Row->AddSlot().AutoWidth()[GuessButton(Guesses[Index], [this, Index]()
			{
				Mission->FirstFlight.DataGuess = Index;
				Mission->LogOnboarding(TEXT("PredictData"), FString::FromInt(Index));
				SetBeat(EBeat::DataLink);
			})];
		}
		Actions = Row;
		break;
	}
	case EBeat::DataLink:
	{
		const int32 Guess = M->FirstFlight.DataGuess;
		Line = FText::Format(Txt(TEXT("Beat_DataLink")), Guesses[FMath::Clamp(Guess, 0, 2)], Num(DataPct, 0),
			FMath::RoundToInt(M->GetConstant(TEXT("ConjunctionSolStart"))), FMath::RoundToInt(M->GetConstant(TEXT("ConjunctionSolEnd"))));
		Actions = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
			[
				Button(FText::Format(Txt(TEXT("Beat_AddRelay")), Num(M->GetConstant(TEXT("RelayOrbiterCost")), 0)), FOnClicked::CreateLambda([this]()
				{
					Mission->SetDesignPick(TEXT("RelayOrbiter"), 1);
					Rebuild();
					return FReply::Handled();
				}), true, Pal::Accent(), 14)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				Button(Txt(TEXT("Guide_KeepDirect")), FOnClicked::CreateLambda([this]()
				{
					bDataStepSkipped = true;
					Mission->LogOnboarding(TEXT("DataLink"), TEXT("keep direct"));
					SetBeat(EBeat::Rockets, TEXT("Flight_Data"));
					Rebuild();
					return FReply::Handled();
				}), true, Pal::Line(), 14)
			];
		break;
	}
	case EBeat::Rockets:
		if (B.Launches == 0)
		{
			Line = Txt(TEXT("Beat_Rockets"));
		}
		else
		{
			Line = FText::Format(Txt(TEXT("Beat_TooHeavy")), Num(B.MassKg / 1000.0, 1), Num(B.CapacityKg / 1000.0, 1));
			if (MassLine) { Actions = FixChoices(*MassLine, true); }
		}
		break;
	case EBeat::Power:
		if (MassLine)
		{
			Line = Txt(TEXT("Beat_PowerMass"));
			Actions = FixChoices(*MassLine, true);
		}
		else
		{
			Line = FText::Format(Txt(TEXT("Beat_Power")), MinCells, Num(M->GetConstant(TEXT("CellKw")), 0));
			if (PowerLine) { Actions = FixChoices(*PowerLine, true); }
		}
		break;
	case EBeat::StormPredict:
	{
		Line = FText::Format(Txt(TEXT("Beat_StormPredict")), MinCells);
		const FText YesNo[] = { Txt(TEXT("Guess_Yes")), Txt(TEXT("Guess_No")) };
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		for (int32 Index = 0; Index < 2; ++Index)
		{
			Row->AddSlot().AutoWidth()[GuessButton(YesNo[Index], [this, Index]()
			{
				Mission->FirstFlight.StormGuess = Index;
				Mission->LogOnboarding(TEXT("PredictStorm"), Index == 0 ? TEXT("yes") : TEXT("no"));
				StormPreviewStart = FPlatformTime::Seconds();
				SetBeat(EBeat::StormPreview);
			})];
		}
		Actions = Row;
		break;
	}
	case EBeat::StormPreview:
		Extra = BuildStormPreview(B);
		break;
	case EBeat::Storm:
	{
		const FText Guess = M->FirstFlight.StormGuess == 0 ? Txt(TEXT("Guess_Yes")) : Txt(TEXT("Guess_No"));
		if (MassLine)
		{
			Line = Txt(TEXT("Beat_StormMass"));
			Actions = FixChoices(*MassLine, true);
		}
		else
		{
			Line = FText::Format(Txt(TEXT("Beat_Storm")), Guess, B.DustCells, MinCells);
			FTDDesignIssueLine Storm;
			Storm.Issue = ETDDesignIssue::PowerShort;
			Actions = FixChoices(Storm, true, true);
		}
		break;
	}
	case EBeat::Poll:
	{
		const FTDDesignIssueLine* Blocking = B.Issues.FindByPredicate([](const FTDDesignIssueLine& L) { return L.bBlocking; });
		if (Blocking)
		{
			Line = FText::Format(Txt(TEXT("Beat_Blocked")), IssueText(*Blocking));
			Actions = Blocking->Issue == ETDDesignIssue::OverMass || Blocking->Issue == ETDDesignIssue::ItemTooBig || Blocking->Issue == ETDDesignIssue::PowerShort
				? FixChoices(*Blocking, true) : FixButton(*Blocking, true);
		}
		else
		{
			Line = Txt(TEXT("Beat_Poll"));
		}
		if (M->FirstFlight.bWorkedExample)
		{
			// Worked example with its reasons (Sweller; Renkl backward fading on later flights).
			const int32 Heavy = UTDDesignRules::GetPickCount(M->GetDesign(), TEXT("FalconHeavy"));
			const int32 Units = UTDDesignRules::GetPickCount(M->GetDesign(), TEXT("Kilopower"));
			Extra = SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
				[
					Label(FText::Format(Txt(TEXT("Example_Rockets")), Heavy, Num(B.MassKg / 1000.0, 1), Num(B.CapacityKg / 1000.0, 1)), 14, Pal::Text())
				]
				+ SVerticalBox::Slot().AutoHeight()[Label(FText::Format(Txt(TEXT("Example_Power")), Units, B.Cells, B.DustCells, MinCells), 14, Pal::Text())]
				+ SVerticalBox::Slot().AutoHeight()[Label(FText::Format(Txt(TEXT("Example_Link")), Num(DataPct, 0)), 14, Pal::Text())];
		}
		break;
	}
	default:
		Line = Txt(TEXT("Beat_Launch"));
		break;
	}

	const int32 LandingPct = FMath::RoundToInt(M->GetConstant(TEXT("LandingHardwarePerPayloadKg")) * 100.0);
	const int32 DustPct = FMath::RoundToInt((1.0 - M->GetConstant(TEXT("DustStormSolarShare"))) * 100.0);
	TSharedRef<SVerticalBox> Middle = SNew(SVerticalBox);
	if (!NamedKey.IsNone())
	{
		// do -> see -> name: FLIGHT names the cause the player just saw (one line, never the only copy of an instruction).
		Middle->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 3.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)[Box(Label(Txt(TEXT("Flight_Tag")), 10, Pal::White(), true, false), Pal::Red(), 3.f)]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				Label(FText::Format(Txt(NamedKey), Num(M->GetConstant(TEXT("LandingHardwarePerPayloadKg")), 2), DustPct, LandingPct), 14, Pal::Text2(), true)
			]
		];
	}
	if (!Line.IsEmpty())
	{
		Middle->AddSlot().AutoHeight()[Label(Line, 19, Pal::Text(), true)];
	}
	Middle->AddSlot().AutoHeight()[Extra];
	Middle->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[BuildCausalMap(LitNodes(), FailingNodes(B), CurrentNode(), 11)];

	return Box(SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 14.f, 0.f)
		[
			SNew(SBox).WidthOverride(46.f).HeightOverride(46.f)
			[
				SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Pal::Red()).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::Format(FText::FromString(TEXT("{0}/7")), Step)).Font(NumFont(15)).ColorAndOpacity(Pal::White())
				]
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[Middle]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)[Actions], Pal::Raised(), 12.f);
}

TSharedRef<SWidget> STDDesignRoom::PollReports(const FTDDesignBudget& B, bool bAnimated)
{
	const UTDMissionSubsystem* M = Mission.Get();
	const double Start = PollStart;
	const double DataPct = B.DataMadeTotalMb > 0.0 ? B.DataReturnedMb / B.DataMadeTotalMb * 100.0 : 100.0;
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (int32 RoleIndex = 0; RoleIndex < TD::NumRoles; ++RoleIndex)
	{
		const ETDRole Role = static_cast<ETDRole>(RoleIndex);
		FText Report;
		switch (Role)
		{
		case ETDRole::Planner:
			Report = FText::Format(Txt(TEXT("Report_Planner")), Num(B.MassKg / 1000.0, 1), Num(B.CapacityKg / 1000.0, 1),
				Num(B.CapacityKg > 0.0 ? (1.0 - B.MassKg / B.CapacityKg) * 100.0 : 0.0, 0));
			break;
		case ETDRole::Engineer:
			Report = FText::Format(Txt(TEXT("Report_Engineer")), B.Cells, B.DustCells, FMath::RoundToInt(M->GetConstant(TEXT("MinCells"))));
			break;
		case ETDRole::Science:
			Report = FText::Format(Txt(TEXT("Report_Science")), Num(B.Science, 0), Num(M->GetConstant(TEXT("ScienceGoal")), 0), B.Instruments);
			break;
		default:
			Report = B.BudgetM > 0.0 ? FText::Format(Txt(TEXT("Report_Comms")), Num(DataPct, 0), Num(B.CostM, 0), Num(B.BudgetM, 0))
				: FText::Format(Txt(TEXT("Report_CommsNoBudget")), Num(DataPct, 0));
			break;
		}
		const bool bGoal = M->IsRoleGoalMet(Role);
		TAttribute<EVisibility> Vis = bAnimated
			? TAttribute<EVisibility>::CreateLambda([Start, RoleIndex]() { return FPlatformTime::Seconds() - Start >= (RoleIndex + 1) * PollStepSeconds ? EVisibility::Visible : EVisibility::Hidden; })
			: TAttribute<EVisibility>(EVisibility::Visible);
		Row->AddSlot().FillWidth(1.f).Padding(4.f, 0.f)
		[
			SNew(SBox).Visibility(Vis)
			[
				Box(SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
						[
							SNew(SBox).WidthOverride(6.f).HeightOverride(14.f)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(RoleColor(Role))]
						]
						+ SHorizontalBox::Slot().FillWidth(1.f)[Label(RoleName(Role), 12, Pal::Text(), true, false)]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							Box(Label(bGoal ? Txt(TEXT("Poll_Go")) : Txt(TEXT("Report_GoOpen")), 11, bGoal ? Pal::White() : Pal::Ink(), true, false),
								bGoal ? Pal::Go() : Pal::Caution(), 2.f)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f, 0.f, 0.f)[SNew(STextBlock).Text(Report).Font(NumFont(11)).ColorAndOpacity(Pal::Text()).AutoWrapText(true)],
					Pal::Raised(), 6.f)
			]
		];
	}
	return Row;
}

// Debrief: self-explanation before the answer (Mayer), on the same causal map the design room used.

FName STDDesignRoom::RootCause(FText& OutTrace) const
{
	const UTDMissionSubsystem* M = Mission.Get();
	const FTDDesignBudget& B = M->GetFlownBudget();
	const FTDMissionState& S = M->GetState();
	const int32 Need = FMath::RoundToInt(M->GetConstant(TEXT("MinCells")));
	const int32 DustPct = FMath::RoundToInt((1.0 - M->GetConstant(TEXT("DustStormSolarShare"))) * 100.0);
	const bool bDustHit = S.DrawnEventIds.ContainsByPredicate([M](const FName& Id)
	{
		const FTDEventRow* Row = M->GetEvent(Id);
		return Row && Row->bDustStorm;
	});
	const double DataPct = B.DataMadeTotalMb > 0.0 ? B.DataReturnedMb / B.DataMadeTotalMb * 100.0 : 100.0;
	const double Goal = M->GetConstant(TEXT("ScienceGoal"));
	if (S.Result == ETDMissionResult::LostAir || (bDustHit && B.DustCells < Need))
	{
		OutTrace = FText::Format(Txt(TEXT("Trace_Power")), DustPct, B.Cells, B.DustCells, Need);
		return TEXT("Power");
	}
	if (S.StormExposures > 0)
	{
		OutTrace = FText::Format(Txt(TEXT("Trace_Shelter")), S.StormExposures);
		return TEXT("Shelter");
	}
	if (DataPct < 95.0)
	{
		OutTrace = FText::Format(Txt(TEXT("Trace_Link")), Num(B.DataMadeMb, 0), Num(B.DataLinkMb, 0), FMath::RoundToInt(M->GetConstant(TEXT("ConjunctionSolStart"))),
			FMath::RoundToInt(M->GetConstant(TEXT("ConjunctionSolEnd"))), Num(DataPct, 0));
		return TEXT("Link");
	}
	if (B.Science < Goal)
	{
		OutTrace = FText::Format(Txt(TEXT("Trace_Instruments")), B.Instruments, Num(B.Science, 0), Num(Goal, 0));
		return TEXT("Instruments");
	}
	if (B.CapacityKg > 0.0 && B.MassKg / B.CapacityKg < 0.6)
	{
		OutTrace = FText::Format(Txt(TEXT("Trace_Rockets")), Num(B.MassKg / 1000.0, 1), Num(B.CapacityKg / 1000.0, 1));
		return TEXT("Rockets");
	}
	// Nothing broke: trace the closest call (the smallest margin), so the loop closes on every outcome.
	const double MassMargin = B.CapacityKg > 0.0 ? 1.0 - B.MassKg / B.CapacityKg : 1.0;
	const double PowerMargin = Need > 0 ? static_cast<double>(B.DustCells - Need) / Need : 1.0;
	if (PowerMargin <= MassMargin)
	{
		OutTrace = FText::Format(Txt(TEXT("Trace_ClosePower")), B.DustCells, Need);
		return TEXT("Power");
	}
	OutTrace = FText::Format(Txt(TEXT("Trace_CloseMass")), Num((1.0 - MassMargin) * 100.0, 0));
	return TEXT("Rockets");
}

TSharedRef<SWidget> STDDesignRoom::BuildDebriefPick()
{
	FText Trace;
	const FName Cause = RootCause(Trace);
	static const FName Order[] = { TEXT("Power"), TEXT("Link"), TEXT("Rockets"), TEXT("Instruments"), TEXT("Shelter") };
	TArray<FName> Choices = { Cause };
	for (const FName& Other : Order)
	{
		if (Other != Cause && Choices.Num() < 3)
		{
			Choices.Add(Other);
		}
	}
	// Same seed, same order; the right answer is not always first.
	const int32 Shift = FMath::Abs(Mission->GetState().Seed) % Choices.Num();
	for (int32 Index = 0; Index < Shift; ++Index)
	{
		const FName First = Choices[0];
		Choices.RemoveAt(0);
		Choices.Add(First);
	}
	const bool bPicked = !DebriefPick.IsNone();
	TSharedRef<SHorizontalBox> Buttons = SNew(SHorizontalBox);
	for (const FName& Choice : Choices)
	{
		const FLinearColor Color = !bPicked ? Pal::Accent() : Choice == Cause ? Pal::Go() : Choice == DebriefPick ? Pal::NoGo() : Pal::Line();
		Buttons->AddSlot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
		[
			Button(Txt(FName(*FString::Printf(TEXT("Decision_%s"), *Choice.ToString()))), FOnClicked::CreateLambda([this, Choice, Cause]()
			{
				if (DebriefPick.IsNone() && Mission.IsValid())
				{
					DebriefPick = Choice;
					Mission->LogOnboarding(TEXT("DebriefPick"), FString::Printf(TEXT("pick=%s root=%s"), *Choice.ToString(), *Cause.ToString()));
				}
				Rebuild();
				return FReply::Handled();
			}), true, Color, 15)
		];
	}
	TSharedRef<SVerticalBox> Body = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Txt(TEXT("Debrief_PickTitle"))).Font(HeadFont(20)).ColorAndOpacity(Pal::Text())]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 10.f)[Label(Txt(TEXT("Debrief_PickSub")), 14, Pal::Text2())]
		+ SVerticalBox::Slot().AutoHeight()[Buttons];
	if (bPicked)
	{
		TSet<FName> Path;
		if (Cause == TEXT("Power")) { Path = { TEXT("Dust"), TEXT("Power"), TEXT("Cells") }; }
		else if (Cause == TEXT("Link")) { Path = { TEXT("Instrument"), TEXT("Data"), TEXT("Link") }; }
		else if (Cause == TEXT("Rockets")) { Path = { TEXT("Payload"), TEXT("Mass"), TEXT("Rockets"), TEXT("Budget") }; }
		else if (Cause == TEXT("Instruments")) { Path = { TEXT("Objective"), TEXT("Instrument") }; }
		else { Path = { TEXT("Power"), TEXT("Cells") }; }
		TSet<FName> All = Path;
		for (const FName& Node : { FName(TEXT("Objective")), FName(TEXT("Instrument")), FName(TEXT("Data")), FName(TEXT("Link")), FName(TEXT("Payload")), FName(TEXT("Mass")),
			FName(TEXT("Rockets")), FName(TEXT("Budget")), FName(TEXT("Power")), FName(TEXT("Cells")), FName(TEXT("Dust")), FName(TEXT("GO")) })
		{
			All.Add(Node);
		}
		const bool bRight = DebriefPick == Cause;
		Body->AddSlot().AutoHeight().Padding(0.f, 12.f, 0.f, 4.f)
		[
			Label(bRight ? Txt(TEXT("Debrief_PickRight")) : FText::Format(Txt(TEXT("Debrief_PickWrong")), Txt(FName(*FString::Printf(TEXT("Decision_%s"), *Cause.ToString())))),
				16, bRight ? Pal::Go() : Pal::Caution(), true)
		];
		Body->AddSlot().AutoHeight().Padding(0.f, 2.f)[SNew(STextBlock).Text(Trace).Font(NumFont(13)).ColorAndOpacity(Pal::Text()).AutoWrapText(true)];
		Body->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[BuildCausalMap(All, Path, NAME_None, 10)];
		Body->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[Label(Txt(TEXT("Debrief_PickNext")), 13, Pal::Text2())];
	}
	return Glow(Box(Body, Panel(), 16.f), !bPicked);
}

// Transit

TSharedRef<SWidget> STDDesignRoom::BuildTransit()
{
	const UTDMissionSubsystem* M = Mission.Get();
	const FTDDesignBudget& B = M->GetFlownBudget();
	return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
	[
		SNew(SBox).WidthOverride(900.f)
		[
			SNew(SButton).ButtonStyle(FlatStyle()).ButtonColorAndOpacity(FLinearColor::Transparent).OnClicked(FOnClicked::CreateLambda([this]()
			{
				if (!bTransitDone)
				{
					bTransitDone = true;
					OnTransitDone.ExecuteIfBound();
				}
				return FReply::Handled();
			}))
			[
				Box(SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[Label(Txt(TEXT("Transit_Title")), 34, Pal::Text(), true)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f)
					[
						Label(FText::Format(Txt(TEXT("Transit_Launch")), B.Launches, Num(B.MassKg / 1000.0, 1)), 18, Pal::Text())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
					[
						Label(FText::Format(Txt(TEXT("Transit_Cruise")), Num(M->GetConstant(TEXT("MarsCruiseDays")), 0),
							Num(M->GetConstant(TEXT("PerseveranceCruiseDays")), 0)), 18, Pal::Text())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
					[
						Label(FText::Format(Txt(TEXT("Transit_Landing")), Num(B.LandingSystemKg / 1000.0, 1)), 18, Pal::Text())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
					[
						Label(FText::Format(Txt(TEXT("Transit_Crew")), B.Cells, B.DustCells), 18, Pal::Text())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)[Label(Txt(TEXT("Transit_Skip")), 13, Muted())],
					Panel(), 28.f)
			]
		]
	];
}

// Debrief

TSharedRef<SWidget> STDDesignRoom::BuildDebrief()
{
	UTDMissionSubsystem* M = Mission.Get();
	const FTDMissionScore Score = M->GetScore();
	const FTDDesignBudget& B = M->GetFlownBudget();
	const FTDMissionState& State = M->GetState();
	const FTDObjectiveRow* Objective = M->GetObjective();

	// Four axes first, the total last (Zachtronics: several measures beat one number).
	static const TCHAR* AxisKeys[] = { TEXT("Score_Success"), TEXT("Score_Science"), TEXT("Score_Efficiency"), TEXT("Score_Decisions") };
	const int32 Values[] = { Score.Success, Score.Science, Score.Efficiency, Score.Decisions };
	const int32 Weights[] = { Objective ? Objective->WeightSuccess : 0, Objective ? Objective->WeightScience : 0,
		Objective ? Objective->WeightEfficiency : 0, Objective ? Objective->WeightDecisions : 0 };
	TSharedRef<SVerticalBox> Axes = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[SNew(STextBlock).Text(Txt(TEXT("Debrief_Criteria"))).Font(HeadFont(20)).ColorAndOpacity(Pal::Text())];
	for (int32 Index = 0; Index < Score.RequirementKeys.Num(); ++Index)
	{
		const bool bOk = Score.RequirementMet.IsValidIndex(Index) && Score.RequirementMet[Index];
		Axes->AddSlot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 0.f, 8.f, 0.f)
			[
				Box(Label(bOk ? Txt(TEXT("Debrief_Pass")) : Txt(TEXT("Debrief_Fail")), 11, Pal::Ink(), true, false),
					bOk ? Pal::Go() : Pal::NoGo(), 4.f)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)[Label(Txt(Score.RequirementKeys[Index]), 14, Pal::Text())]
		];
	}
	Axes->AddSlot().AutoHeight().Padding(0.f, 14.f, 0.f, 10.f)[SNew(STextBlock).Text(Txt(TEXT("Debrief_Axes"))).Font(HeadFont(20)).ColorAndOpacity(Pal::Text())];
	for (int32 Axis = 0; Axis < 4; ++Axis)
	{
		const FText Why = Axis == 2 && Score.Efficiency == 0 && Score.RequirementMet.Contains(false) ? Txt(TEXT("Why_Eff_NotScored"))
			: Score.WhyKeys.IsValidIndex(Axis) ? Txt(Score.WhyKeys[Axis]) : FText::GetEmpty();
		Axes->AddSlot().AutoHeight()
		[
			BuildMeter(FText::Format(Txt(TEXT("Score_AxisLabel")), Txt(AxisKeys[Axis]), Weights[Axis]), Values[Axis], 100.0,
				FText::Format(FText::FromString(TEXT("{0} / 100")), Values[Axis]), false,
				Axis == 2 && Score.Efficiency == 0 && Score.RequirementMet.Contains(false) ? Txt(TEXT("Why_Eff_NotScored"))
					: Score.WhyKeys.IsValidIndex(Axis) ? Txt(Score.WhyKeys[Axis]) : FText::GetEmpty())
		];
		Axes->AddSlot().AutoHeight().Padding(0.f, -6.f, 0.f, 8.f)[Label(Why, 13, Muted())];
	}
	Axes->AddSlot().AutoHeight().Padding(0.f, 6.f)
	[
		Label(FText::Format(Txt(TEXT("Debrief_Total")), Score.Total), 26, Pal::Text(), true)
	];

	// What broke: trace each problem back to the design decision behind it.
	TSharedRef<SVerticalBox> Broke = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[SNew(STextBlock).Text(Txt(TEXT("Debrief_WhatBroke"))).Font(HeadFont(20)).ColorAndOpacity(Pal::Text())];
	// Each cause names the source its rule rests on (Source Panel id), so the why-chain is traceable.
	auto AddCause = [this, M, &Broke](const FText& Text, const FLinearColor& Color, const TCHAR* SourceId = nullptr)
	{
		FString Line = Text.ToString();
		if (SourceId)
		{
			const FTDSourceRow* Source = M->GetSource(FName(SourceId));
			Line += FString::Printf(TEXT("  [%s%s]"), SourceId, Source ? *(TEXT(": ") + Source->Title.Left(70)) : TEXT(""));
		}
		Broke->AddSlot().AutoHeight().Padding(0.f, 3.f)[Label(FText::FromString(Line), 14, Color)];
	};
	bool bAny = false;
	if (State.Result == ETDMissionResult::LostAir)
	{
		AddCause(FText::Format(Txt(TEXT("Broke_Air")), B.Cells), Pal::NoGo(), TEXT("BVAD2"));
		bAny = true;
	}
	if (State.StormExposures > 0)
	{
		AddCause(FText::Format(Txt(TEXT("Broke_Exposure")), State.StormExposures), Pal::NoGo(), TEXT("George2024"));
		bAny = true;
	}
	if (B.DustCells < B.Cells && State.DrawnEventIds.ContainsByPredicate([M](const FName& Id)
	{
		const FTDEventRow* Row = M->GetEvent(Id);
		return Row && Row->bDustStorm;
	}))
	{
		AddCause(FText::Format(Txt(TEXT("Broke_Dust")), B.Cells, B.DustCells), Pal::Caution(), TEXT("InSightPia25284"));
		bAny = true;
	}
	if (B.DataMadeTotalMb > 0.0 && B.DataReturnedMb < 0.95 * B.DataMadeTotalMb)
	{
		AddCause(FText::Format(Txt(TEXT("Broke_Data")), Num(B.DataReturnedMb / B.DataMadeTotalMb * 100.0, 0)), Pal::Caution(), TEXT("NasaMarsRelay"));
		bAny = true;
	}
	if (B.CapacityKg > 0.0 && B.MassKg / B.CapacityKg < 0.6)
	{
		AddCause(FText::Format(Txt(TEXT("Broke_EmptyRocket")), Num(100.0 - B.MassKg / B.CapacityKg * 100.0, 0)), Pal::Caution());
		bAny = true;
	}
	if (M->GetOps().RestBreaks > 0)
	{
		AddCause(FText::Format(Txt(TEXT("Broke_RestRule")), M->GetOps().RestBreaks), Pal::Caution(), TEXT("NasaHeomd415"));
		bAny = true;
	}
	if (!bAny)
	{
		AddCause(Txt(TEXT("Broke_Nothing")), Pal::Go());
	}

	// Compare: this session's flights, then real NASA missions (sourced numbers only).
	TSharedRef<SVerticalBox> Compare = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[SNew(STextBlock).Text(Txt(TEXT("Debrief_Flights"))).Font(HeadFont(20)).ColorAndOpacity(Pal::Text())];
	const TArray<FTDFlightRecord>& Flights = M->GetFlights();
	for (int32 Index = FMath::Max(0, Flights.Num() - 5); Index < Flights.Num(); ++Index)
	{
		const FTDFlightRecord& Flight = Flights[Index];
		const bool bThis = Index == Flights.Num() - 1;
		Compare->AddSlot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.45f)[Label(FText::FromString(FString::Printf(TEXT("%d. %s"), Index + 1, *Flight.DesignCode)), 12,
				bThis ? Pal::Accent() : Pal::Text(), bThis, false)]
			+ SHorizontalBox::Slot().FillWidth(0.45f).VAlign(VAlign_Center).Padding(6.f, 0.f)
			[
				SNew(SBox).HeightOverride(10.f)[Bar(Flight.Score.Total / 100.f, bThis ? Pal::Accent() : Muted())]
			]
			+ SHorizontalBox::Slot().FillWidth(0.1f)[Label(FText::AsNumber(Flight.Score.Total), 12, Pal::Text(), true, false)]
		];
	}
	if (Flights.Num() >= 2)
	{
		const FTDFlightRecord& Prev = Flights[Flights.Num() - 2];
		const FTDFlightRecord& Now = Flights.Last();
		const bool bSame = Prev.Seed == Now.Seed && Prev.ObjectiveId == Now.ObjectiveId && Prev.Design.bRealisticModel == Now.Design.bRealisticModel
			&& Prev.EventIds == Now.EventIds;
		auto Passed = [](const FTDMissionScore& S) { int32 N = 0; for (bool b : S.RequirementMet) { N += b ? 1 : 0; } return N; };
		TSharedRef<SVerticalBox> Table = SNew(SVerticalBox);
		auto Row = [this, &Table](const TCHAR* Key, const FString& A, const FString& B)
		{
			Table->AddSlot().AutoHeight().Padding(0.f, 1.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.5f)[Label(Txt(Key), 12, Muted(), false, false)]
				+ SHorizontalBox::Slot().FillWidth(0.25f)[Label(FText::FromString(A), 12, Pal::Text(), false, false)]
				+ SHorizontalBox::Slot().FillWidth(0.25f)[Label(FText::FromString(B), 12, Pal::Accent(), true, false)]
			];
		};
		Row(TEXT("Cmp_Row_Score"), FString::FromInt(Prev.Score.Total), FString::FromInt(Now.Score.Total));
		Row(TEXT("Cmp_Row_Criteria"), FString::Printf(TEXT("%d/%d"), Passed(Prev.Score), Prev.Score.RequirementMet.Num()),
			FString::Printf(TEXT("%d/%d"), Passed(Now.Score), Now.Score.RequirementMet.Num()));
		Row(TEXT("Cmp_Row_Mass"), FString::Printf(TEXT("%.1f/%.1f"), Prev.Budget.MassKg / 1000.0, Prev.Budget.CapacityKg / 1000.0),
			FString::Printf(TEXT("%.1f/%.1f"), Now.Budget.MassKg / 1000.0, Now.Budget.CapacityKg / 1000.0));
		Row(TEXT("Cmp_Row_Cost"), FString::Printf(TEXT("%.0f"), Prev.Budget.CostM), FString::Printf(TEXT("%.0f"), Now.Budget.CostM));
		Row(TEXT("Cmp_Row_Cells"), FString::Printf(TEXT("%d (%d)"), Prev.Budget.Cells, Prev.Budget.DustCells), FString::Printf(TEXT("%d (%d)"), Now.Budget.Cells, Now.Budget.DustCells));
		Row(TEXT("Cmp_Row_Data"), FString::Printf(TEXT("%.0f/%.0f"), Prev.Budget.DataReturnedMb, Prev.Budget.DataMadeTotalMb),
			FString::Printf(TEXT("%.0f/%.0f"), Now.Budget.DataReturnedMb, Now.Budget.DataMadeTotalMb));
		Row(TEXT("Cmp_Row_Walks"), FString::FromInt(Prev.ScienceWalks), FString::FromInt(Now.ScienceWalks));
		Row(TEXT("Cmp_Row_Decisions"), FString::Printf(TEXT("%d/%d"), Prev.GoodDecisions, Prev.Decisions), FString::Printf(TEXT("%d/%d"), Now.GoodDecisions, Now.Decisions));

		// What changed in the design (option counts).
		TArray<FString> Changes;
		TSet<FName> Ids;
		for (const FTDDesignPick& P : Prev.Design.Picks) { Ids.Add(P.OptionId); }
		for (const FTDDesignPick& P : Now.Design.Picks) { Ids.Add(P.OptionId); }
		for (const FName& Id : Ids)
		{
			const int32 A = UTDDesignRules::GetPickCount(Prev.Design, Id);
			const int32 C = UTDDesignRules::GetPickCount(Now.Design, Id);
			if (A != C)
			{
				Changes.Add(FString::Printf(TEXT("%s %d -> %d"), *Txt(FName(*FString::Printf(TEXT("Opt_%s"), *Id.ToString()))).ToString(), A, C));
			}
		}
		if (Prev.Design.bRealisticModel != Now.Design.bRealisticModel)
		{
			Changes.Add(Now.Design.bRealisticModel ? Txt(TEXT("Model_Realistic")).ToString() : Txt(TEXT("Model_Simplified")).ToString());
		}
		Compare->AddSlot().AutoHeight().Padding(0.f, 12.f, 0.f, 4.f)[SNew(STextBlock).Text(Txt(TEXT("Debrief_Compare"))).Font(HeadFont(18)).ColorAndOpacity(Pal::Text())];
		Compare->AddSlot().AutoHeight()[Label(bSame ? Txt(TEXT("Debrief_CompareSame")) : Txt(TEXT("Debrief_CompareDiff")), 12, bSame ? Pal::Go() : Pal::Caution())];
		Compare->AddSlot().AutoHeight().Padding(0.f, 4.f)
		[
			Label(Changes.Num() > 0 ? FText::Format(Txt(TEXT("Debrief_Changed")), FText::FromString(FString::Join(Changes, TEXT(", "))))
				: Txt(TEXT("Debrief_NoChange")), 13, Pal::Text(), true)
		];
		Compare->AddSlot().AutoHeight().Padding(0.f, 4.f)[Table];
	}
	Compare->AddSlot().AutoHeight().Padding(0.f, 12.f, 0.f, 6.f)[SNew(STextBlock).Text(Txt(TEXT("Debrief_Real"))).Font(HeadFont(18)).ColorAndOpacity(Pal::Text())];
	Compare->AddSlot().AutoHeight().Padding(0.f, 2.f)
	[
		Label(FText::Format(Txt(TEXT("Real_Perseverance")), Num(M->GetConstant(TEXT("PerseveranceRoverMass")), 0), Num(M->GetConstant(TEXT("MmrtgPowerBol")), 0),
			Num(M->GetConstant(TEXT("Mars2020BuildLaunchCost")), 1)), 13, Pal::Text())
	];
	Compare->AddSlot().AutoHeight().Padding(0.f, 2.f)
	[
		Label(FText::Format(Txt(TEXT("Real_Curiosity")), Num(M->GetConstant(TEXT("MslLaunchMassSumOfParts")), 0), Num(M->GetConstant(TEXT("CuriosityRoverMass")), 0),
			Num(M->GetConstant(TEXT("MslRoverShareOfLaunch")), 0)), 13, Pal::Text())
	];
	Compare->AddSlot().AutoHeight().Padding(0.f, 2.f)
	[
		Label(FText::Format(Txt(TEXT("Real_InSight")), Num(M->GetConstant(TEXT("InSightUsCost")), 1), Num(M->GetConstant(TEXT("InSightSolarWhPerSolStart")), 0),
			Num(M->GetConstant(TEXT("InSightSolarWhPerSol2022")), 0)), 13, Pal::Text())
	];
	Compare->AddSlot().AutoHeight().Padding(0.f, 2.f)
	[
		Label(FText::Format(Txt(TEXT("Real_CrewLander")), Num(M->GetConstant(TEXT("FlownEdlLandedMinT")), 1), Num(M->GetConstant(TEXT("FlownEdlLandedMaxT")), 0),
			Num(M->GetConstant(TEXT("HumanEdlLandedT")), 0), Num(M->GetConstant(TEXT("FalconHeavyToMars")) / 1000.0, 1)), 13, Pal::Text())
	];

	// Team debrief (group processing): which role goals held, and the decisions on the surface.
	TSharedRef<SVerticalBox> Team = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[SNew(STextBlock).Text(Txt(TEXT("Debrief_Team"))).Font(HeadFont(20)).ColorAndOpacity(Pal::Text())];
	for (int32 RoleIndex = 0; RoleIndex < TD::NumRoles; ++RoleIndex)
	{
		const ETDRole Role = static_cast<ETDRole>(RoleIndex);
		const bool bMet = UTDDesignRules::IsRoleGoalMet(Role, B, Objective, M->GetConstantsTable());
		Team->AddSlot().AutoHeight().Padding(0.f, 2.f)
		[
			Label(FText::Format(Txt(TEXT("Debrief_RoleLine")), RoleName(Role), bMet ? Txt(TEXT("Role_GoalMet")) : Txt(TEXT("Role_GoalOpen")),
				Txt(FName(*FString::Printf(TEXT("Goal_%s"), *StaticEnum<ETDRole>()->GetNameStringByValue(RoleIndex))))), 13,
				bMet ? Pal::Go() : Pal::Caution())
		];
	}
	Team->AddSlot().AutoHeight().Padding(0.f, 6.f)
	[
		Label(FText::Format(Txt(TEXT("Debrief_Decisions")), State.GoodDecisions, State.Decisions, State.ScienceWalks), 13, Pal::Text())
	];
	if (State.AdviceArrived > 0)
	{
		Team->AddSlot().AutoHeight().Padding(0.f, 2.f)[Label(FText::Format(Txt(TEXT("Debrief_Advice")), State.AdviceArrived, State.AdviceStale), 13, Pal::Text())];
	}
	Team->AddSlot().AutoHeight().Padding(0.f, 4.f)[Label(Txt(TEXT("Debrief_Question")), 15, Pal::Text(), true)];

	const FName ObjectiveId = M->GetFlights().Num() > 0 ? M->GetFlights().Last().ObjectiveId : NAME_None;
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			Box(SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[Label(Txt(TEXT("Debrief_Title")), 28, Pal::Text(), true, false)]
					+ SVerticalBox::Slot().AutoHeight()[Label(FText::Format(Txt(TEXT("Debrief_Sub")), Objective ? Txt(Objective->NameKey) : FText::GetEmpty(),
						FText::FromString(M->GetDesignCode()), FText::FromString(FString::FromInt(State.Seed))), 14, Muted(), false, false)]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f)
				[
					Button(Txt(TEXT("Debrief_Redesign")), FOnClicked::CreateLambda([this, ObjectiveId]()
					{
						if (Mission.IsValid())
						{
							FText Trace;
							const FName Cause = RootCause(Trace);
							Mission->FirstFlight.FocusSlot = DebriefPick.IsNone() ? -1 : static_cast<int32>(DecisionSlot(Cause));
						}
						if (Mission.IsValid() && Mission->BeginDesign(ObjectiveId, true))
						{
							Show(ETDDesignScreen::Design);
						}
						return FReply::Handled();
					}), true, Pal::Accent(), 16)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f)
				[
					Button(Txt(TEXT("Debrief_NewObjective")), FOnClicked::CreateLambda([this]()
					{
						Show(ETDDesignScreen::Objectives);
						return FReply::Handled();
					}), true, Pal::Text(), 16)
				], Panel(), 14.f)
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 12.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.34f).Padding(0.f, 0.f, 12.f, 0.f)[Box(SNew(SScrollBox) + SScrollBox::Slot()[Axes], Panel(), 16.f)]
			+ SHorizontalBox::Slot().FillWidth(0.33f).Padding(0.f, 0.f, 12.f, 0.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot().Padding(0.f, 0.f, 0.f, 12.f)[BuildDebriefPick()]
				+ SScrollBox::Slot().Padding(0.f, 0.f, 0.f, 12.f)[DebriefPick.IsNone() ? SNullWidget::NullWidget : Box(Broke, Panel(), 16.f)]
				+ SScrollBox::Slot()[Box(Team, Panel(), 16.f)]
			]
			+ SHorizontalBox::Slot().FillWidth(0.33f)[Box(SNew(SScrollBox) + SScrollBox::Slot()[Compare], Panel(), 16.f)]
		];
}

TSharedRef<SWidget> STDDesignRoom::BuildLaunchChecklist(const FTDDesignBudget& Budget)
{
	// NASA launch poll as the model (research note §5.6): hard NO-GO rows in red, each with its number and a FIX
	// that opens the console that owns it; the panel is always visible and re-evaluates on every change.
	const TArray<bool>& Go = Mission->GetDesign().SignedOff;
	int32 GoCount = 0;
	ETDRole FirstWaiting = ETDRole::Planner;
	bool bFoundWaiting = false;
	for (int32 RoleIndex = 0; RoleIndex < TD::NumRoles; ++RoleIndex)
	{
		const bool bGo = Go.IsValidIndex(RoleIndex) && Go[RoleIndex];
		GoCount += bGo ? 1 : 0;
		if (!bGo && !bFoundWaiting)
		{
			bFoundWaiting = true;
			FirstWaiting = static_cast<ETDRole>(RoleIndex);
		}
	}
	const FTDDesignIssueLine* Missing = Budget.Issues.FindByPredicate([](const FTDDesignIssueLine& L) { return L.Issue == ETDDesignIssue::MissingSlot; });
	int32 MissingCount = 0;
	int32 RedCount = 0;
	const FTDDesignIssueLine* FirstRed = nullptr;
	for (const FTDDesignIssueLine& L : Budget.Issues)
	{
		if (L.Issue == ETDDesignIssue::MissingSlot)
		{
			++MissingCount;
		}
		else if (L.bBlocking)
		{
			++RedCount;
			FirstRed = FirstRed ? FirstRed : &L;
		}
	}
	const bool bPollOpen = !IsFirstFlight() || static_cast<int32>(GetBeat()) >= static_cast<int32>(EBeat::Poll);

	TSharedRef<SHorizontalBox> Rows = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 16.f, 0.f)
		[
			SNew(STextBlock).Text(Txt(TEXT("Check_Title"))).Font(HeadFont(14)).ColorAndOpacity(Pal::Text())
		];
	int32 Number = 0;
	auto Row = [this, &Rows, &Number](bool bGo, const FText& Text, TFunction<void()> Fix)
	{
		++Number;
		TSharedRef<SHorizontalBox> Line = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
			[
				Box(SNew(STextBlock).Text(FText::Format(FText::FromString(TEXT("{0} {1}")), FText::AsNumber(Number), bGo ? Txt(TEXT("MC_Go")) : Txt(TEXT("MC_NoGo"))))
					.Font(NumFont(12)).ColorAndOpacity(Pal::White()), bGo ? Pal::Go() : Pal::NoGo(), 4.f)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				Label(Text, 13, bGo ? Pal::Text2() : Pal::Text(), !bGo, false)
			];
		if (!bGo && Fix)
		{
			Line->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f, 0.f, 0.f)
			[
				Button(Txt(TEXT("MC_Fix")), FOnClicked::CreateLambda([this, Fix]() { Fix(); Rebuild(); return FReply::Handled(); }), true, Pal::Red(), 12)
			];
		}
		Rows->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 18.f, 0.f)[Line];
	};
	const ETDRole MissingRole = Missing ? Missing->Role : ETDRole::Planner;
	Row(MissingCount == 0, MissingCount == 0 ? Txt(TEXT("Check_Slots")) : FText::Format(Txt(TEXT("Check_SlotsMissing")), MissingCount),
		[this, MissingRole]() { ActiveRole = MissingRole; });
	const ETDRole RedRole = FirstRed ? FirstRed->Role : ETDRole::Planner;
	Row(RedCount == 0, RedCount == 0 ? Txt(TEXT("Check_Red")) : FText::Format(Txt(TEXT("Check_RedOpen")), RedCount, FirstRed ? IssueText(*FirstRed) : FText::GetEmpty()),
		[this, RedRole]() { ActiveRole = RedRole; });
	if (!bPollOpen)
	{
		Row(false, Txt(TEXT("Check_Guided")), nullptr);
	}
	Row(GoCount == TD::NumRoles, FText::Format(Txt(TEXT("Check_Poll")), GoCount, TD::NumRoles),
		Budget.bLaunchable ? TFunction<void()>([this, FirstWaiting]() { ActiveRole = FirstWaiting; }) : TFunction<void()>());
	const bool bReady = Budget.ConceptMaturity >= 4 && bPollOpen;
	Rows->AddSlot().FillWidth(1.f)[SNullWidget::NullWidget];
	Rows->AddSlot().AutoWidth().VAlign(VAlign_Center)
	[
		Label(bReady ? Txt(TEXT("Check_Ready")) : FText::Format(Txt(TEXT("Check_NotReady")), (MissingCount > 0 ? 1 : 0) + (RedCount > 0 ? 1 : 0) + (GoCount < TD::NumRoles ? 1 : 0) + (bPollOpen ? 0 : 1)),
			15, bReady ? Pal::Go() : Pal::NoGo(), true, false)
	];
	return Rows;
}
