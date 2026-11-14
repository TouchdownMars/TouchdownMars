// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
// One tracked objective (research note §6.3, No Man's Sky "Beyond"): the HUD always says what to do next, where,
// and why the phase cannot run yet. Shared by the helmet HUD (text) and the canvas HUD (world marker).

#pragma once

#include "CoreMinimal.h"

class UTDMissionSubsystem;
class ATDAstronautCharacter;

enum class ETDObjectiveKind : uint8
{
	None,
	Decision,   // event waiting at Mission Control
	Power,      // cells not all placed
	PlanCrew,   // nobody has a task yet
	OwnTask,    // your hands-on task
	RunSols,    // all set: run the phase at Mission Control
	Running     // time-lapse
};

struct FTDObjective
{
	ETDObjectiveKind Kind = ETDObjectiveKind::None;
	FText Title;
	FText Step;
	bool bHasTarget = false;
	FVector Target = FVector::ZeroVector;
	float Distance = 0.f; // cm, 2D
	bool bUrgent = false;
};

namespace TDObjective
{
	TOUCHDOWNMARS_API FTDObjective Compute(const UTDMissionSubsystem* Mission, const ATDAstronautCharacter* Astronaut);
}
