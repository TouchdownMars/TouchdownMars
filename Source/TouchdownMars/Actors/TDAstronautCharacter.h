// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
// First-person astronaut: walk the outpost on Mars gravity; the mission console (the existing decision HUD)
// opens on the wrist tablet (Tab). Input actions are created in code (no input assets needed).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TDAstronautCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

UCLASS()
class TOUCHDOWNMARS_API ATDAstronautCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATDAstronautCharacter();

	/** Eye height above the capsule centre (cm); capsule half height 92 cm puts the eyes near 1.62 m. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Astronaut")
	float EyeHeight = 70.f;

	/** Suited walking speed (cm/s), a designer value. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Astronaut")
	float WalkSpeed = 140.f;

	/** Mouse look sensitivity (degrees per mouse unit). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Astronaut")
	float LookSensitivity = 0.5f;

	/** Field of view (degrees, horizontal); 90 on a monitor (accessibility guidance). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Astronaut")
	float FieldOfView = 90.f;

	/** How far the helmet looks for a named object (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Astronaut")
	float InspectDistance = 900.f;

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Astronaut")
	bool IsConsoleOpen() const { return bConsoleOpen; }

	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Astronaut")
	void SetConsoleOpen(bool bOpen);

	/** Name of what the astronaut looks at (crew member, system, vehicle), empty if nothing. */
	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Astronaut")
	FText GetFocusText() const { return FocusText; }

	UFUNCTION(BlueprintPure, Category = "Touchdown Mars|Astronaut")
	float GetMarsGravityRatio() const { return GravityRatio; }

	/** Prompt for the HUD when one of your own tasks is in reach ("E: Off-load the rover"). */
	FText GetInteractPrompt() const;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Touchdown Mars|Astronaut")
	TObjectPtr<UCameraComponent> Camera;

private:
	void CreateInput();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ToggleConsole();
	void UpdateFocus();
	bool ApplyMarsGravity();
	void SyncHud();

	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> Context;
	UPROPERTY(Transient) TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LookAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> ConsoleAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> OpsAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> InteractAction;
	void ToggleOps();
	void Interact();
	/** Your planned task whose site is within reach (none if far). */
	FName NearbyOwnTask() const;

	bool bConsoleOpen = false;
	bool bGravityApplied = false;
	bool bHudSynced = false;
	float GravityRatio = 1.f;
	FText FocusText;
};
