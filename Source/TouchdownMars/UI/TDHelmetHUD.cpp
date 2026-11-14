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

void ATDHelmetHUD::DrawHUD()
{
	Super::DrawHUD();
	const APlayerController* PC = GetOwningPlayerController();
	const ATDAstronautCharacter* Astronaut = PC ? Cast<ATDAstronautCharacter>(PC->GetPawn()) : nullptr;
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	const UTDShotDirector* Director = UTDShotDirector::Get(this);
	const ATDPlayerController* DesignPC = Cast<ATDPlayerController>(PC);
	if (!Canvas || !Astronaut || !Mission || (Director && Director->IsPlayingShot()) || Astronaut->IsConsoleOpen()
		|| (DesignPC && DesignPC->IsDesignScreenOpen()) || !Mission->HasMission())
	{
		return;
	}
	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
	const float Scale = FMath::Max(0.6f, Canvas->ClipY / 1080.f) * 1.6f;
	const FVector2D Centre(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);

	// Centre dot (the view centre sits above the 2D panel).
	DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.85f), Centre.X - 3.f, Centre.Y - 3.f, 6.f, 6.f);

	// What the helmet looks at.
	const FText Focus = Astronaut->GetFocusText();
	if (!Focus.IsEmpty() && Font)
	{
		float Width = 0.f, Height = 0.f;
		GetTextSize(Focus.ToString(), Width, Height, Font, Scale);
		DrawRect(FLinearColor(0.05f, 0.08f, 0.13f, 0.75f), Centre.X - Width * 0.5f - 12.f, Centre.Y + 18.f, Width + 24.f, Height + 10.f);
		DrawText(Focus.ToString(), FLinearColor(0.96f, 0.95f, 0.92f), Centre.X - Width * 0.5f, Centre.Y + 23.f, Font, Scale);
	}

	// Jezero weather of the phase's first sol (Perseverance MEDA archive; labelled ARCHIVE, never live).
	if (Font)
	{
		const FTDPhaseRow* PhaseRow = Mission->GetPhaseRow(Mission->GetState().PhaseIndex);
		if (const FTDWeatherRow* Weather = PhaseRow ? Mission->GetWeather(PhaseRow->SolStart) : nullptr)
		{
			FNumberFormattingOptions NoGroup;
			NoGroup.UseGrouping = false;
			NoGroup.MaximumFractionalDigits = 0;
			const FString Line = FText::Format(Mission->GetText(TEXT("Weather_Line")), FText::AsNumber(Weather->MedaSol, &NoGroup),
				FText::FromString(Weather->EarthDate), FText::AsNumber(FMath::RoundToInt(Weather->AirMinC)),
				FText::AsNumber(FMath::RoundToInt(Weather->AirMaxC)), FText::AsNumber(FMath::RoundToInt(Weather->PressureMeanPa), &NoGroup)).ToString();
			const float Small = Scale * 0.62f;
			float Width = 0.f, Height = 0.f;
			GetTextSize(Line, Width, Height, Font, Small);
			const float X = 24.f; // under the phase strip and the Sun badge, clear of the CAPCOM panel
			const float Y = Canvas->ClipY * 0.19f;
			DrawRect(FLinearColor(0.05f, 0.08f, 0.13f, 0.7f), X - 10.f, Y - 5.f, Width + 20.f, Height + 10.f);
			DrawText(Line, FLinearColor(0.80f, 0.86f, 0.95f), X, Y, Font, Small);
		}
	}

	// Base operations: crew labels over their heads, your own task sites, the time-lapse banner.
	if (Font)
	{
		DrawOps(Mission, Astronaut, Font, Scale);
	}

	// Console hint; a waiting event card makes it urgent.
	if (Font)
	{
		const FTDMissionState& State = Mission->GetState();
		const bool bCardWaiting = !State.CurrentEventId.IsNone() && State.EventChoice < 0 && Mission->IsMissionRunning();
		const FString Hint = Mission->GetText(bCardWaiting ? TEXT("UI_ConsoleHintCard") : TEXT("UI_ConsoleHint")).ToString();
		float Width = 0.f, Height = 0.f;
		GetTextSize(Hint, Width, Height, Font, Scale);
		const float Y = Canvas->ClipY * 0.86f;
		DrawRect(FLinearColor(0.05f, 0.08f, 0.13f, 0.8f), Canvas->ClipX * 0.5f - Width * 0.5f - 16.f, Y - 6.f, Width + 32.f, Height + 12.f);
		DrawText(Hint, FLinearColor(0.97f, 0.82f, 0.27f), Canvas->ClipX * 0.5f - Width * 0.5f, Y, Font, Scale);
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

	if (!Mission->HasOps())
	{
		return;
	}
	// Your own planned tasks: a marker at each site, E when you are there.
	const FTDOpsState& Ops = Mission->GetOps();
	if (Ops.Crew.Num() > 0 && !Mission->IsRunningPhase())
	{
		for (const FName& Id : Ops.Crew[0].Tasks)
		{
			const FTDTaskRow* Row = Mission->GetTasksTable()->FindRow<FTDTaskRow>(Id, TEXT("TDHelmetHUD"), false);
			if (!Row)
			{
				continue;
			}
			const FVector Site = Mission->GetSitePosition(Row->Site) + FVector(0.f, 0.f, Eye.Z + 80.f);
			const FVector Screen = Project(Site);
			if (Screen.Z <= 0.f)
			{
				continue;
			}
			const FString Line = FText::Format(Mission->GetText(TEXT("Ops_Marker")), Mission->GetText(Row->NameKey),
				FText::AsNumber(FMath::RoundToInt(FVector2D::Distance(FVector2D(Eye), FVector2D(Site)) / 100.f))).ToString();
			float W = 0.f, H = 0.f;
			GetTextSize(Line, W, H, Font, Small);
			const float X = FMath::Clamp(Screen.X, 20.f, Canvas->ClipX - W - 20.f);
			const float Y = FMath::Clamp(Screen.Y, 80.f, Canvas->ClipY * 0.8f);
			DrawRect(FLinearColor(0.88f, 0.24f, 0.19f, 0.9f), X - 8.f, Y - 4.f, W + 16.f, H + 8.f);
			DrawText(Line, FLinearColor::White, X, Y, Font, Small);
		}
	}
	const FText Prompt = Astronaut->GetInteractPrompt();
	if (!Prompt.IsEmpty())
	{
		float W = 0.f, H = 0.f;
		GetTextSize(Prompt.ToString(), W, H, Font, Scale * 0.8f);
		const float Y = Canvas->ClipY * 0.62f;
		DrawRect(FLinearColor(0.88f, 0.24f, 0.19f, 0.92f), Canvas->ClipX * 0.5f - W * 0.5f - 14.f, Y - 6.f, W + 28.f, H + 12.f);
		DrawText(Prompt.ToString(), FLinearColor::White, Canvas->ClipX * 0.5f - W * 0.5f, Y, Font, Scale * 0.8f);
	}
	if (Mission->IsRunningPhase())
	{
		int32 SolStart = 0;
		int32 SolEnd = 0;
		Mission->GetPhaseSols(Mission->GetState().PhaseIndex, SolStart, SolEnd);
		const FString Banner = FText::Format(Mission->GetText(TEXT("Ops_RunBanner")), SolStart, SolEnd).ToString();
		float W = 0.f, H = 0.f;
		GetTextSize(Banner, W, H, Font, Scale * 0.8f);
		DrawRect(FLinearColor(0.88f, 0.24f, 0.19f, 0.92f), Canvas->ClipX * 0.5f - W * 0.5f - 18.f, 18.f, W + 36.f, H + 14.f);
		DrawText(Banner, FLinearColor::White, Canvas->ClipX * 0.5f - W * 0.5f, 25.f, Font, Scale * 0.8f);
	}
}
