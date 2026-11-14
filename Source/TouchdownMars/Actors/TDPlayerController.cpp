// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Actors/TDPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
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

void ATDPlayerController::HandleNextPhase()
{
	if (UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>())
	{
		Mission->EndPhase();
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
