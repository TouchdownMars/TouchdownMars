// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "UI/TDObjective.h"

#include "Actors/TDAstronautCharacter.h"
#include "Actors/TDWorkSpot.h"
#include "Engine/DataTable.h"
#include "Model/TDMissionRules.h"
#include "Model/TDOperations.h"
#include "Systems/TDMissionSubsystem.h"

namespace
{
	FText Meters(float Cm)
	{
		return FText::AsNumber(FMath::Max(0, FMath::RoundToInt(Cm / 100.f)));
	}
}

FTDObjective TDObjective::Compute(const UTDMissionSubsystem* Mission, const ATDAstronautCharacter* Astronaut)
{
	FTDObjective O;
	if (!Mission || !Mission->HasMission() || !Mission->IsMissionRunning())
	{
		return O;
	}
	const FTDMissionState& State = Mission->GetState();
	int32 SolStart = 0;
	int32 SolEnd = 0;
	Mission->GetPhaseSols(State.PhaseIndex, SolStart, SolEnd);
	const FVector Here = Astronaut ? Astronaut->GetActorLocation() : FVector::ZeroVector;
	const ATDMissionControlStation* Station = Astronaut ? ATDMissionControlStation::Find(Astronaut->GetWorld()) : nullptr;
	const bool bAtStation = Astronaut && Astronaut->IsNearStation();
	auto AtStation = [&](FTDObjective& Out, const FText& Step)
	{
		if (Station && !bAtStation)
		{
			Out.bHasTarget = true;
			Out.Target = Station->GetActorLocation() + FVector(0.f, 0.f, 260.f);
			Out.Distance = FVector::Dist2D(Here, Station->GetActorLocation());
			Out.Step = FText::Format(Mission->GetText(TEXT("Track_WalkTo")), Step, Meters(Out.Distance));
		}
		else
		{
			Out.Step = Step;
		}
	};

	if (Mission->IsRunningPhase())
	{
		O.Kind = ETDObjectiveKind::Running;
		O.Title = Mission->GetText(TEXT("Track_Running"));
		O.Step = FText::Format(Mission->GetText(TEXT("Ops_RunBanner")), SolStart, SolEnd);
		return O;
	}
	if (!State.CurrentEventId.IsNone() && State.EventChoice < 0)
	{
		O.Kind = ETDObjectiveKind::Decision;
		O.bUrgent = true;
		O.Title = Mission->GetText(TEXT("Track_Decision"));
		AtStation(O, Mission->GetText(TEXT("Track_DecisionStep")));
		return O;
	}
	const int32 Placed = UTDMissionRules::GetCellsPlaced(State);
	if (Placed < State.CellsTotal)
	{
		O.Kind = ETDObjectiveKind::Power;
		O.bUrgent = true;
		O.Title = Mission->GetText(TEXT("Track_Power"));
		AtStation(O, FText::Format(Mission->GetText(TEXT("Track_PowerStep")), State.CellsTotal - Placed, State.CellsTotal));
		return O;
	}

	if (Mission->HasOps())
	{
		const FTDOpsState& Ops = Mission->GetOps();
		// Your own hands-on task: the nearest unfinished work spot.
		if (Ops.Crew.Num() > 0 && Astronaut)
		{
			float Best = TNumericLimits<float>::Max();
			for (const FName& Task : Ops.Crew[0].Tasks)
			{
				int32 Done = 0;
				int32 Spots = 0;
				FVector Next;
				if (!Astronaut->GetTaskWork(Task, Done, Spots, Next))
				{
					continue;
				}
				const float Dist = FVector::Dist2D(Here, Next);
				if (Dist >= Best)
				{
					continue;
				}
				Best = Dist;
				const FTDTaskRow* Row = Mission->GetTasksTable() ? Mission->GetTasksTable()->FindRow<FTDTaskRow>(Task, TEXT("TDObjective"), false) : nullptr;
				const FTDHandsOnSpec Spec = TDHandsOn::SpecFor(Task);
				O.Kind = ETDObjectiveKind::OwnTask;
				O.Title = Row ? Mission->GetText(Row->NameKey) : FText::FromName(Task);
				const FText Verb = FText::Format(Mission->GetText(TDHandsOn::VerbKey(Spec.Verb)), Done + 1, Spots);
				O.bHasTarget = true;
				O.Target = Next + FVector(0.f, 0.f, 190.f);
				O.Distance = Dist;
				O.Step = Dist > 400.f ? FText::Format(Mission->GetText(TEXT("Track_WalkTo")), Verb, Meters(Dist))
					: FText::Format(Mission->GetText(TEXT("Track_HoldHere")), Verb);
			}
			if (O.Kind == ETDObjectiveKind::OwnTask)
			{
				return O;
			}
		}
		bool bAnyPlanned = false;
		for (const FTDCrewPlan& Crew : Ops.Crew)
		{
			bAnyPlanned |= Crew.Tasks.Num() > 0;
		}
		if (!bAnyPlanned)
		{
			O.Kind = ETDObjectiveKind::PlanCrew;
			O.Title = Mission->GetText(TEXT("Track_Plan"));
			AtStation(O, Mission->GetText(TEXT("Track_PlanStep")));
			return O;
		}
	}
	O.Kind = ETDObjectiveKind::RunSols;
	O.Title = Mission->GetText(TEXT("Track_Run"));
	AtStation(O, FText::Format(Mission->GetText(TEXT("Track_RunStep")), SolStart, SolEnd));
	return O;
}
