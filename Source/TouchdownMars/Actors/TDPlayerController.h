// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TDPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UTDHudWidget;
class STDDesignRoom;
class STDBaseComputer;

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

	/** Mission Design Room (Act 1): objective cards first. */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Design")
	void OpenDesignRoom();

	/** Mission Debrief (Act 3) after lift-off or loss, when a designed mission flew. */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Design")
	void ShowDebrief();

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Design")
	bool IsDesignScreenOpen() const { return bDesignScreenOpen; }

	/** Base computer (key B): crew plan and task board for the current phase. */
	void ToggleBaseComputer();
	bool IsBaseComputerOpen() const { return bBaseOpen; }

	/** Runs the phase as a time-lapse (base computer button, Next phase, Space). */
	void RunPhase();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleNextPhase();
	void HandleBack();
	void ShowDesignScreen();
	void HideDesignScreen();
	void HandleTransitDone();

	TSharedPtr<STDDesignRoom> DesignRoom;
	bool bDesignScreenOpen = false;

	void HideBaseComputer();

	/** Gamepad: cursor for the mouse screens (registered with Slate), B = back, Menu = run the phase. */
	bool WantsPadCursor() const;
	void HandlePadBack();
	TSharedPtr<class FTDGamepadCursor> PadCursor;

#if !UE_BUILD_SHIPPING
	/** -TDPadTest: injects real gamepad events into Slate (cursor click on the cold open, then a left-stick walk). */
	void RunPadTest(int32 Step);
	FTimerHandle PadTestTimer;
	FVector PadTestStart;
#endif

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> PadContext;
	TSharedPtr<STDBaseComputer> BaseComputer;
	FTimerHandle OpsDemoTimer;
	bool bBaseOpen = false;

	UPROPERTY(Transient)
	TObjectPtr<UTDHudWidget> Hud;
};
