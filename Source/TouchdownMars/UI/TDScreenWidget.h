// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
// Diegetic screens: the Mission Control console face and the field tablet in the player's hand. Drawn in code
// (NativePaint), shown in the world through a UWidgetComponent, unlit so they read as lit displays in daylight.
// The console reads the mission state itself; the tablet is fed by the astronaut (SetTablet).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TDScreenWidget.generated.h"

UENUM()
enum class ETDScreenMode : uint8
{
	Console,
	Tablet
};

UCLASS(NotBlueprintable)
class TOUCHDOWNMARS_API UTDScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	ETDScreenMode Mode = ETDScreenMode::Console;

	/** Tablet feed: task line, beat progress of the spot in reach (0-1), spots done / total, working now. */
	void SetTablet(const FText& InTitle, float InProgress, int32 InSpotsDone, int32 InSpotsTotal, bool bInWorking);
	/** Console: Mission Control wants the player (power not routed, decision waiting). */
	void SetNeedsYou(bool bInNeeds) { bNeedsYou = bInNeeds; }

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
		FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;

private:
	int32 PaintConsole(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 LayerId, float Now) const;
	int32 PaintTablet(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 LayerId, float Now) const;

	FText Title;
	float Progress = 0.f;
	int32 SpotsDone = 0;
	int32 SpotsTotal = 0;
	bool bWorking = false;
	bool bNeedsYou = false;
};
