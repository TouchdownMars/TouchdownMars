// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Model/TDOperations.h"

namespace
{
	const TCHAR* Ctx = TEXT("TDOps");

	const FTDTaskRow* FindTask(const UDataTable* Tasks, FName Id)
	{
		return Tasks && !Id.IsNone() ? Tasks->FindRow<FTDTaskRow>(Id, Ctx, false) : nullptr;
	}

	float PlannedHoursOf(const FTDOpsState& Ops, int32 CrewIndex, const UDataTable* Tasks, const UDataTable* Constants, bool bEvaOnly)
	{
		float Hours = 0.f;
		if (!Ops.Crew.IsValidIndex(CrewIndex))
		{
			return 0.f;
		}
		for (const FName& Id : Ops.Crew[CrewIndex].Tasks)
		{
			if (const FTDTaskRow* Row = FindTask(Tasks, Id))
			{
				if (!bEvaOnly || Row->bEva)
				{
					Hours += UTDOps::TaskHours(*Row, Constants);
				}
			}
		}
		return Hours;
	}
}

FTDOpsState UTDOps::MakeState()
{
	FTDOpsState Ops;
	// Crew 0 is the player; the other three are AI crew with their own roles (gdd.md "Base operations").
	const ETDRole Roles[] = { ETDRole::Engineer, ETDRole::Planner, ETDRole::Science, ETDRole::Comms };
	for (int32 Index = 0; Index < 4; ++Index)
	{
		FTDCrewPlan& Crew = Ops.Crew.AddDefaulted_GetRef();
		Crew.Role = Roles[Index];
		Crew.bPlayer = Index == 0;
	}
	Ops.PlannedHours.Init(0.f, 4);
	Ops.Fatigue.Init(0, 4);
	return Ops;
}

float UTDOps::PhaseHours(int32 Sols, const UDataTable* Constants)
{
	return FMath::Max(1, Sols) * UTDDesignRules::Constant(Constants, TEXT("CrewWorkHoursPerSol"));
}

float UTDOps::CrewPhaseHours(const FTDOpsState& Ops, int32 CrewIndex, int32 Sols, const UDataTable* Constants)
{
	const float Full = PhaseHours(Sols, Constants);
	const bool bTired = Ops.Fatigue.IsValidIndex(CrewIndex) && Ops.Fatigue[CrewIndex] > 0;
	return bTired ? Full * static_cast<float>(UTDDesignRules::Constant(Constants, TEXT("FatigueWorkShare"))) : Full;
}

float UTDOps::PhaseEvaHours(int32 Sols, const UDataTable* Constants)
{
	// NASA HEOMD-415: at least one non-EVA sol between EVAs, so at most every second sol is an EVA sol.
	const int32 EvaSols = (FMath::Max(0, Sols) + 1) / 2;
	return EvaSols * UTDDesignRules::Constant(Constants, TEXT("EvaHoursPerEvaSol"));
}

float UTDOps::TaskHours(const FTDTaskRow& Row, const UDataTable* Constants)
{
	return FMath::Max(0.25f, static_cast<float>(UTDDesignRules::Constant(Constants, Row.HoursKey)));
}

TArray<int32> UTDOps::CrewOn(const FTDOpsState& Ops, FName TaskId)
{
	TArray<int32> Out;
	for (int32 Index = 0; Index < Ops.Crew.Num(); ++Index)
	{
		if (Ops.Crew[Index].Tasks.Contains(TaskId))
		{
			Out.Add(Index);
		}
	}
	return Out;
}

ETDTaskBlock UTDOps::CanPlan(const FTDOpsState& Ops, const UDataTable* Tasks, FName TaskId, int32 CrewIndex, const FTDMissionDesign& Design,
	const UDataTable* Options, int32 PhaseIndex, int32 PhaseSols, bool bDustStorm, const UDataTable* Constants, bool bOverrideRest)
{
	const FTDTaskRow* Row = FindTask(Tasks, TaskId);
	if (!Row || !Ops.Crew.IsValidIndex(CrewIndex))
	{
		return ETDTaskBlock::WrongPhase;
	}
	if (Row->PhaseMask != 0 && (Row->PhaseMask & (1 << PhaseIndex)) == 0)
	{
		return ETDTaskBlock::WrongPhase;
	}
	if (!Row->RequiresOption.IsNone() && UTDDesignRules::GetPickCount(Design, Row->RequiresOption) == 0)
	{
		return ETDTaskBlock::NotLanded;
	}
	if (!Row->Prerequisite.IsNone() && !IsDone(Ops, Row->Prerequisite) && CrewOn(Ops, Row->Prerequisite).Num() == 0)
	{
		return ETDTaskBlock::NeedsFirst;
	}
	if (!Row->bRepeatable && IsDone(Ops, TaskId))
	{
		return ETDTaskBlock::AlreadyDone;
	}
	if (Row->Effect == ETDTaskEffect::CleanSolar && Ops.SolarDustCells == 0)
	{
		return ETDTaskBlock::AlreadyDone; // arrays are clean
	}
	if (Row->Effect == ETDTaskEffect::ExtractWater && !Ops.bWaterFound)
	{
		return ETDTaskBlock::NeedsFirst;
	}
	if ((Row->bEva || Row->Effect == ETDTaskEffect::Expedition) && bDustStorm)
	{
		return ETDTaskBlock::Storm;
	}
	// After a traverse the rover recharges on the fission grid (NASA HEOMD-415: about 2 sols on a 10 kW unit).
	if (Ops.RoverChargingPhase == PhaseIndex && (Row->Effect == ETDTaskEffect::Expedition || Row->Prerequisite == FName(TEXT("OffloadRover"))))
	{
		return ETDTaskBlock::RoverCharging;
	}
	const float Hours = TaskHours(*Row, Constants);
	const float EvaBudget = PhaseEvaHours(PhaseSols, Constants);
	auto Fits = [&](int32 Index, ETDTaskBlock& Why)
	{
		if (PlannedHoursOf(Ops, Index, Tasks, Constants, false) + Hours > CrewPhaseHours(Ops, Index, PhaseSols, Constants) + 0.01f)
		{
			Why = ETDTaskBlock::NoHours;
			return false;
		}
		if (Row->bEva && !bOverrideRest && PlannedHoursOf(Ops, Index, Tasks, Constants, true) + Hours > EvaBudget + 0.01f)
		{
			Why = ETDTaskBlock::NoEvaHours;
			return false;
		}
		return true;
	};
	ETDTaskBlock Why = ETDTaskBlock::Ok;
	if (!Fits(CrewIndex, Why))
	{
		return Why;
	}
	// Team tasks (and every EVA: never alone outside) need enough other crew with the hours.
	const int32 Team = FMath::Max(Row->Crew, Row->bEva ? 2 : 1);
	int32 Free = 1;
	for (int32 Index = 0; Index < Ops.Crew.Num() && Free < Team; ++Index)
	{
		ETDTaskBlock Ignored;
		if (Index != CrewIndex && !Ops.Crew[Index].Tasks.Contains(TaskId) && Fits(Index, Ignored))
		{
			++Free;
		}
	}
	return Free >= Team ? ETDTaskBlock::Ok : ETDTaskBlock::NeedsBuddy;
}

bool UTDOps::Plan(FTDOpsState& Ops, const UDataTable* Tasks, FName TaskId, int32 CrewIndex, const FTDMissionDesign& Design,
	const UDataTable* Options, int32 PhaseIndex, int32 PhaseSols, bool bDustStorm, const UDataTable* Constants, bool bOverrideRest)
{
	if (CrewOn(Ops, TaskId).Num() > 0
		|| CanPlan(Ops, Tasks, TaskId, CrewIndex, Design, Options, PhaseIndex, PhaseSols, bDustStorm, Constants, bOverrideRest) != ETDTaskBlock::Ok)
	{
		return false;
	}
	const FTDTaskRow* Row = FindTask(Tasks, TaskId);
	const int32 Team = FMath::Max(Row->Crew, Row->bEva ? 2 : 1);
	Ops.Crew[CrewIndex].Tasks.Add(TaskId);
	// Fill the team with the free crew who have the most hours left (role match first).
	TArray<int32> Others;
	for (int32 Index = 0; Index < Ops.Crew.Num(); ++Index)
	{
		if (Index != CrewIndex)
		{
			Others.Add(Index);
		}
	}
	const float Hours = TaskHours(*Row, Constants);
	const float EvaBudget = PhaseEvaHours(PhaseSols, Constants);
	Others.Sort([&](int32 A, int32 B)
	{
		if (Ops.Crew[A].bPlayer != Ops.Crew[B].bPlayer)
		{
			return !Ops.Crew[A].bPlayer;
		}
		const bool RoleA = Ops.Crew[A].Role == Row->Role;
		const bool RoleB = Ops.Crew[B].Role == Row->Role;
		if (RoleA != RoleB)
		{
			return RoleA;
		}
		return PlannedHoursOf(Ops, A, Tasks, Constants, false) < PlannedHoursOf(Ops, B, Tasks, Constants, false);
	});
	int32 Count = 1;
	for (int32 Index : Others)
	{
		if (Count >= Team)
		{
			break;
		}
		const bool bHours = PlannedHoursOf(Ops, Index, Tasks, Constants, false) + Hours <= CrewPhaseHours(Ops, Index, PhaseSols, Constants) + 0.01f;
		const bool bEva = !Row->bEva || bOverrideRest || PlannedHoursOf(Ops, Index, Tasks, Constants, true) + Hours <= EvaBudget + 0.01f;
		if (bHours && bEva)
		{
			Ops.Crew[Index].Tasks.Add(TaskId);
			++Count;
		}
	}
	for (int32 Index = 0; Index < Ops.Crew.Num(); ++Index)
	{
		Ops.PlannedHours[Index] = PlannedHoursOf(Ops, Index, Tasks, Constants, false);
	}
	RecomputeRestBroken(Ops, Tasks, PhaseSols, Constants);
	return true;
}

void UTDOps::Unplan(FTDOpsState& Ops, FName TaskId)
{
	for (FTDCrewPlan& Crew : Ops.Crew)
	{
		Crew.Tasks.Remove(TaskId);
	}
}

void UTDOps::RecomputeRestBroken(FTDOpsState& Ops, const UDataTable* Tasks, int32 PhaseSols, const UDataTable* Constants)
{
	// Spacewalk hours beyond the rest-sol rule (only possible with an override): a medical flag and fatigue next phase.
	Ops.RestBroken.Reset();
	const float EvaBudget = PhaseEvaHours(PhaseSols, Constants);
	for (int32 Index = 0; Index < Ops.Crew.Num(); ++Index)
	{
		if (PlannedHoursOf(Ops, Index, Tasks, Constants, true) > EvaBudget + 0.01f)
		{
			Ops.RestBroken.Add(Index);
		}
	}
}

void UTDOps::AutoPlan(FTDOpsState& Ops, const UDataTable* Tasks, const FTDMissionDesign& Design, const UDataTable* Options,
	int32 PhaseIndex, int32 PhaseSols, bool bDustStorm, const UDataTable* Constants)
{
	if (!Tasks)
	{
		return;
	}
	// Each AI crew member takes the open tasks of their own role first, then any other open task (work priorities).
	TArray<FName> Ids = Tasks->GetRowNames();
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		for (int32 CrewIndex = 1; CrewIndex < Ops.Crew.Num(); ++CrewIndex)
		{
			for (const FName& Id : Ids)
			{
				const FTDTaskRow* Row = FindTask(Tasks, Id);
				if (!Row || (Pass == 0 && Row->Role != Ops.Crew[CrewIndex].Role) || CrewOn(Ops, Id).Num() > 0)
				{
					continue;
				}
				Plan(Ops, Tasks, Id, CrewIndex, Design, Options, PhaseIndex, PhaseSols, bDustStorm, Constants);
			}
		}
	}
}

TArray<FName> UTDOps::Finish(FTDOpsState& Ops, const UDataTable* Tasks)
{
	TArray<FName> Finished;
	for (const FTDCrewPlan& Crew : Ops.Crew)
	{
		for (const FName& Id : Crew.Tasks)
		{
			Finished.AddUnique(Id);
		}
	}
	for (const FName& Id : Finished)
	{
		Ops.Done.FindOrAdd(Id) += 1;
	}
	for (FTDCrewPlan& Crew : Ops.Crew)
	{
		Crew.Tasks.Reset();
	}
	Ops.PlannedHours.Init(0.f, Ops.Crew.Num());
	// Fatigue: one phase of reduced hours after the rest-sol rule was broken.
	if (Ops.Fatigue.Num() != Ops.Crew.Num())
	{
		Ops.Fatigue.Init(0, Ops.Crew.Num());
	}
	for (int32& Left : Ops.Fatigue)
	{
		Left = FMath::Max(0, Left - 1);
	}
	for (int32 Index : Ops.RestBroken)
	{
		if (Ops.Fatigue.IsValidIndex(Index))
		{
			Ops.Fatigue[Index] = 1;
		}
	}
	Ops.RestBreaks += Ops.RestBroken.Num();
	Ops.RestBroken.Reset();
	return Finished;
}

void UTDOps::ApplyEffect(const FTDTaskRow& Row, FTDOpsState& Ops, FTDMissionState& State, const UDataTable* Constants)
{
	const int32 LevelMin = FMath::RoundToInt(UTDDesignRules::Constant(Constants, TEXT("LevelMin")));
	const int32 LevelMax = FMath::Max(LevelMin + 1, FMath::RoundToInt(UTDDesignRules::Constant(Constants, TEXT("LevelMax"))));
	auto Raise = [&State, LevelMin, LevelMax](ETDSystem System)
	{
		const int32 Index = static_cast<int32>(System);
		if (State.Levels.IsValidIndex(Index))
		{
			State.Levels[Index] = FMath::Clamp(State.Levels[Index] + 1, LevelMin, LevelMax);
		}
	};
	switch (Row.Effect)
	{
	case ETDTaskEffect::SetUpGreenhouse: Raise(ETDSystem::Greenhouse); break;
	case ETDTaskEffect::Harvest: State.bHarvest = true; break;
	case ETDTaskEffect::ScienceTraverse: ++State.ScienceWalks; break;
	case ETDTaskEffect::Survey: Ops.bWaterFound = true; break;
	case ETDTaskEffect::ExtractWater:
		Ops.WaterKg += UTDDesignRules::Constant(Constants, TEXT("WaterPerExtractionKg"));
		Raise(ETDSystem::Air); // oxygen by electrolysis of the extracted water
		break;
	case ETDTaskEffect::BuildBerm:
		Ops.RegolithKg += UTDDesignRules::Constant(Constants, TEXT("RegolithPerBermTaskKg"));
		++State.OpsRadiationRelief;
		break;
	case ETDTaskEffect::CleanSolar: Ops.SolarDustCells = 0; break;
	case ETDTaskEffect::Maintain:
	{
		int32 Lowest = 0;
		for (int32 Index = 1; Index < State.Levels.Num(); ++Index)
		{
			if (State.Levels[Index] < State.Levels[Lowest])
			{
				Lowest = Index;
			}
		}
		Raise(static_cast<ETDSystem>(Lowest));
		break;
	}
	case ETDTaskEffect::Weather: Ops.bWeatherStation = true; break;
	case ETDTaskEffect::Expedition:
		State.ScienceWalks += FMath::RoundToInt(UTDDesignRules::Constant(Constants, TEXT("ExpeditionScienceWalks")));
		Ops.RoverChargingPhase = State.PhaseIndex + 1;
		break;
	case ETDTaskEffect::LinkCheck: State.bCalledHomeThisPhase = false; break;
	default: break;
	}
}

TArray<FName> UTDOps::FindPath(const UDataTable* Sites, FName From, FName To)
{
	TArray<FName> Path;
	if (!Sites || From.IsNone() || To.IsNone())
	{
		return Path;
	}
	auto Pos = [Sites](FName Id)
	{
		const FTDSiteRow* Row = Sites->FindRow<FTDSiteRow>(Id, Ctx, false);
		return Row ? FVector2D(Row->X, Row->Y) : FVector2D::ZeroVector;
	};
	TMap<FName, float> Dist;
	TMap<FName, FName> Prev;
	TSet<FName> Open;
	for (const FName& Id : Sites->GetRowNames())
	{
		Dist.Add(Id, TNumericLimits<float>::Max());
		Open.Add(Id);
	}
	if (!Dist.Contains(From) || !Dist.Contains(To))
	{
		return Path;
	}
	Dist[From] = 0.f;
	while (Open.Num() > 0)
	{
		FName Best;
		float BestDist = TNumericLimits<float>::Max();
		for (const FName& Id : Open)
		{
			if (Dist[Id] < BestDist)
			{
				BestDist = Dist[Id];
				Best = Id;
			}
		}
		if (Best.IsNone() || Best == To)
		{
			break;
		}
		Open.Remove(Best);
		const FTDSiteRow* Row = Sites->FindRow<FTDSiteRow>(Best, Ctx, false);
		if (!Row)
		{
			continue;
		}
		for (const FName& Next : Row->Links)
		{
			if (!Open.Contains(Next))
			{
				continue;
			}
			const float Alt = BestDist + FVector2D::Distance(Pos(Best), Pos(Next));
			if (Alt < Dist[Next])
			{
				Dist[Next] = Alt;
				Prev.Add(Next, Best);
			}
		}
	}
	if (Dist[To] == TNumericLimits<float>::Max())
	{
		return Path;
	}
	for (FName At = To; !At.IsNone(); At = Prev.FindRef(At))
	{
		Path.Insert(At, 0);
		if (At == From)
		{
			break;
		}
	}
	return Path;
}
