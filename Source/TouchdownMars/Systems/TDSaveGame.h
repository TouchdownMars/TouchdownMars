// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Model/TDTypes.h"
#include "TDSaveGame.generated.h"

/** Slot TD_Settings. No personal data, no names, no analytics. */
UCLASS()
class TOUCHDOWNMARS_API UTDSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static const TCHAR* SlotName() { return TEXT("TD_Settings"); }

	UPROPERTY()
	ETDLanguage Language = ETDLanguage::EN;

	UPROPERTY()
	ETDAgeBand Band = ETDAgeBand::Commander;

	UPROPERTY()
	bool bSoundOn = true;

	UPROPERTY()
	bool bReduceMotion = false;

	UPROPERTY()
	bool bFullscreen = false;

	/** Outcome Shots at loop step 4 (camera.md). No menu entry; config / console only. */
	UPROPERTY()
	bool bOutcomeShotsEnabled = true;

	/** Look speed multiplier (mouse and right stick). */
	UPROPERTY()
	float LookSpeed = 1.f;

	UPROPERTY()
	bool bInvertLookY = false;

	/** Accessibility: one press works a spot through instead of holding. */
	UPROPERTY()
	bool bTapToWork = false;

	UPROPERTY()
	int32 LastSeed = 0;

	UPROPERTY()
	bool bHasLastReport = false;

	UPROPERTY()
	FTDMissionReport LastReport;
};
