// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "UI/TDSurfaceHud.h"

#include "Actors/TDAstronautCharacter.h"
#include "Actors/TDPlayerController.h"
#include "Model/TDMissionRules.h"
#include "Rendering/DrawElements.h"
#include "Systems/TDMissionSubsystem.h"
#include "Systems/TDShotDirector.h"
#include "UI/TDDesignTokens.h"
#include "UI/TDObjective.h"
#include "UI/TDStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

using namespace TDStyle;

namespace
{
	/** Progress ring for hold-to-work: a track circle and the worked arc; flashes on every hit. */
	class STDHoldRing : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(STDHoldRing) {}
			SLATE_ATTRIBUTE(float, Progress)
			SLATE_ATTRIBUTE(float, Pulse)
			SLATE_ATTRIBUTE(bool, Station)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Progress = InArgs._Progress;
			Pulse = InArgs._Pulse;
			Station = InArgs._Station;
		}

		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(104.f, 104.f); }

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling, FSlateWindowElementList& Out,
			int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override
		{
			const FVector2D Size = Geometry.GetLocalSize();
			const FVector2D C = Size * 0.5f;
			const float P = Pulse.Get();
			const float R = 38.f + 5.f * P;
			auto Arc = [&](float From, float To, const FLinearColor& Color, float Thick, int32 Layer)
			{
				TArray<FVector2D> Points;
				const int32 Steps = FMath::Max(2, FMath::CeilToInt((To - From) * 64.f));
				for (int32 I = 0; I <= Steps; ++I)
				{
					const float A = (From + (To - From) * I / Steps) * 2.f * PI - PI * 0.5f;
					Points.Add(C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R);
				}
				FSlateDrawElement::MakeLines(Out, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Thick);
			};
			if (Station.Get())
			{
				Arc(0.f, 1.f, Hex(TEXT("FFFFFF"), 0.85f), 4.f, LayerId);
				return LayerId + 1;
			}
			Arc(0.f, 1.f, Hex(TEXT("15181C"), 0.55f), 9.f, LayerId);
			Arc(0.f, 1.f, Hex(TEXT("FAF9F6"), 0.35f), 7.f, LayerId + 1);
			const float V = FMath::Clamp(Progress.Get(), 0.f, 1.f);
			if (V > 0.001f)
			{
				const FLinearColor Fill = FLinearColor::LerpUsingHSV(Orange(), Hex(TEXT("FFE2A8")), P);
				Arc(0.f, V, Fill, 7.f + 3.f * P, LayerId + 2);
			}
			return LayerId + 3;
		}

	private:
		TAttribute<float> Progress;
		TAttribute<float> Pulse;
		TAttribute<bool> Station;
	};

	TSharedRef<SWidget> Card(const TSharedRef<SWidget>& Content, float Width)
	{
		// Light NASA-punk panel with a red edge, like the design room cards.
		return SNew(SBox).WidthOverride(Width)
		[
			SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Hex(TEXT("15181C"), 0.25f)).Padding(FMargin(0.f, 0.f, 2.f, 2.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(6.f)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Red())]]
				+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Hex(TEXT("EFEDE8"), 0.93f)).Padding(FMargin(16.f, 12.f))[Content]]
			]
		];
	}

	ATDAstronautCharacter* AstronautOf(const TWeakObjectPtr<ATDPlayerController>& PC)
	{
		return PC.IsValid() ? Cast<ATDAstronautCharacter>(PC->GetPawn()) : nullptr;
	}
}

void STDSurfaceHud::Construct(const FArguments& InArgs)
{
	Mission = InArgs._Mission;
	Controller = InArgs._Controller;
	SetVisibility(TAttribute<EVisibility>::CreateSP(this, &STDSurfaceHud::HudVisibility));
	ChildSlot
	[
		SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
		[
			SNew(SBox).WidthOverride(1920.f).HeightOverride(1080.f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(28.f, 26.f)[BuildObjective()]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(28.f, 26.f)[BuildLife()]
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(28.f, 26.f)[BuildComms()]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[BuildCentre()]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(28.f, 26.f)[BuildControls()]
			]
		]
	];
}

FText STDSurfaceHud::Txt(const TCHAR* Key) const
{
	return Mission.IsValid() ? Mission->GetText(FName(Key)) : FText::FromString(Key);
}

EVisibility STDSurfaceHud::HudVisibility() const
{
	const UTDMissionSubsystem* M = Mission.Get();
	const ATDPlayerController* PC = Controller.Get();
	if (!M || !PC || !M->HasMission() || !M->IsMissionRunning() || PC->IsDesignScreenOpen() || PC->IsBaseComputerOpen() || !AstronautOf(Controller))
	{
		return EVisibility::Collapsed;
	}
	if (const UTDShotDirector* Director = UTDShotDirector::Get(PC); Director && Director->IsPlayingShot())
	{
		return EVisibility::Collapsed;
	}
	return EVisibility::HitTestInvisible;
}

TSharedRef<SWidget> STDSurfaceHud::BuildObjective()
{
	auto Objective = [this]() { return TDObjective::Compute(Mission.Get(), AstronautOf(Controller)); };
	auto PhaseLine = [this]()
	{
		const UTDMissionSubsystem* M = Mission.Get();
		if (!M || !M->HasMission())
		{
			return FText::GetEmpty();
		}
		int32 A = 0;
		int32 B = 0;
		M->GetPhaseSols(M->GetState().PhaseIndex, A, B);
		const FTDPhaseRow* Phase = M->GetPhaseRow(M->GetState().PhaseIndex);
		return FText::Format(M->GetText(TEXT("Ops_Sols")), A, B, Phase ? M->GetText(Phase->NameKey) : FText::GetEmpty());
	};
	return Card(SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(STextBlock).Text(Txt(TEXT("Track_Header"))).Font(HeadFont(12)).ColorAndOpacity(Text2())]
			+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text_Lambda(PhaseLine).Font(NumFont(12)).ColorAndOpacity(Red())]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 2.f)
		[
			SNew(STextBlock).Font(Font(25, true)).AutoWrapText(true)
			.Text_Lambda([this, Objective]()
			{
				const FTDObjective O = Objective();
				// A changed objective slides in with a flash (research note §6.3).
				const FString Key = O.Title.ToString();
				if (Key != LastObjective)
				{
					LastObjective = Key;
					ObjectiveChanged = FPlatformTime::Seconds();
					if (Mission.IsValid() && !Key.IsEmpty()) { Mission->PlaySfxPitched(TEXT("UI_Hover"), 1.2f, 0.7f); }
				}
				return O.Title;
			})
			.ColorAndOpacity_Lambda([this, Objective]()
			{
				const float Since = float(FPlatformTime::Seconds() - ObjectiveChanged);
				const FLinearColor Base = Objective().bUrgent ? Red() : Text();
				return FSlateColor(Since < 0.6f ? FLinearColor::LerpUsingHSV(Orange(), Base, Since / 0.6f) : Base);
			})
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Font(Font(17)).AutoWrapText(true).ColorAndOpacity(Text())
			.Text_Lambda([Objective]() { return Objective().Step; })
		], 560.f);
}

TSharedRef<SWidget> STDSurfaceHud::BuildLife()
{
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[SNew(STextBlock).Text(Txt(TEXT("HUD_LifeSupport"))).Font(HeadFont(12)).ColorAndOpacity(Text2())];
	for (ETDSystem System : { ETDSystem::Air, ETDSystem::Greenhouse, ETDSystem::Shelter })
	{
		const int32 Index = static_cast<int32>(System);
		auto Level = [this, Index]() { return Mission.IsValid() && Mission->GetState().Levels.IsValidIndex(Index) ? Mission->GetState().Levels[Index] : 0; };
		TSharedRef<SHorizontalBox> Bars = SNew(SHorizontalBox);
		for (int32 Seg = 0; Seg < 4; ++Seg)
		{
			Bars->AddSlot().AutoWidth().Padding(0.f, 0.f, 3.f, 0.f)
			[
				SNew(SBox).WidthOverride(26.f).HeightOverride(12.f)
				[
					SNew(SBorder).BorderImage(White()).BorderBackgroundColor_Lambda([Level, Seg, System]()
					{
						const int32 L = Level();
						if (Seg >= L) { return FSlateColor(Track()); }
						// Danger flashes.
						const float Blink = L <= 1 ? 0.55f + 0.45f * FMath::Sin(float(FPlatformTime::Seconds()) * 8.f) : 1.f;
						FLinearColor C = TDTokens::LevelColor(System, L);
						C.A = Blink;
						return FSlateColor(C);
					})
				]
			];
		}
		Rows->AddSlot().AutoHeight().Padding(0.f, 3.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SBox).WidthOverride(8.f).HeightOverride(18.f)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(TDTokens::System(System))]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[SNew(STextBlock).Text(Txt(*TDTokens::SystemTextKey(System).ToString())).Font(Font(15, true)).ColorAndOpacity(Text())]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f)[Bars]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(74.f)
				[
					SNew(STextBlock).Font(NumFont(12)).ColorAndOpacity(Text2())
					.Text_Lambda([this, Level]() { return Mission.IsValid() ? Mission->GetText(FName(*FString::Printf(TEXT("Level_%d"), Level()))) : FText::GetEmpty(); })
				]
			]
		];
	}
	// Power cells: tokens, filled = placed.
	TSharedRef<SHorizontalBox> Cells = SNew(SHorizontalBox);
	for (int32 Cell = 0; Cell < 12; ++Cell)
	{
		Cells->AddSlot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
		[
			SNew(SBox).WidthOverride(16.f).HeightOverride(16.f)
			.Visibility_Lambda([this, Cell]() { return Mission.IsValid() && Cell < Mission->GetState().CellsTotal ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				SNew(SBorder).BorderImage(White()).BorderBackgroundColor_Lambda([this, Cell]()
				{
					const bool bUsed = Mission.IsValid() && Cell < UTDMissionRules::GetCellsPlaced(Mission->GetState());
					return FSlateColor(bUsed ? Charcoal() : Orange());
				})
			]
		];
	}
	Rows->AddSlot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Font(HeadFont(12)).ColorAndOpacity(Text2())
			.Text_Lambda([this]()
			{
				const UTDMissionSubsystem* M = Mission.Get();
				return M ? FText::Format(M->GetText(TEXT("HUD_Cells")), UTDMissionRules::GetCellsPlaced(M->GetState()), M->GetState().CellsTotal) : FText::GetEmpty();
			})
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Cells]
	];
	// Jezero weather of the phase's first sol (MEDA archive; labelled ARCHIVE, never live).
	Rows->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
	[
		SNew(STextBlock).Font(NumFont(11)).ColorAndOpacity(Text2()).AutoWrapText(true)
		.Text_Lambda([this]()
		{
			const UTDMissionSubsystem* M = Mission.Get();
			const FTDPhaseRow* PhaseRow = M ? M->GetPhaseRow(M->GetState().PhaseIndex) : nullptr;
			const FTDWeatherRow* Weather = PhaseRow ? M->GetWeather(PhaseRow->SolStart) : nullptr;
			if (!Weather)
			{
				return FText::GetEmpty();
			}
			FNumberFormattingOptions NoGroup;
			NoGroup.UseGrouping = false;
			NoGroup.MaximumFractionalDigits = 0;
			return FText::Format(M->GetText(TEXT("Weather_Line")), FText::AsNumber(Weather->MedaSol, &NoGroup), FText::FromString(Weather->EarthDate),
				FText::AsNumber(FMath::RoundToInt(Weather->AirMinC)), FText::AsNumber(FMath::RoundToInt(Weather->AirMaxC)),
				FText::AsNumber(FMath::RoundToInt(Weather->PressureMeanPa), &NoGroup));
		})
	];
	return Card(Rows, 420.f);
}

TSharedRef<SWidget> STDSurfaceHud::BuildComms()
{
	TSharedRef<SVerticalBox> Lines = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SBox).WidthOverride(10.f).HeightOverride(10.f)
				[
					SNew(SBorder).BorderImage(White()).BorderBackgroundColor_Lambda([this]()
					{
						// Radio lamp: lit for a few seconds after a new line.
						const bool bFresh = Mission.IsValid() && FPlatformTime::Seconds() - Mission->GetLastCommsTime() < 4.0;
						return FSlateColor(bFresh ? Go() : Track());
					})
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(Txt(TEXT("HUD_Comms"))).Font(HeadFont(12)).ColorAndOpacity(Text2())]
		];
	for (int32 Back = 2; Back >= 0; --Back)
	{
		Lines->AddSlot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(STextBlock).AutoWrapText(true).Font(Font(Back == 0 ? 16 : 14, Back == 0))
			.ColorAndOpacity(Back == 0 ? Text() : Text2())
			.Text_Lambda([this, Back]()
			{
				const TArray<FText>& All = Mission.IsValid() ? Mission->GetComms() : TArray<FText>();
				return All.IsValidIndex(All.Num() - 1 - Back) ? All[All.Num() - 1 - Back] : FText::GetEmpty();
			})
		];
	}
	return SNew(SBox).Visibility_Lambda([this]() { return Mission.IsValid() && Mission->GetComms().Num() > 0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
	[
		Card(Lines, 600.f)
	];
}

TSharedRef<SWidget> STDSurfaceHud::BuildCentre()
{
	auto Work = [this]() { const ATDAstronautCharacter* A = AstronautOf(Controller); return A ? A->GetWorkUi() : FTDWorkUi(); };
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SBox).WidthOverride(104.f).HeightOverride(104.f)
			.Visibility_Lambda([Work]() { return Work().bShow ? EVisibility::HitTestInvisible : EVisibility::Hidden; })
			[
				SNew(STDHoldRing)
				.Progress_Lambda([Work]() { return Work().Ring; })
				.Pulse_Lambda([Work]() { return Work().BeatPulse; })
				.Station_Lambda([Work]() { return Work().bStation; })
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 70.f, 0.f, 0.f)
		[
			SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Hex(TEXT("15181C"), 0.88f)).Padding(FMargin(18.f, 9.f))
			.Visibility_Lambda([Work]() { return Work().bShow ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[
				SNew(STextBlock).Font(Font(19, true)).ColorAndOpacity(Hex(TEXT("FAF9F6")))
				.Text_Lambda([Work]() { return Work().Prompt; })
			]
		]
		// TAB / Y away from the console: say where Mission Control is.
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 14.f, 0.f, 0.f)
		[
			SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Red()).Padding(FMargin(18.f, 9.f))
			.Visibility_Lambda([this]()
			{
				const ATDAstronautCharacter* A = AstronautOf(Controller);
				return A && FPlatformTime::Seconds() - A->GetStationPingTime() < 3.5 ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
			[
				SNew(STextBlock).Text(Txt(TEXT("MC_Far"))).Font(Font(18, true)).ColorAndOpacity(Hex(TEXT("FFFFFF")))
			]
		];
}

TSharedRef<SWidget> STDSurfaceHud::BuildControls()
{
	return SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Hex(TEXT("15181C"), 0.55f)).Padding(FMargin(12.f, 7.f))
	[
		SNew(STextBlock).Text(Txt(TEXT("HUD_Controls"))).Font(NumFont(12)).ColorAndOpacity(Hex(TEXT("FAF9F6"), 0.9f))
	];
}
