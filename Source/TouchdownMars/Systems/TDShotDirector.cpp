// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Systems/TDShotDirector.h"

#include "Actors/TDSceneActors.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Systems/TDMissionSubsystem.h"
#include "TimerManager.h"
#include "TouchdownMars.h"

const FName UTDShotDirector::MainCameraTag(TEXT("CAM_Main"));

namespace
{
	const TCHAR* ShotTablePath = TEXT("/Game/TouchdownMars/Data/DT_OutcomeShots.DT_OutcomeShots");
	constexpr float SkipGuardSeconds = 0.1f; // the click that chose the card option never skips its own shot
}

UTDShotDirector::UTDShotDirector()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

UTDShotDirector* UTDShotDirector::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
	return GameMode ? GameMode->FindComponentByClass<UTDShotDirector>() : nullptr;
}

void UTDShotDirector::BeginPlay()
{
	Super::BeginPlay();
	if (UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>())
	{
		Mission->OnPhaseStarted.AddDynamic(this, &UTDShotDirector::HandlePhaseStarted);
	}
}

bool UTDShotDirector::LoadTable() const
{
	if (!ShotTable)
	{
		ShotTable = LoadObject<UDataTable>(nullptr, ShotTablePath);
	}
	return ShotTable != nullptr;
}

APlayerController* UTDShotDirector::GetPC() const
{
	return UGameplayStatics::GetPlayerController(this, 0);
}

bool UTDShotDirector::IsReduceMotion() const
{
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	return Mission && Mission->IsReduceMotion();
}

AActor* UTDShotDirector::FindTaggedActor(FName Tag) const
{
	if (Tag.IsNone())
	{
		return nullptr;
	}
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(Tag))
		{
			return *It;
		}
	}
	return nullptr;
}

const FOutcomeShot* UTDShotDirector::FindRowForEvent(FName EventId) const
{
	if (!LoadTable() || EventId.IsNone())
	{
		return nullptr;
	}
	const FOutcomeShot* Found = nullptr;
	ShotTable->ForeachRow<FOutcomeShot>(TEXT("FindRowForEvent"), [&](const FName&, const FOutcomeShot& Row)
	{
		if (!Found && Row.EventId == EventId)
		{
			Found = &Row;
		}
	});
	return Found;
}

const FOutcomeShot* UTDShotDirector::FindRowForShot(FName ShotId) const
{
	if (!LoadTable() || ShotId.IsNone())
	{
		return nullptr;
	}
	const FOutcomeShot* Found = nullptr;
	ShotTable->ForeachRow<FOutcomeShot>(TEXT("FindRowForShot"), [&](const FName&, const FOutcomeShot& Row)
	{
		if (!Found && Row.ShotId == ShotId)
		{
			Found = &Row;
		}
	});
	return Found;
}

void UTDShotDirector::ActivateMainCamera()
{
	MainCamera = FindTaggedActor(MainCameraTag);
	if (!MainCamera)
	{
		// Test maps keep the greybox rig.
		TActorIterator<ATDCameraRig> It(GetWorld());
		if (It)
		{
			MainCamera = *It;
		}
	}
	if (!MainCamera)
	{
		UE_LOG(LogTouchdownMars, Warning, TEXT("ShotDirector: no actor tagged %s and no camera rig in %s"), *MainCameraTag.ToString(), *GetWorld()->GetMapName());
		return;
	}
	MainCameraBase = MainCamera->GetActorLocation();
	DriftTime = 0.f;
	if (APlayerController* PC = GetPC())
	{
		PC->SetViewTarget(MainCamera);
	}
}

void UTDShotDirector::HandlePhaseStarted(ETDPhase Phase)
{
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	if (!Mission)
	{
		return;
	}
	const FTDMissionState& State = Mission->GetState();
	const TArray<int32> Playable = Mission->GetPlayablePhases();
	if (State.SolsElapsed == 0 && Playable.Num() > 0 && State.PhaseIndex == Playable[0])
	{
		PlayedShots.Reset(); // a new playthrough: first occurrences get the full hold again
	}
}

FName UTDShotDirector::GetResultTextKey(FName EventId, int32 OutcomeState) const
{
	const FOutcomeShot* Row = FindRowForEvent(EventId);
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	if (!Row || !Mission || Row->ResultTextKey.IsNone())
	{
		return NAME_None;
	}
	if (OutcomeState == 0 || OutcomeState == 1)
	{
		const FName ChoiceKey(*FString::Printf(TEXT("%s_%s"), *Row->ResultTextKey.ToString(), OutcomeState == 0 ? TEXT("A") : TEXT("B")));
		if (Mission->HasText(ChoiceKey))
		{
			return ChoiceKey;
		}
	}
	return Mission->HasText(Row->ResultTextKey) ? Row->ResultTextKey : NAME_None;
}

FText UTDShotDirector::GetResultText(FName EventId, int32 OutcomeState) const
{
	const FName Key = GetResultTextKey(EventId, OutcomeState);
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	return Key.IsNone() || !Mission ? FText::GetEmpty() : Mission->GetBandText(Key);
}

void UTDShotDirector::PlayShot(FName EventId, int32 OutcomeState)
{
	if (bPlaying)
	{
		SkipShot(); // at most one shot at a time; the old one still reports its finish
	}
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	if (Mission && !Mission->IsOutcomeShotsEnabled())
	{
		FinishNextTick(EventId);
		return;
	}
	if (GetWorld()->GetTimeSeconds() < NoShotSeconds)
	{
		FinishNextTick(EventId);
		return;
	}
	const FOutcomeShot* Row = FindRowForEvent(EventId);
	if (!Row)
	{
		UE_LOG(LogTouchdownMars, Warning, TEXT("ShotDirector: no DT_OutcomeShots row for event %s; turn continues"), *EventId.ToString());
		FinishNextTick(EventId);
		return;
	}
	const bool bCadet = Mission && Mission->GetState().Band == ETDAgeBand::Cadet;
	if (bCadet && !Row->bAllowedInCadet)
	{
		FinishNextTick(EventId);
		return;
	}
	AActor* Camera = FindTaggedActor(Row->CameraTag);
	if (!Camera || !MainCamera || !GetPC())
	{
		UE_LOG(LogTouchdownMars, Warning, TEXT("ShotDirector: event %s shot %s has no camera tagged %s (or no main camera); turn continues"),
			*EventId.ToString(), *Row->ShotId.ToString(), *Row->CameraTag.ToString());
		FinishNextTick(EventId);
		return;
	}
	float HoldSeconds = Row->Hold;
	if (PlayedShots.Contains(Row->ShotId))
	{
		HoldSeconds = FMath::Min(HoldSeconds, ReplayHold);
	}
	if (bCadet)
	{
		HoldSeconds = FMath::Min(HoldSeconds, CadetMaxHold);
	}
	PlayedShots.Add(Row->ShotId);
	StartShot(*Row, Camera, EventId, OutcomeState, HoldSeconds);
}

void UTDShotDirector::StartShot(const FOutcomeShot& Row, AActor* Camera, FName EventId, int32 OutcomeState, float HoldSeconds)
{
	const bool bCut = IsReduceMotion();
	const float BlendIn = bCut ? 0.f : FMath::Max(0.f, Row.BlendIn);
	ShotBlendOut = bCut ? 0.f : FMath::Max(0.f, Row.BlendOut);
	bPlaying = true;
	bFinishFired = false;
	ShotElapsed = 0.f;
	ActiveEventId = EventId;
	ActiveShotId = Row.ShotId;
	if (MainCamera)
	{
		MainCamera->SetActorLocation(MainCameraBase); // the drift never shows inside a shot
	}

	UE_LOG(LogTouchdownMars, Display, TEXT("ShotDirector: play %s (event %s) in %.2f + hold %.2f + out %.2f s"),
		*Row.ShotId.ToString(), *EventId.ToString(), BlendIn, HoldSeconds, ShotBlendOut);
	OnShotStarted.Broadcast(Row.ShotId, Row.CameraTag, OutcomeState);
	GetPC()->SetViewTargetWithBlend(Camera, BlendIn, VTBlend_EaseInOut, 3.f);

	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.SetTimer(BlendOutTimer, this, &UTDShotDirector::BeginBlendOut, FMath::Max(BlendIn + HoldSeconds, 0.01f), false);
	// Safety: a shot can never hold the turn longer than its own length plus one second.
	TWeakObjectPtr<UTDShotDirector> WeakThis(this);
	Timers.SetTimer(SafetyTimer, FTimerDelegate::CreateLambda([WeakThis]()
	{
		if (WeakThis.IsValid() && WeakThis->bPlaying)
		{
			UE_LOG(LogTouchdownMars, Warning, TEXT("ShotDirector: safety timeout for %s"), *WeakThis->ActiveShotId.ToString());
			WeakThis->SkipShot();
		}
	}), BlendIn + HoldSeconds + ShotBlendOut + 1.f, false);
}

void UTDShotDirector::BeginBlendOut()
{
	if (!bPlaying)
	{
		return;
	}
	if (APlayerController* PC = GetPC())
	{
		PC->SetViewTargetWithBlend(MainCamera, ShotBlendOut, VTBlend_EaseInOut, 3.f);
	}
	GetWorld()->GetTimerManager().SetTimer(FinishTimer, this, &UTDShotDirector::Finish, FMath::Max(ShotBlendOut, 0.01f), false);
}

void UTDShotDirector::SkipShot()
{
	if (!bPlaying)
	{
		return;
	}
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(BlendOutTimer);
	Timers.ClearTimer(FinishTimer);
	if (APlayerController* PC = GetPC())
	{
		PC->SetViewTargetWithBlend(MainCamera, IsReduceMotion() ? 0.f : SkipBlend, VTBlend_EaseInOut, 3.f);
	}
	Finish();
}

void UTDShotDirector::Finish()
{
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(BlendOutTimer);
	Timers.ClearTimer(FinishTimer);
	Timers.ClearTimer(SafetyTimer);
	bPlaying = false;
	if (bFinishFired)
	{
		return;
	}
	bFinishFired = true;
	const FName EventId = ActiveEventId;
	const FName ShotId = ActiveShotId;
	ActiveEventId = NAME_None;
	ActiveShotId = NAME_None;
	UE_LOG(LogTouchdownMars, Display, TEXT("ShotDirector: finished %s (event %s)"), *ShotId.ToString(), *EventId.ToString());
	OnShotEnded.Broadcast(ShotId);
	OnShotFinished.Broadcast(EventId);
}

void UTDShotDirector::FinishNextTick(FName EventId)
{
	TWeakObjectPtr<UTDShotDirector> WeakThis(this);
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakThis, EventId]()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->OnShotFinished.Broadcast(EventId);
		}
	}));
}

void UTDShotDirector::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bPlaying)
	{
		ShotElapsed += DeltaTime;
		APlayerController* PC = GetPC();
		if (ShotElapsed > SkipGuardSeconds && PC && PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
		{
			UE_LOG(LogTouchdownMars, Display, TEXT("ShotDirector: left click skips %s at %.2f s; control back now, camera blend %.2f s"),
				*ActiveShotId.ToString(), ShotElapsed, IsReduceMotion() ? 0.f : SkipBlend);
			SkipShot();
		}
		return;
	}
	ApplyDrift();
	DriftTime += DeltaTime;
}

void UTDShotDirector::ApplyDrift()
{
	if (!MainCamera || DriftDistance <= 0.f || DriftSeconds <= 0.f)
	{
		return;
	}
	if (IsReduceMotion())
	{
		MainCamera->SetActorLocation(MainCameraBase);
		return;
	}
	// Ping-pong along the camera's right axis: 0 -> DriftDistance in DriftSeconds and back, eased, rotation untouched.
	const float Alpha = 0.5f - 0.5f * FMath::Cos(PI * DriftTime / DriftSeconds);
	MainCamera->SetActorLocation(MainCameraBase + MainCamera->GetActorRightVector() * (DriftDistance * Alpha));
}

// Debug / demo recording. Console: td.Shot <ShotId>, td.ShotAll, td.OutcomeShots 0|1. Not compiled in Shipping.

void UTDShotDirector::DebugPlayShot(FName ShotId)
{
	const FOutcomeShot* Row = FindRowForShot(ShotId);
	AActor* Camera = Row ? FindTaggedActor(Row->CameraTag) : nullptr;
	if (!Row || !Camera || !MainCamera || !GetPC())
	{
		UE_LOG(LogTouchdownMars, Warning, TEXT("ShotDirector debug: cannot play %s (row, camera or main camera missing)"), *ShotId.ToString());
		FinishNextTick(NAME_None);
		return;
	}
	if (bPlaying)
	{
		SkipShot();
	}
	StartShot(*Row, Camera, Row->EventId, 0, Row->Hold);
}

void UTDShotDirector::DebugPlayAllShots()
{
	DebugQueue.Reset();
	if (!LoadTable())
	{
		return;
	}
	ShotTable->ForeachRow<FOutcomeShot>(TEXT("DebugPlayAllShots"), [&](const FName&, const FOutcomeShot& Row)
	{
		DebugQueue.AddUnique(Row.ShotId);
	});
	if (!bDebugChain)
	{
		bDebugChain = true;
		OnShotFinished.AddDynamic(this, &UTDShotDirector::HandleDebugChainStep);
	}
	HandleDebugChainStep(NAME_None);
}

void UTDShotDirector::HandleDebugChainStep(FName EventId)
{
	if (DebugQueue.Num() == 0)
	{
		if (bDebugChain)
		{
			bDebugChain = false;
			OnShotFinished.RemoveDynamic(this, &UTDShotDirector::HandleDebugChainStep);
		}
		return;
	}
	const FName Next = DebugQueue[0];
	DebugQueue.RemoveAt(0);
	// Start on the next tick so the finished shot's listeners run first.
	TWeakObjectPtr<UTDShotDirector> WeakThis(this);
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakThis, Next]()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->DebugPlayShot(Next);
		}
	}));
}

#if !UE_BUILD_SHIPPING
namespace
{
	FAutoConsoleCommandWithWorldAndArgs GTDShotCommand(TEXT("td.Shot"), TEXT("Play one Outcome Shot by ShotId (demo recording), e.g. td.Shot S03_STORM"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UTDShotDirector* Director = UTDShotDirector::Get(World); Director && Args.Num() > 0)
			{
				Director->DebugPlayShot(FName(*Args[0]));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GTDShotAllCommand(TEXT("td.ShotAll"), TEXT("Play every Outcome Shot in table order (demo recording)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (UTDShotDirector* Director = UTDShotDirector::Get(World))
			{
				Director->DebugPlayAllShots();
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GTDOutcomeShotsCommand(TEXT("td.OutcomeShots"), TEXT("td.OutcomeShots 0|1: switch Outcome Shots off/on (saved setting)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UTDMissionSubsystem* Mission = World ? World->GetSubsystem<UTDMissionSubsystem>() : nullptr; Mission && Args.Num() > 0)
			{
				Mission->SetOutcomeShotsEnabled(Args[0] != TEXT("0"));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GTDReduceMotionCommand(TEXT("td.ReduceMotion"), TEXT("td.ReduceMotion 0|1 (saved setting)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UTDMissionSubsystem* Mission = World ? World->GetSubsystem<UTDMissionSubsystem>() : nullptr; Mission && Args.Num() > 0)
			{
				Mission->SetReduceMotion(Args[0] != TEXT("0"));
			}
		}));
}
#endif
