// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Model/TDTypes.h"
#include "TDGameMode.generated.h"

class UTDShotDirector;

/** No pawn; starts the mission after every actor and the HUD have bound to the subsystem. */
UCLASS()
class TOUCHDOWNMARS_API ATDGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATDGameMode();

	virtual void StartPlay() override;

	/** Default Commander (gdd.md §2 Mechanic 8, locked 13.11.2026). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touchdown Mars")
	ETDAgeBand StartBand = ETDAgeBand::Commander;

	/** True = the band from the player's settings (UTDSaveGame); false = StartBand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touchdown Mars")
	bool bUseSavedBand = true;

	/** 0 = random four-digit seed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touchdown Mars")
	int32 StartSeed = 0;

	/** Main camera (CAM_Main) and Outcome Shots. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Camera")
	TObjectPtr<UTDShotDirector> ShotDirector;
};
