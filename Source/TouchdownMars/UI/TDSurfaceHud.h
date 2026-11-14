// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
// Helmet HUD on the surface, in the design-room style (TDStyle): tracked objective (top left), life support and
// power (top right), comms log (bottom left), hold-to-work ring and prompt (centre), controls (bottom right).
// Fixed positions, scaled from a 1920x1080 canvas so it never blows up on a different screen or DPI.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UTDMissionSubsystem;
class ATDPlayerController;

class TOUCHDOWNMARS_API STDSurfaceHud : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STDSurfaceHud) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UTDMissionSubsystem>, Mission)
		SLATE_ARGUMENT(TWeakObjectPtr<ATDPlayerController>, Controller)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	EVisibility HudVisibility() const;
	TSharedRef<SWidget> BuildObjective();
	TSharedRef<SWidget> BuildLife();
	TSharedRef<SWidget> BuildComms();
	TSharedRef<SWidget> BuildCentre();
	TSharedRef<SWidget> BuildControls();
	FText Txt(const TCHAR* Key) const;

	TWeakObjectPtr<UTDMissionSubsystem> Mission;
	TWeakObjectPtr<ATDPlayerController> Controller;
	mutable FString LastObjective;
	mutable double ObjectiveChanged = 0.0;
};
