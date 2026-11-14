// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
// Helmet display for first-person play (Canvas, works in Shipping): centre dot, what the astronaut looks at,
// and the console hint. The decision UI stays in the UMG HUD.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TDHelmetHUD.generated.h"

UCLASS()
class TOUCHDOWNMARS_API ATDHelmetHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawOps(const class UTDMissionSubsystem* Mission, const class ATDAstronautCharacter* Astronaut, class UFont* Font, float Scale);
	void DrawObjectiveMarker(const class UTDMissionSubsystem* Mission, const class ATDAstronautCharacter* Astronaut, class UFont* Font, float Scale);
};
