// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Actors/TDGameMode.h"

#include "Actors/TDAstronautCharacter.h"
#include "Actors/TDPlayerController.h"
#include "UI/TDHelmetHUD.h"
#include "Actors/TDSceneActors.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Systems/TDMissionSubsystem.h"
#include "Systems/TDShotDirector.h"

ATDGameMode::ATDGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = ATDPlayerController::StaticClass();
	ShotDirector = CreateDefaultSubobject<UTDShotDirector>(TEXT("ShotDirector"));
	HUDClass = ATDHelmetHUD::StaticClass();
}

UClass* ATDGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	return bFirstPerson ? ATDAstronautCharacter::StaticClass() : nullptr;
}

void ATDGameMode::StartPlay()
{
	Super::StartPlay(); // BeginPlay runs here, so the HUD and scene actors are bound before the first broadcast.
	// The player starts without a pawn, which resets the view; point it at the fixed main camera afterwards.
	if (ShotDirector)
	{
		ShotDirector->ActivateMainCamera();
	}
	if (UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>())
	{
		Mission->RequestSpaceWeather();
		if (!bMissionDesign)
		{
			Mission->StartMission(bUseSavedBand ? Mission->GetPreferredBand() : StartBand, StartSeed);
		}
		else if (ATDPlayerController* PC = Cast<ATDPlayerController>(GetWorld()->GetFirstPlayerController()))
		{
			PC->OpenDesignRoom();
		}
	}
}
