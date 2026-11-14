// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Actors/TDPlayerController.h"

#include "UI/TDDesignRoom.h"
#include "UI/TDBaseComputer.h"
#include "UI/TDGamepadCursor.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"
#include "GenericPlatform/IInputInterface.h"
#include "Framework/Application/SlateUser.h"
#include "GameFramework/PlayerInput.h"
#include "Misc/CoreDelegates.h"
#include "InputMappingContext.h"
#include "Misc/CommandLine.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Engine/GameViewportClient.h"

#include "Actors/TDAstronautCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/NavigationConfig.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/World.h"
#include "Systems/TDMissionSubsystem.h"
#include "TouchdownMars.h"
#include "UI/TDWidgets.h"

ATDPlayerController::ATDPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
}

void ATDPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Tab toggles the mission console in first person; keep Slate from using it for focus navigation.
	if (FSlateApplication::IsInitialized())
	{
		TSharedRef<FNavigationConfig> Navigation = MakeShared<FNavigationConfig>();
		Navigation->bTabNavigation = false;
		FSlateApplication::Get().SetNavigationConfig(Navigation);
	}

	// Gamepad: the Menu button runs the phase (same as Space/Enter); screens get a stick-driven cursor.
	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (NextPhaseAction)
		{
			PadContext = NewObject<UInputMappingContext>(this, TEXT("IMC_TD_Pad"));
			PadContext->MapKey(NextPhaseAction, EKeys::Gamepad_Special_Right);
			InputSubsystem->AddMappingContext(PadContext, 1);
		}
	}
	if (FSlateApplication::IsInitialized())
	{
		TWeakObjectPtr<ATDPlayerController> WeakThis(this);
		PadCursor = MakeShared<FTDGamepadCursor>(
			[WeakThis]() { return WeakThis.IsValid() && WeakThis->WantsPadCursor(); },
			[WeakThis]() { if (WeakThis.IsValid()) { WeakThis->HandlePadBack(); } });
		FSlateApplication::Get().RegisterInputPreProcessor(PadCursor);
	}

#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("TDPadTest")))
	{
		GetWorldTimerManager().SetTimer(PadTestTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { RunPadTest(0); }), 8.f, false);
	}
#endif

	if (HudWidgetClass)
	{
		Hud = CreateWidget<UTDHudWidget>(this, HudWidgetClass);
		Hud->AddToViewport();
	}
	else
	{
		UE_LOG(LogTouchdownMars, Warning, TEXT("PC_TD has no HudWidgetClass"));
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	// macOS can start the game behind the launching app; then the first click only activates the window.
	// Bring the game window to the front and give the viewport keyboard and mouse focus.
	if (FSlateApplication::IsInitialized() && GEngine && GEngine->GameViewport)
	{
		if (TSharedPtr<SWindow> Window = GEngine->GameViewport->GetWindow())
		{
			Window->BringToFront(true);
			Window->HACK_ForceToFront();
		}
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

void ATDPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (NextPhaseAction)
		{
			Input->BindAction(NextPhaseAction, ETriggerEvent::Started, this, &ATDPlayerController::HandleNextPhase);
		}
		if (BackAction)
		{
			Input->BindAction(BackAction, ETriggerEvent::Started, this, &ATDPlayerController::HandleBack);
		}
	}
}

void ATDPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (DesignRoom.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(DesignRoom.ToSharedRef());
	}
	DesignRoom.Reset();
	if (BaseComputer.IsValid() && bBaseOpen && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(BaseComputer.ToSharedRef());
	}
	BaseComputer.Reset();
	if (PadCursor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(PadCursor);
	}
	PadCursor.Reset();
	Super::EndPlay(EndPlayReason);
}

void ATDPlayerController::ShowDesignScreen()
{
	UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	if (!Mission || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	if (!DesignRoom.IsValid())
	{
		SAssignNew(DesignRoom, STDDesignRoom)
			.Mission(Mission)
			.OnTransitDone(FTDDesignScreenAction::CreateUObject(this, &ATDPlayerController::HandleTransitDone));
	}
	if (!bDesignScreenOpen)
	{
		GEngine->GameViewport->AddViewportWidgetContent(DesignRoom.ToSharedRef(), 50);
		bDesignScreenOpen = true;
	}
	// The design room takes the mouse; the astronaut stands still and the mission HUD hides.
	if (Hud)
	{
		Hud->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(DesignRoom);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	SetShowMouseCursor(true);
}

void ATDPlayerController::HideDesignScreen()
{
	if (DesignRoom.IsValid() && bDesignScreenOpen && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(DesignRoom.ToSharedRef());
	}
	bDesignScreenOpen = false;
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
	if (Hud)
	{
		Hud->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (ATDAstronautCharacter* Astronaut = Cast<ATDAstronautCharacter>(GetPawn()))
	{
		Astronaut->SetConsoleOpen(false); // back to walking: game input, cursor hidden
	}
	else
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);
	}
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

void ATDPlayerController::OpenDesignRoom()
{
#if !UE_BUILD_SHIPPING
	// Screenshot runs (-TDCam=x,y,z,pitch,yaw): no design room, a fixed camera over the base.
	FString CamSpec;
	if (FParse::Value(FCommandLine::Get(), TEXT("TDCam="), CamSpec, false))
	{
		TArray<FString> Parts;
		CamSpec.ParseIntoArray(Parts, TEXT(","));
		if (Parts.Num() == 5)
		{
			const FVector Location(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2]));
			const FRotator Rotation(FCString::Atof(*Parts[3]), FCString::Atof(*Parts[4]), 0.f);
			if (ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(Location, Rotation))
			{
				Cam->GetCameraComponent()->SetFieldOfView(70.f);
				SetIgnoreMoveInput(true);
				SetIgnoreLookInput(true);
				SetViewTarget(Cam);
				if (Hud)
				{
					Hud->SetVisibility(ESlateVisibility::Collapsed);
				}
				UE_LOG(LogTouchdownMars, Display, TEXT("Design: screenshot camera %s"), *CamSpec);
				// -TDOpsDemo with a fixed camera: fly the reference design and watch the crew work.
				UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
				if (FParse::Param(FCommandLine::Get(), TEXT("TDOpsDemo")) && Mission && Mission->BeginDesign(TEXT("FirstLight"), false))
				{
					Mission->FirstFlight.bSkipped = true;
					Mission->UseRecommendedDesign();
					Mission->SetAllRolesGo();
					if (Mission->LaunchDesign())
					{
						HandleTransitDone();
					}
				}
				return;
			}
		}
	}
#endif

	ShowDesignScreen();
	if (DesignRoom.IsValid())
	{
		DesignRoom->Show(ETDDesignScreen::Objectives);
	}
	UE_LOG(LogTouchdownMars, Display, TEXT("Design: room open"));
}

void ATDPlayerController::ShowDebrief()
{
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	if (!Mission || !Mission->HasFlownDesign())
	{
		return;
	}
	ShowDesignScreen();
	if (DesignRoom.IsValid())
	{
		DesignRoom->Show(ETDDesignScreen::Debrief);
	}
	UE_LOG(LogTouchdownMars, Display, TEXT("Design: debrief open"));
}

void ATDPlayerController::HandleTransitDone()
{
	HideDesignScreen();
	UE_LOG(LogTouchdownMars, Display, TEXT("Design: landed, surface mission running"));
#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("TDPadTest")))
	{
		GetWorldTimerManager().SetTimer(PadTestTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { RunPadTest(2); }), 3.f, false);
	}
	// Automated test runs (-TDAutoPlay): play the surface mission to the end to reach the debrief.
	if (FParse::Param(FCommandLine::Get(), TEXT("TDOpsDemo")))
	{
		GetWorldTimerManager().SetTimer(OpsDemoTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
			if (!Mission || !Mission->IsMissionRunning() || Mission->IsRunningPhase())
			{
				return;
			}
			Mission->AutoPlanTasks();
			Mission->DevFillPhase();
			RunPhase();
		}), 6.f, true, 3.f);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("TDAutoPlay")) && GEngine)
	{
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			GEngine->Exec(GetWorld(), TEXT("td.AutoPlay"));
		}), 2.f, false);
	}
#endif
}

void ATDPlayerController::HandleNextPhase()
{
	if (bDesignScreenOpen)
	{
		return;
	}
	if (const ATDAstronautCharacter* Astronaut = Cast<ATDAstronautCharacter>(GetPawn()); Astronaut && !Astronaut->IsConsoleOpen())
	{
		return; // Space is jump while walking
	}
	RunPhase();
}

void ATDPlayerController::RunPhase()
{
	UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	if (!Mission || !Mission->CanEndPhase() || Mission->IsRunningPhase())
	{
		return;
	}
	// Watch the crew work: close the overlays and give the view back.
	HideBaseComputer();
	if (ATDAstronautCharacter* Astronaut = Cast<ATDAstronautCharacter>(GetPawn()))
	{
		Astronaut->SetConsoleOpen(false);
	}
	Mission->RunPhase();
}

void ATDPlayerController::ToggleBaseComputer()
{
	if (bDesignScreenOpen)
	{
		return;
	}
	if (bBaseOpen)
	{
		HideBaseComputer();
		return;
	}
	UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	if (!Mission || !Mission->HasOps() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	if (!BaseComputer.IsValid())
	{
		SAssignNew(BaseComputer, STDBaseComputer)
			.Mission(Mission)
			.OnClose(FTDBaseComputerAction::CreateUObject(this, &ATDPlayerController::HideBaseComputer))
			.OnRun(FTDBaseComputerAction::CreateUObject(this, &ATDPlayerController::RunPhase));
	}
	BaseComputer->Rebuild();
	GEngine->GameViewport->AddViewportWidgetContent(BaseComputer.ToSharedRef(), 45);
	bBaseOpen = true;
	if (Hud)
	{
		Hud->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(BaseComputer);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	SetShowMouseCursor(true);
}

void ATDPlayerController::HideBaseComputer()
{
	if (!bBaseOpen)
	{
		return;
	}
	if (BaseComputer.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(BaseComputer.ToSharedRef());
	}
	bBaseOpen = false;
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
	if (Hud)
	{
		Hud->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (ATDAstronautCharacter* Astronaut = Cast<ATDAstronautCharacter>(GetPawn()))
	{
		Astronaut->SetConsoleOpen(Astronaut->IsConsoleOpen());
	}
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

void ATDPlayerController::HandleBack()
{
	// Esc: close the card back or an open panel, else the pause menu (gdd.md §2 Controls).
	if (Hud)
	{
		Hud->HandleBack();
	}
}

bool ATDPlayerController::WantsPadCursor() const
{
	if (bDesignScreenOpen || bBaseOpen)
	{
		return true;
	}
	if (const ATDAstronautCharacter* Astronaut = Cast<ATDAstronautCharacter>(GetPawn()); Astronaut && Astronaut->IsConsoleOpen())
	{
		return true;
	}
	// The mission report at the end (no design flow) is a mouse screen too.
	const UTDMissionSubsystem* Mission = GetWorld() ? GetWorld()->GetSubsystem<UTDMissionSubsystem>() : nullptr;
	return Mission && Mission->HasMission() && !Mission->IsMissionRunning();
}

void ATDPlayerController::HandlePadBack()
{
	if (bBaseOpen)
	{
		HideBaseComputer();
		return;
	}
	if (ATDAstronautCharacter* Astronaut = Cast<ATDAstronautCharacter>(GetPawn()); Astronaut && Astronaut->IsConsoleOpen() && !bDesignScreenOpen)
	{
		Astronaut->SetConsoleOpen(false);
	}
}

#if !UE_BUILD_SHIPPING
void ATDPlayerController::RunPadTest(int32 Step)
{
	FSlateApplication& Slate = FSlateApplication::Get();
	const FPlatformUserId User = IPlatformInputDeviceMapper::Get().GetPrimaryPlatformUser();
	const FInputDeviceId Device = IPlatformInputDeviceMapper::Get().GetDefaultInputDevice();
	UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	auto Next = [this](int32 S, float Delay)
	{
		GetWorldTimerManager().SetTimer(PadTestTimer, FTimerDelegate::CreateWeakLambda(this, [this, S]() { RunPadTest(S); }), Delay, false);
	};
	switch (Step)
	{
	case 0:
	{
		// Left stick right for 0.4 s: the pad cursor must move.
		const FVector2D Before = Slate.GetCursorPos();
		Slate.OnControllerAnalog(FGamepadKeyNames::LeftAnalogX, User, Device, 1.f);
		GetWorldTimerManager().SetTimer(PadTestTimer, FTimerDelegate::CreateWeakLambda(this, [this, Before, User, Device]()
		{
			FSlateApplication::Get().OnControllerAnalog(FGamepadKeyNames::LeftAnalogX, User, Device, 0.f);
			const FVector2D After = FSlateApplication::Get().GetCursorPos();
			UE_LOG(LogTouchdownMars, Display, TEXT("PadTest: cursor moved %.0f px with the left stick (screen open %d)"), (After - Before).Size(), WantsPadCursor() ? 1 : 0);
			RunPadTest(1);
		}), 0.4f, false);
		break;
	}
	case 1:
	{
		// Put the cursor on "Take the console" (design canvas 322,787) and press A.
		if (GEngine && GEngine->GameViewport)
		{
			const FGeometry& G = GEngine->GameViewport->GetGameViewportWidget()->GetCachedGeometry();
			const FVector2D Size = G.GetLocalSize();
			const float Scale = FMath::Min(Size.X / 1920.f, Size.Y / 1080.f);
			const FVector2D Offset = (Size - FVector2D(1920.f, 1080.f) * Scale) * 0.5f;
			const FVector2D Screen = G.LocalToAbsolute(Offset + FVector2D(322.f, 787.f) * Scale);
			if (TSharedPtr<FSlateUser> SlateUser = Slate.GetCursorUser())
			{
				// A real stick move sends mouse-move events (hover); the teleport here sends one by hand.
				const FVector2D Old = SlateUser->GetCursorPosition();
				SlateUser->SetCursorPosition(Screen);
				const FPointerEvent Move(SlateUser->GetUserIndex(), FSlateApplication::CursorPointerIndex, Screen, Old, TSet<FKey>(), FKey(), 0.f, FModifierKeysState());
				Slate.ProcessMouseMoveEvent(Move);
			}
		}
		GetWorldTimerManager().SetTimer(PadTestTimer, FTimerDelegate::CreateWeakLambda(this, [this, Mission, User, Device]()
		{
			FSlateApplication::Get().OnControllerButtonPressed(FGamepadKeyNames::FaceButtonBottom, User, Device, false);
			FSlateApplication::Get().OnControllerButtonReleased(FGamepadKeyNames::FaceButtonBottom, User, Device, false);
			GetWorldTimerManager().SetTimer(PadTestTimer, FTimerDelegate::CreateWeakLambda(this, [Mission]()
			{
				UE_LOG(LogTouchdownMars, Display, TEXT("PadTest: A on 'Take the console' -> cold open closed %d"), Mission && Mission->FirstFlight.bColdOpenSeen ? 1 : 0);
			}), 1.f, false);
		}), 0.3f, false);
		break;
	}
	case 2:
	{
		// On the surface: left stick forward for 2 s must walk the astronaut.
		if (APawn* P = GetPawn())
		{
			PadTestStart = P->GetActorLocation();
		}
		Slate.OnControllerAnalog(FGamepadKeyNames::LeftAnalogY, User, Device, 1.f);
		Next(3, 2.f);
		break;
	}
	case 3:
	{
		Slate.OnControllerAnalog(FGamepadKeyNames::LeftAnalogY, User, Device, 0.f);
		const float Walked = GetPawn() ? FVector::Dist2D(GetPawn()->GetActorLocation(), PadTestStart) : 0.f;
		UE_LOG(LogTouchdownMars, Display, TEXT("PadTest: left stick walked %.0f cm in 2 s"), Walked);
		// View button opens the base computer, B closes it.
		Slate.OnControllerButtonPressed(FGamepadKeyNames::SpecialLeft, User, Device, false);
		Slate.OnControllerButtonReleased(FGamepadKeyNames::SpecialLeft, User, Device, false);
		Next(4, 1.f);
		break;
	}
	case 4:
	{
		UE_LOG(LogTouchdownMars, Display, TEXT("PadTest: View -> base computer open %d"), bBaseOpen ? 1 : 0);
		Slate.OnControllerButtonPressed(FGamepadKeyNames::FaceButtonRight, User, Device, false);
		Slate.OnControllerButtonReleased(FGamepadKeyNames::FaceButtonRight, User, Device, false);
		Next(5, 1.f);
		break;
	}
	case 5:
		UE_LOG(LogTouchdownMars, Display, TEXT("PadTest: B -> base computer open %d"), bBaseOpen ? 1 : 0);
		break;
	default:
		break;
	}
}
#endif
