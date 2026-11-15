// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "UI/TDScreenWidget.h"

#include "Engine/World.h"
#include "Model/TDMissionRules.h"
#include "Rendering/DrawElements.h"
#include "Systems/TDMissionSubsystem.h"
#include "UI/TDDesignTokens.h"
#include "UI/TDStyle.h"

namespace
{
	// Screen palette: a dark glass face with the HUD's system colours on top. [DESIGNER VALUE]
	const FLinearColor ScreenBg = TDStyle::Hex(TEXT("0A1220"));
	const FLinearColor ScreenPanel = TDStyle::Hex(TEXT("132036"));
	const FLinearColor ScreenLine = TDStyle::Hex(TEXT("27405F"));
	const FLinearColor ScreenText = TDStyle::Hex(TEXT("E8F1FA"));
	const FLinearColor ScreenText2 = TDStyle::Hex(TEXT("8FA6BF"));
	const FLinearColor ScreenGlow = TDStyle::Hex(TEXT("5FD4FF"));

	struct FPainter
	{
		const FGeometry& Geo;
		FSlateWindowElementList& Out;

		void Box(int32 Layer, const FVector2D& Pos, const FVector2D& Size, const FLinearColor& Color) const
		{
			if (Size.X <= 0.f || Size.Y <= 0.f) { return; }
			FSlateDrawElement::MakeBox(Out, Layer, Geo.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), TDStyle::White(), ESlateDrawEffect::None, Color);
		}
		void Frame(int32 Layer, const FVector2D& Pos, const FVector2D& Size, const FLinearColor& Color, float T = 2.f) const
		{
			Box(Layer, Pos, FVector2D(Size.X, T), Color);
			Box(Layer, Pos + FVector2D(0.f, Size.Y - T), FVector2D(Size.X, T), Color);
			Box(Layer, Pos, FVector2D(T, Size.Y), Color);
			Box(Layer, Pos + FVector2D(Size.X - T, 0.f), FVector2D(T, Size.Y), Color);
		}
		void Text(int32 Layer, const FVector2D& Pos, const FString& S, const FSlateFontInfo& Font, const FLinearColor& Color) const
		{
			FSlateDrawElement::MakeText(Out, Layer, Geo.ToPaintGeometry(FVector2D(2000.f, Font.Size * 2.f), FSlateLayoutTransform(Pos)), S, Font, ESlateDrawEffect::None, Color);
		}
		void Lines(int32 Layer, const TArray<FVector2D>& Points, const FLinearColor& Color, float Thick) const
		{
			FSlateDrawElement::MakeLines(Out, Layer, Geo.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Thick);
		}
	};

	FString SystemLabel(int32 Index)
	{
		switch (Index)
		{
		case 0: return TEXT("AIR");
		case 1: return TEXT("GREENHOUSE");
		default: return TEXT("SHELTER");
		}
	}

	FString LevelWord(int32 Level)
	{
		return Level <= 0 ? TEXT("DOWN") : Level == 1 ? TEXT("DANGER") : Level == 2 ? TEXT("LOW") : TEXT("GOOD");
	}

	// Glass face shared by both screens: base, a slow bright scan band and faint raster lines.
	void PaintGlass(const FPainter& P, int32 Layer, const FVector2D& Size, float Now)
	{
		P.Box(Layer, FVector2D::ZeroVector, Size, ScreenBg);
		for (float Y = 0.f; Y < Size.Y; Y += 6.f)
		{
			P.Box(Layer, FVector2D(0.f, Y), FVector2D(Size.X, 1.f), FLinearColor(1.f, 1.f, 1.f, 0.025f));
		}
		const float Band = FMath::Fmod(Now * 0.25f, 1.3f) - 0.15f;
		P.Box(Layer, FVector2D(0.f, Band * Size.Y), FVector2D(Size.X, Size.Y * 0.08f), FLinearColor(0.37f, 0.83f, 1.f, 0.05f));
	}
}

void UTDScreenWidget::SetTablet(const FText& InTitle, float InProgress, int32 InSpotsDone, int32 InSpotsTotal, bool bInWorking)
{
	Title = InTitle;
	Progress = FMath::Clamp(InProgress, 0.f, 1.f);
	SpotsDone = InSpotsDone;
	SpotsTotal = InSpotsTotal;
	bWorking = bInWorking;
}

int32 UTDScreenWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
	FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	const int32 Base = Super::NativePaint(Args, Geometry, Culling, Out, LayerId, Style, bParentEnabled);
	const UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;
	return Mode == ETDScreenMode::Console ? PaintConsole(Geometry, Out, Base + 1, Now) : PaintTablet(Geometry, Out, Base + 1, Now);
}

int32 UTDScreenWidget::PaintConsole(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 L, float Now) const
{
	const FPainter P{ Geometry, Out };
	const FVector2D Size = Geometry.GetLocalSize();
	PaintGlass(P, L, Size, Now);
	const UWorld* World = GetWorld();
	const UTDMissionSubsystem* Mission = World ? World->GetSubsystem<UTDMissionSubsystem>() : nullptr;
	const float Blink = 0.5f + 0.5f * FMath::Sin(Now * 5.f);

	// Header strip.
	P.Box(L + 1, FVector2D::ZeroVector, FVector2D(Size.X, 58.f), ScreenPanel);
	P.Box(L + 1, FVector2D(0.f, 58.f), FVector2D(Size.X, 3.f), TDStyle::Orange());
	P.Text(L + 2, FVector2D(26.f, 10.f), TEXT("MISSION CONTROL"), TDStyle::HeadFont(24), ScreenText);
	P.Text(L + 2, FVector2D(330.f, 16.f), TEXT("JEZERO OUTPOST"), TDStyle::Font(18), ScreenText2);
	P.Box(L + 2, FVector2D(Size.X - 250.f, 22.f), FVector2D(14.f, 14.f), TDStyle::Green() * (0.4f + 0.6f * Blink));

	if (!Mission || !Mission->HasMission())
	{
		// Standby: a turning dial while the mission is being set up.
		P.Text(L + 2, FVector2D(Size.X - 226.f, 12.f), TEXT("STANDBY"), TDStyle::NumFont(22), TDTokens::CoreAccent());
		const FVector2D C(Size.X * 0.5f, Size.Y * 0.55f);
		TArray<FVector2D> Arc;
		for (int32 I = 0; I <= 24; ++I)
		{
			const float A = Now * 2.5f + I / 24.f * PI * 1.4f;
			Arc.Add(C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * 60.f);
		}
		P.Lines(L + 2, Arc, ScreenGlow, 6.f);
		P.Text(L + 2, C + FVector2D(-120.f, 90.f), TEXT("LINKING SYSTEMS"), TDStyle::HeadFont(20), ScreenText2);
		return L + 3;
	}

	const FTDMissionState& S = Mission->GetState();
	P.Text(L + 2, FVector2D(Size.X - 226.f, 12.f), FString::Printf(TEXT("SOL %d"), S.SolsElapsed + 1), TDStyle::NumFont(24), TDTokens::CoreAccent());

	// Left: life support, one row per system with four level segments.
	const float Top = 86.f;
	P.Text(L + 2, FVector2D(26.f, Top), TEXT("LIFE SUPPORT"), TDStyle::HeadFont(15), ScreenText2);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const ETDSystem System = static_cast<ETDSystem>(Index);
		const int32 Level = S.Levels.IsValidIndex(Index) ? S.Levels[Index] : 0;
		const float Y = Top + 36.f + Index * 92.f;
		P.Box(L + 1, FVector2D(22.f, Y), FVector2D(396.f, 80.f), ScreenPanel);
		P.Box(L + 2, FVector2D(22.f, Y), FVector2D(6.f, 80.f), TDTokens::System(System));
		P.Text(L + 2, FVector2D(42.f, Y + 8.f), SystemLabel(Index), TDStyle::HeadFont(17), ScreenText);
		const FLinearColor LevelColor = TDTokens::LevelColor(System, Level);
		const bool bBad = Level <= 1;
		P.Text(L + 2, FVector2D(300.f, Y + 9.f), LevelWord(Level), TDStyle::NumFont(16), LevelColor * (bBad ? 0.5f + 0.5f * Blink : 1.f));
		for (int32 Seg = 0; Seg < 4; ++Seg)
		{
			const bool bOn = Seg < Level;
			P.Box(L + 2, FVector2D(42.f + Seg * 92.f, Y + 44.f), FVector2D(84.f, 22.f), bOn ? LevelColor : ScreenLine);
		}
	}

	// Middle: power cells routed this phase.
	const float MidX = 446.f;
	const int32 Placed = UTDMissionRules::GetCellsPlaced(S);
	P.Text(L + 2, FVector2D(MidX, Top), TEXT("POWER CELLS"), TDStyle::HeadFont(15), ScreenText2);
	P.Text(L + 2, FVector2D(MidX, Top + 26.f), FString::Printf(TEXT("%d / %d ROUTED"), Placed, S.CellsTotal), TDStyle::NumFont(22), ScreenText);
	{
		int32 Cell = 0;
		for (int32 Sys = 0; Sys < 3; ++Sys)
		{
			const int32 N = S.Allocation.IsValidIndex(Sys) ? S.Allocation[Sys] : 0;
			for (int32 K = 0; K < N && Cell < 12; ++K, ++Cell)
			{
				const FVector2D At(MidX + (Cell % 4) * 74.f, Top + 74.f + (Cell / 4) * 74.f);
				P.Box(L + 2, At, FVector2D(62.f, 62.f), TDTokens::System(static_cast<ETDSystem>(Sys)));
			}
		}
		for (; Cell < FMath::Min(S.CellsTotal, 12); ++Cell)
		{
			const FVector2D At(MidX + (Cell % 4) * 74.f, Top + 74.f + (Cell / 4) * 74.f);
			P.Box(L + 1, At, FVector2D(62.f, 62.f), ScreenPanel);
			P.Frame(L + 2, At, FVector2D(62.f, 62.f), TDStyle::Orange() * (0.35f + 0.65f * Blink), 3.f);
		}
	}

	// Right: trend traces, one per system; calmer and higher when the system is healthy (cosmetic, not data).
	const FVector2D G(MidX + 316.f, Top + 26.f);
	const FVector2D GS(Size.X - G.X - 22.f, 238.f);
	P.Text(L + 2, FVector2D(G.X, Top), TEXT("SYSTEM TRENDS"), TDStyle::HeadFont(15), ScreenText2);
	P.Box(L + 1, G, GS, ScreenPanel);
	for (int32 K = 1; K < 4; ++K)
	{
		P.Box(L + 2, G + FVector2D(0.f, GS.Y * K / 4.f), FVector2D(GS.X, 1.f), ScreenLine);
		P.Box(L + 2, G + FVector2D(GS.X * K / 4.f, 0.f), FVector2D(1.f, GS.Y), ScreenLine);
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const ETDSystem System = static_cast<ETDSystem>(Index);
		const int32 Level = S.Levels.IsValidIndex(Index) ? S.Levels[Index] : 0;
		const float BaseY = G.Y + GS.Y * (0.9f - 0.2f * Level);
		const float Amp = Level <= 1 ? 18.f : 7.f;
		TArray<FVector2D> Trace;
		for (int32 I = 0; I <= 60; ++I)
		{
			const float X = I / 60.f;
			const float T = Now * (0.6f + 0.25f * Index) + X * 9.f;
			const float Y = BaseY + Amp * (FMath::Sin(T * 1.7f + Index) * 0.6f + FMath::Sin(T * 4.3f + Index * 2.f) * 0.4f);
			Trace.Add(FVector2D(G.X + X * GS.X, FMath::Clamp(Y, G.Y + 4.f, G.Y + GS.Y - 4.f)));
		}
		P.Lines(L + 3, Trace, TDTokens::System(System), 3.f);
	}
	const float Sweep = FMath::Fmod(Now * 0.4f, 1.f);
	P.Box(L + 3, G + FVector2D(GS.X * Sweep, 0.f), FVector2D(2.f, GS.Y), ScreenGlow * 0.8f);

	// Footer: what Mission Control wants, and the Earth link delay (a stored ephemeris value, never live).
	const float FY = Size.Y - 64.f;
	const bool bEnded = !Mission->IsMissionRunning();
	const FLinearColor Status = bEnded ? ScreenGlow : bNeedsYou ? TDStyle::Orange() : TDStyle::Green();
	P.Box(L + 1, FVector2D(0.f, FY), FVector2D(Size.X, 64.f), ScreenPanel);
	P.Box(L + 2, FVector2D(22.f, FY + 14.f), FVector2D(36.f, 36.f), Status * (bNeedsYou ? 0.35f + 0.65f * Blink : 1.f));
	const FString StatusText = bEnded ? TEXT("MISSION ENDED - DEBRIEF READY")
		: bNeedsYou ? TEXT("ACTION NEEDED - ROUTE POWER / DECIDE")
		: Mission->IsRunningPhase() ? TEXT("SOL IN PROGRESS - CREW AT WORK") : TEXT("SYSTEMS NOMINAL");
	P.Text(L + 2, FVector2D(74.f, FY + 14.f), StatusText, TDStyle::HeadFont(20), ScreenText);
	const double Delay = Mission->GetConstant(TEXT("LightTimeNow"));
	if (Delay > 0.0)
	{
		P.Text(L + 2, FVector2D(Size.X - 430.f, FY + 18.f), FString::Printf(TEXT("EARTH LINK %.1f MIN ONE-WAY (14 NOV)"), Delay), TDStyle::NumFont(15), ScreenText2);
	}
	return L + 4;
}

int32 UTDScreenWidget::PaintTablet(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 L, float Now) const
{
	const FPainter P{ Geometry, Out };
	const FVector2D Size = Geometry.GetLocalSize();
	PaintGlass(P, L, Size, Now);
	const float Blink = 0.5f + 0.5f * FMath::Sin(Now * 6.f);

	// Header.
	P.Box(L + 1, FVector2D::ZeroVector, FVector2D(Size.X, 44.f), ScreenPanel);
	P.Box(L + 1, FVector2D(0.f, 44.f), FVector2D(Size.X, 3.f), TDStyle::Orange());
	P.Text(L + 2, FVector2D(16.f, 8.f), TEXT("FIELD TABLET"), TDStyle::HeadFont(16), ScreenText);
	// Battery and signal marks.
	P.Frame(L + 2, FVector2D(Size.X - 70.f, 13.f), FVector2D(44.f, 20.f), ScreenText2, 2.f);
	P.Box(L + 2, FVector2D(Size.X - 66.f, 17.f), FVector2D(30.f, 12.f), TDStyle::Green());
	for (int32 K = 0; K < 3; ++K)
	{
		P.Box(L + 2, FVector2D(Size.X - 112.f + K * 9.f, 30.f - K * 6.f), FVector2D(6.f, 6.f + K * 6.f), ScreenGlow);
	}

	// Task line.
	P.Text(L + 2, FVector2D(16.f, 58.f), Title.ToString().ToUpper(), TDStyle::HeadFont(17), ScreenText);

	// Beat progress bar.
	const FVector2D BarAt(16.f, 104.f);
	const FVector2D BarSize(Size.X - 32.f, 40.f);
	P.Box(L + 1, BarAt, BarSize, ScreenPanel);
	const FLinearColor Fill = Progress >= 1.f ? TDStyle::Green() : TDStyle::Orange();
	P.Box(L + 2, BarAt + FVector2D(4.f, 4.f), FVector2D((BarSize.X - 8.f) * Progress, BarSize.Y - 8.f), Fill);
	P.Text(L + 3, BarAt + FVector2D(BarSize.X - 84.f, 8.f), FString::Printf(TEXT("%3d%%"), FMath::RoundToInt(Progress * 100.f)), TDStyle::NumFont(18), ScreenText);

	// Spots of this task: done green, current pulsing.
	const int32 Count = FMath::Clamp(SpotsTotal, 0, 8);
	for (int32 K = 0; K < Count; ++K)
	{
		const FVector2D At(16.f + K * 40.f, 160.f);
		if (K < SpotsDone)
		{
			P.Box(L + 2, At, FVector2D(28.f, 28.f), TDStyle::Green());
		}
		else if (K == SpotsDone)
		{
			P.Box(L + 2, At, FVector2D(28.f, 28.f), TDStyle::Orange() * (0.4f + 0.6f * Blink));
		}
		else
		{
			P.Frame(L + 2, At, FVector2D(28.f, 28.f), ScreenText2, 2.f);
		}
	}
	P.Text(L + 2, FVector2D(Size.X - 150.f, 162.f), FString::Printf(TEXT("SPOT %d/%d"), FMath::Min(SpotsDone + 1, FMath::Max(1, SpotsTotal)), FMath::Max(1, SpotsTotal)), TDStyle::NumFont(15), ScreenText2);

	// Sensor trace: quick and green while working, slow and blue when idle.
	const FVector2D W(16.f, 206.f);
	const FVector2D WS(Size.X - 32.f, Size.Y - W.Y - 52.f);
	P.Box(L + 1, W, WS, ScreenPanel);
	TArray<FVector2D> Trace;
	const float Speed = bWorking ? 6.f : 1.5f;
	const float Amp = bWorking ? WS.Y * 0.36f : WS.Y * 0.15f;
	for (int32 I = 0; I <= 48; ++I)
	{
		const float X = I / 48.f;
		const float T = Now * Speed + X * 12.f;
		Trace.Add(FVector2D(W.X + X * WS.X, W.Y + WS.Y * 0.5f + Amp * (FMath::Sin(T) * 0.7f + FMath::Sin(T * 2.9f) * 0.3f)));
	}
	P.Lines(L + 2, Trace, bWorking ? TDStyle::Green() : ScreenGlow, 3.f);
	P.Text(L + 2, FVector2D(16.f, Size.Y - 40.f), bWorking ? TEXT("WORKING") : Progress >= 1.f ? TEXT("SPOT DONE") : TEXT("READY - HOLD TO WORK"),
		TDStyle::HeadFont(14), bWorking ? TDStyle::Green() : ScreenText2);
	return L + 4;
}
