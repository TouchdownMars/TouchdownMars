// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TDPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UTDHudWidget;

/** Mouse cursor always visible; all play goes through the HUD. Space/Enter = Next phase, Esc = Back. */
UCLASS()
class TOUCHDOWNMARS_API ATDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATDPlayerController();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Touchdown Mars|UI")
	TSubclassOf<UTDHudWidget> HudWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Touchdown Mars|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Touchdown Mars|Input")
	TObjectPtr<UInputAction> NextPhaseAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Touchdown Mars|Input")
	TObjectPtr<UInputAction> BackAction;

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|UI")
	UTDHudWidget* GetMissionHud() const { return Hud; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void HandleNextPhase();
	void HandleBack();

	UPROPERTY(Transient)
	TObjectPtr<UTDHudWidget> Hud;
};
