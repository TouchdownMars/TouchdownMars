// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "UI/TDHelmetHUD.h"

#include "Actors/TDAstronautCharacter.h"
#include "Actors/TDPlayerController.h"
#include "Actors/TDSceneActors.h"
#include "EngineUtils.h"
#include "Model/TDOperations.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"
#include "Systems/TDMissionSubsystem.h"
#include "Systems/TDShotDirector.h"
#include "UI/TDObjective.h"

void ATDHelmetHUD::DrawHUD()
{
	Super::DrawHUD();
	const APlayerController* PC = GetOwningPlayerController();
	const ATDAstronautCharacter* Astronaut = PC ? Cast<ATDAstronautCharacter>(PC->GetPawn()) : nullptr;
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	const UTDShotDirector* Director = UTDShotDirector::Get(this);
	const ATDPlayerController* DesignPC = Cast<ATDPlayerController>(PC);
	if (!Canvas || !Astronaut || !Mission || (Director && Director->IsPlayingShot()) || Astronaut->IsConsoleOpen()
		|| (DesignPC && (DesignPC->IsDesignScreenOpen() || DesignPC->IsBaseComputerOpen())) || !Mission->HasMission())
	{
		return;
	}
	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
	const float Scale = FMath::Max(0.6f, Canvas->ClipY / 1080.f) * 1.25f;
	const FVector2D Centre(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);

	// Centre dot (the view centre sits above the 2D panel).
	DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.85f), Centre.X - 3.f, Centre.Y - 3.f, 6.f, 6.f);

	// What the helmet looks at.
	const FText Focus = Astronaut->GetFocusText();
	if (!Focus.IsEmpty() && Font)
	{
		float Width = 0.f, Height = 0.f;
		GetTextSize(Focus.ToString(), Width, Height, Font, Scale);
		// Above the centre: the hold ring and prompt sit below it.
		const float Top = Centre.Y - 74.f - Height;
		DrawRect(FLinearColor(0.08f, 0.09f, 0.11f, 0.72f), Centre.X - Width * 0.5f - 12.f, Top - 5.f, Width + 24.f, Height + 10.f);
		DrawText(Focus.ToString(), FLinearColor(0.96f, 0.95f, 0.92f), Centre.X - Width * 0.5f, Top, Font, Scale);
	}

	// Base operations: crew labels over their heads, your own task sites, the time-lapse banner.
	if (Font)
	{
		DrawOps(Mission, Astronaut, Font, Scale);
	}

	// Tracked objective: one world marker with the distance, held on the screen edge when off view.
	if (Font)
	{
		DrawObjectiveMarker(Mission, Astronaut, Font, Scale);
	}
}

void ATDHelmetHUD::DrawOps(const UTDMissionSubsystem* Mission, const ATDAstronautCharacter* Astronaut, UFont* Font, float Scale)
{
	const float Small = Scale * 0.55f;
	const FVector Eye = Astronaut->GetActorLocation();
	// Crew: name, what they are doing, progress, and a short spoken line (F.E.A.R.: if the AI did not say it, it did not happen).
	for (TActorIterator<ATDCrewMember> It(GetWorld()); It; ++It)
	{
		const ATDCrewMember* Crew = *It;
		if (Crew->IsHidden())
		{
			continue;
		}
		const FVector Head = Crew->GetActorLocation() + FVector(0.f, 0.f, 230.f);
		if (FVector::Dist(Head, Eye) > 9000.f)
		{
			continue;
		}
		const FVector Screen = Project(Head);
		if (Screen.Z <= 0.f || Screen.X < 0.f || Screen.Y < 0.f || Screen.X > Canvas->ClipX || Screen.Y > Canvas->ClipY)
		{
			continue;
		}
		const int32 CrewIndex = Crew->System == ETDSystem::Air ? 1 : (Crew->System == ETDSystem::Greenhouse ? 2 : 3);
		FString Line = Mission->GetCrewName(CrewIndex).ToString();
		if (!Crew->StatusText.IsEmpty())
		{
			Line += TEXT(" · ") + Crew->StatusText.ToString();
		}
		float W = 0.f, H = 0.f;
		GetTextSize(Line, W, H, Font, Small);
		const float X = Screen.X - W * 0.5f;
		float Y = Screen.Y - H;
		DrawRect(FLinearColor(0.08f, 0.09f, 0.11f, 0.72f), X - 8.f, Y - 4.f, W + 16.f, H + 8.f);
		DrawText(Line, FLinearColor(0.98f, 0.97f, 0.95f), X, Y, Font, Small);
		if (Crew->StatusProgress >= 0.f)
		{
			DrawRect(FLinearColor(0.85f, 0.83f, 0.80f, 0.9f), X - 8.f, Y + H + 4.f, W + 16.f, 5.f);
			DrawRect(FLinearColor(0.12f, 0.56f, 0.31f, 1.f), X - 8.f, Y + H + 4.f, (W + 16.f) * Crew->StatusProgress, 5.f);
		}
		const FText Bark = Crew->GetBark();
		if (!Bark.IsEmpty())
		{
			const FString B = FString::Printf(TEXT("\"%s\""), *Bark.ToString());
			float BW = 0.f, BH = 0.f;
			GetTextSize(B, BW, BH, Font, Small);
			Y -= BH + 12.f;
			DrawRect(FLinearColor(0.98f, 0.97f, 0.95f, 0.9f), Screen.X - BW * 0.5f - 8.f, Y - 4.f, BW + 16.f, BH + 8.f);
			DrawText(B, FLinearColor(0.08f, 0.09f, 0.11f), Screen.X - BW * 0.5f, Y, Font, Small);
		}
	}

}

void ATDHelmetHUD::DrawObjectiveMarker(const UTDMissionSubsystem* Mission, const ATDAstronautCharacter* Astronaut, UFont* Font, float Scale)
{
	const FTDObjective O = TDObjective::Compute(Mission, Astronaut);
	if (!O.bHasTarget || O.Distance < 250.f)
	{
		return;
	}
	FVector Screen = Project(O.Target);
	const FVector2D Centre(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	const float Margin = 70.f;
	bool bOff = Screen.Z <= 0.f || Screen.X < Margin || Screen.Y < Margin || Screen.X > Canvas->ClipX - Margin || Screen.Y > Canvas->ClipY - Margin;
	FVector2D P(Screen.X, Screen.Y);
	if (bOff)
	{
		// Behind or outside the view: put the marker on the screen edge in the target's direction.
		FVector2D Dir = P - Centre;
		if (Screen.Z <= 0.f)
		{
			Dir = -Dir;
		}
		if (Dir.IsNearlyZero())
		{
			Dir = FVector2D(0.f, 1.f);
		}
		Dir.Normalize();
		const float Sx = (Canvas->ClipX * 0.5f - Margin) / FMath::Max(0.001f, FMath::Abs(Dir.X));
		const float Sy = (Canvas->ClipY * 0.5f - Margin) / FMath::Max(0.001f, FMath::Abs(Dir.Y));
		P = Centre + Dir * FMath::Min(Sx, Sy);
	}
	const float Pulse = 0.5f + 0.5f * FMath::Sin(GetWorld()->GetRealTimeSeconds() * 4.f);
	const FLinearColor Red(0.88f, 0.24f, 0.19f, 1.f);
	const FLinearColor Ink(0.98f, 0.97f, 0.95f, 1.f);
	const float R = 13.f * FMath::Max(0.8f, Canvas->ClipY / 1080.f) * (1.f + 0.15f * Pulse);
	// Diamond.
	const FVector2D Pts[4] = { P + FVector2D(0.f, -R), P + FVector2D(R, 0.f), P + FVector2D(0.f, R), P + FVector2D(-R, 0.f) };
	for (int32 I = 0; I < 4; ++I)
	{
		DrawLine(Pts[I].X, Pts[I].Y, Pts[(I + 1) % 4].X, Pts[(I + 1) % 4].Y, Red, 4.f);
	}
	DrawRect(Red, P.X - R * 0.35f, P.Y - R * 0.35f, R * 0.7f, R * 0.7f);
	const FString Line = FString::Printf(TEXT("%s  %d m"), *O.Title.ToString(), FMath::RoundToInt(O.Distance / 100.f));
	const float Small = Scale * 0.5f;
	float W = 0.f, H = 0.f;
	GetTextSize(Line, W, H, Font, Small);
	const float X = FMath::Clamp(P.X - W * 0.5f, 12.f, Canvas->ClipX - W - 12.f);
	const float Y = FMath::Clamp(P.Y + R + 8.f, 12.f, Canvas->ClipY - H - 12.f);
	DrawRect(FLinearColor(0.08f, 0.09f, 0.11f, 0.78f), X - 8.f, Y - 3.f, W + 16.f, H + 6.f);
	DrawText(Line, Ink, X, Y, Font, Small);
}
