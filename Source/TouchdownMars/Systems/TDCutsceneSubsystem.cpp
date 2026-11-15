// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Systems/TDCutsceneSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "FileMediaSource.h"
#include "Framework/Application/SlateApplication.h"
#include "MediaPlayer.h"
#include "MediaSoundComponent.h"
#include "MediaTexture.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Systems/TDMissionSubsystem.h"

namespace TDCutscene
{
	constexpr double SkipGuardSeconds = 0.6;   // the click that started the cutscene must not skip it
	constexpr double OpenTimeoutSeconds = 5.0;  // a clip that never starts is skipped
	constexpr double MaxClipSeconds = 25.0;
}

/** Full-screen catcher: any key or click skips the current clip. */
class STDCutsceneInput : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STDCutsceneInput) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UTDCutsceneSubsystem>, Owner)
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Owner = InArgs._Owner;
		ChildSlot[InArgs._Content.Widget];
	}
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent&) override { if (Owner.IsValid()) { Owner->Skip(); } return FReply::Handled(); }
	virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent&) override { if (Owner.IsValid()) { Owner->Skip(); } return FReply::Handled(); }

private:
	TWeakObjectPtr<UTDCutsceneSubsystem> Owner;
};

bool UTDCutsceneSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

TStatId UTDCutsceneSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDCutsceneSubsystem, STATGROUP_Tickables);
}

bool UTDCutsceneSubsystem::Enabled()
{
	const TCHAR* Cmd = FCommandLine::Get();
	if (FParse::Param(Cmd, TEXT("TDCutscenesOn")))
	{
		return true;   // dev: keep the cutscenes in a shortcut run (e.g. -TDSurface -TDOpsDemo -TDCutscenesOn)
	}
	return !(FParse::Param(Cmd, TEXT("TDSurface")) || FParse::Param(Cmd, TEXT("TDShotTour")) || FParse::Param(Cmd, TEXT("TDWorkTest"))
		|| FParse::Param(Cmd, TEXT("TDNoCutscenes")) || FString(Cmd).Contains(TEXT("-TDCam=")) || GIsAutomationTesting);
}

void UTDCutsceneSubsystem::Play(const UObject* WorldContext, const TArray<FName>& Clips, TFunction<void()> OnDone)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	UTDCutsceneSubsystem* Self = World ? World->GetSubsystem<UTDCutsceneSubsystem>() : nullptr;
	if (!Self || !Enabled() || Clips.Num() == 0 || !GEngine || !GEngine->GameViewport)
	{
		if (OnDone) { OnDone(); }
		return;
	}
	if (Self->bActive)
	{
		Self->Queue.Append(Clips);    // chained request: play after the current ones, keep the first callback
		return;
	}
	Self->Queue = Clips;
	Self->Done = MoveTemp(OnDone);
	Self->bActive = true;
	Self->BuildOverlay();
	Self->StartNext();
}

bool UTDCutsceneSubsystem::IsPlaying(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UTDCutsceneSubsystem* Self = World ? World->GetSubsystem<UTDCutsceneSubsystem>() : nullptr;
	return Self && Self->bActive;
}

void UTDCutsceneSubsystem::BuildOverlay()
{
	if (!Player)
	{
		Player = NewObject<UMediaPlayer>(this);
		Player->PlayOnOpen = true;
		Player->SetLooping(false);
		Texture = NewObject<UMediaTexture>(this);
		Texture->AutoClear = true;
		Texture->ClearColor = FLinearColor::Black;
		Texture->SetMediaPlayer(Player);
		Texture->UpdateResource();
		Source = NewObject<UFileMediaSource>(this);
	}
	if (!SoundOwner && GetWorld())
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		SoundOwner = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
		if (SoundOwner)
		{
			Sound = NewObject<UMediaSoundComponent>(SoundOwner);
			Sound->SetMediaPlayer(Player);
			Sound->bIsUISound = true;
			SoundOwner->SetRootComponent(Sound);
			Sound->RegisterComponent();
		}
	}
	Brush = MakeShared<FSlateBrush>();
	Brush->SetResourceObject(Texture);
	Brush->ImageSize = FVector2D(1920.f, 1080.f);
	Brush->DrawAs = ESlateBrushDrawType::Image;

	Overlay = SNew(STDCutsceneInput).Owner(this)
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor::Black)
		]
		+ SOverlay::Slot()
		[
			SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
			[
				SNew(SImage).Image(Brush.Get())
			]
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom).HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 56.f)
		[
			SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.55f)).Padding(FMargin(18.f, 8.f))
			[
				SAssignNew(Caption, STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 22)).ColorAndOpacity(FLinearColor(0.96f, 0.93f, 0.88f))
			]
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom).HAlign(HAlign_Right).Padding(0.f, 0.f, 28.f, 20.f)
		[
			SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 13)).ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.6f))
			.Text_Lambda([this]()
			{
				const UTDMissionSubsystem* Mission = GetWorld() ? GetWorld()->GetSubsystem<UTDMissionSubsystem>() : nullptr;
				return Mission ? Mission->GetText(TEXT("Cutscene_Skip")) : FText::GetEmpty();
			})
		]
		// AI credit tag, shown for the whole clip (sources.csv: GeminiVeoCutscenes), like the Gemini tag on the design cards.
		+ SOverlay::Slot().VAlign(VAlign_Top).HAlign(HAlign_Left).Padding(28.f, 20.f, 0.f, 0.f)
		[
			SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.55f)).Padding(FMargin(10.f, 5.f))
			[
				SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 14)).ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.9f))
				.Text(FText::FromString(TEXT("AI video (Google Veo)")))
			]
		]
	];
	GEngine->GameViewport->AddViewportWidgetContent(Overlay.ToSharedRef(), 1000);
	FSlateApplication::Get().SetAllUserFocus(Overlay, EFocusCause::SetDirectly);
}

void UTDCutsceneSubsystem::StartNext()
{
	if (Queue.Num() == 0)
	{
		Finish();
		return;
	}
	const FName Clip = Queue[0];
	Queue.RemoveAt(0);
	const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Movies") / (Clip.ToString() + TEXT(".mp4")));
	if (!FPaths::FileExists(Path))
	{
		UE_LOG(LogTemp, Warning, TEXT("Cutscene: missing %s"), *Path);
		StartNext();
		return;
	}
	if (Caption.IsValid())
	{
		const UTDMissionSubsystem* Mission = GetWorld() ? GetWorld()->GetSubsystem<UTDMissionSubsystem>() : nullptr;
		Caption->SetText(Mission ? Mission->GetText(FName(*(TEXT("Cutscene_") + Clip.ToString()))) : FText::GetEmpty());
	}
	Source->SetFilePath(Path);
	Player->OpenSource(Source);
	if (Sound) { Sound->Start(); }
	ClipStart = FPlatformTime::Seconds();
	bStarted = false;
	UE_LOG(LogTemp, Display, TEXT("Cutscene: %s"), *Clip.ToString());
}

void UTDCutsceneSubsystem::Skip()
{
	if (bActive && FPlatformTime::Seconds() - ClipStart > TDCutscene::SkipGuardSeconds)
	{
		if (Player) { Player->Close(); }
		StartNext();
	}
}

void UTDCutsceneSubsystem::Tick(float DeltaTime)
{
#if !UE_BUILD_SHIPPING
	// Dev: -TDCutsceneTest plays every cutscene once, 3 s after start.
	static bool bTestDone = false;
	if (!bTestDone && FParse::Param(FCommandLine::Get(), TEXT("TDCutsceneTest")) && GetWorld() && GetWorld()->GetTimeSeconds() > 3.f)
	{
		bTestDone = true;
		Play(this, {TEXT("CS_Launch"), TEXT("CS_Landing"), TEXT("CS_LiftOff"), TEXT("CS_Return")}, []() { UE_LOG(LogTemp, Display, TEXT("Cutscene: test done")); });
	}
#endif
	if (!bActive || !Player)
	{
		return;
	}
	const double Elapsed = FPlatformTime::Seconds() - ClipStart;
	if (!bStarted && Player->IsPlaying())
	{
		bStarted = true;
	}
	const bool bEnded = bStarted && !Player->IsPlaying() && !Player->IsPreparing() && !Player->IsBuffering();
	if (bEnded || (!bStarted && Elapsed > TDCutscene::OpenTimeoutSeconds) || Elapsed > TDCutscene::MaxClipSeconds)
	{
		Player->Close();
		StartNext();
	}
}

void UTDCutsceneSubsystem::RemoveOverlay()
{
	if (Overlay.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
	}
	Overlay.Reset();
	Caption.Reset();
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

void UTDCutsceneSubsystem::Finish()
{
	bActive = false;
	if (Sound) { Sound->Stop(); }
	if (Player) { Player->Close(); }
	RemoveOverlay();
	TFunction<void()> Callback = MoveTemp(Done);
	Done = nullptr;
	if (Callback) { Callback(); }
}

void UTDCutsceneSubsystem::Deinitialize()
{
	bActive = false;
	Queue.Reset();
	Done = nullptr;
	RemoveOverlay();
	Super::Deinitialize();
}
