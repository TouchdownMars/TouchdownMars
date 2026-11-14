// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Misc/AutomationTest.h"
#include "Model/TDOperations.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace TDOpsTest
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	const UDataTable* Load(const TCHAR* Name)
	{
		return LoadObject<UDataTable>(nullptr, *FString::Printf(TEXT("/Game/TouchdownMars/Data/%s.%s"), Name, Name));
	}

	struct FTables
	{
		const UDataTable* Tasks = Load(TEXT("DT_TD_Tasks"));
		const UDataTable* Options = Load(TEXT("DT_TD_MissionOptions"));
		const UDataTable* Constants = Load(TEXT("DT_TD_Constants"));
		FTDMissionDesign Design;
		FTables()
		{
			Design = UTDDesignRules::MakeDefaultDesign(Options, TEXT("FirstLight"));
			UTDDesignRules::SetPick(Design, Options, TEXT("SolarField"), 1);
		}
		bool Valid(FAutomationTestBase& Test) const
		{
			const bool bOk = Tasks && Options && Constants;
			if (!bOk)
			{
				Test.AddError(TEXT("Ops DataTables missing; run Scripts/import_data.py"));
			}
			return bOk;
		}
	};

	constexpr int32 Arrival = 0;
	constexpr int32 FirstWalk = 1;
	constexpr int32 Exploration = 2;
}

/** NASA HEOMD-415: no spacewalks on sols 1-3; a spacewalk task cannot be planned in Arrival. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDOpsNoEvaOnArrival, "TD.Ops.NoSpacewalkOnArrival", TDOpsTest::Flags)
bool FTDOpsNoEvaOnArrival::RunTest(const FString&)
{
	const TDOpsTest::FTables T;
	if (!T.Valid(*this)) { return false; }
	FTDOpsState Ops = UTDOps::MakeState();
	Ops.Done.Add(TEXT("SuitCheck"), 1);
	TestEqual(TEXT("rover off-load blocked on arrival"),
		UTDOps::CanPlan(Ops, T.Tasks, TEXT("OffloadRover"), 1, T.Design, T.Options, TDOpsTest::Arrival, 3, false, T.Constants), ETDTaskBlock::WrongPhase);
	TestEqual(TEXT("rover off-load fine on sols 4-5"),
		UTDOps::CanPlan(Ops, T.Tasks, TEXT("OffloadRover"), 1, T.Design, T.Options, TDOpsTest::FirstWalk, 2, false, T.Constants), ETDTaskBlock::Ok);
	return true;
}

/** Never alone outside: planning a spacewalk puts a second crew member on it. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDOpsBuddy, "TD.Ops.SpacewalkTakesTwo", TDOpsTest::Flags)
bool FTDOpsBuddy::RunTest(const FString&)
{
	const TDOpsTest::FTables T;
	if (!T.Valid(*this)) { return false; }
	FTDOpsState Ops = UTDOps::MakeState();
	Ops.Done.Add(TEXT("SuitCheck"), 1);
	TestTrue(TEXT("planned"), UTDOps::Plan(Ops, T.Tasks, TEXT("OffloadPayload"), 2, T.Design, T.Options, TDOpsTest::FirstWalk, 2, false, T.Constants));
	TestEqual(TEXT("two crew on it"), UTDOps::CrewOn(Ops, TEXT("OffloadPayload")).Num(), 2);
	return true;
}

/** Prerequisites: no spacewalk before the suit check. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDOpsPrereq, "TD.Ops.SuitCheckFirst", TDOpsTest::Flags)
bool FTDOpsPrereq::RunTest(const FString&)
{
	const TDOpsTest::FTables T;
	if (!T.Valid(*this)) { return false; }
	const FTDOpsState Ops = UTDOps::MakeState();
	TestEqual(TEXT("needs the suit check"),
		UTDOps::CanPlan(Ops, T.Tasks, TEXT("OffloadRover"), 1, T.Design, T.Options, TDOpsTest::FirstWalk, 2, false, T.Constants), ETDTaskBlock::NeedsFirst);
	return true;
}

/** Rest-sol rule: in a 2-sol phase a crew member has one spacewalk sol; two 2.5 h spacewalks do not fit with 4 h. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDOpsCadence, "TD.Ops.RestSolBetweenSpacewalks", TDOpsTest::Flags)
bool FTDOpsCadence::RunTest(const FString&)
{
	const TDOpsTest::FTables T;
	if (!T.Valid(*this)) { return false; }
	TestEqual(TEXT("2 sols: 1 spacewalk sol of 4 h"), UTDOps::PhaseEvaHours(2, T.Constants), 4.f);
	TestEqual(TEXT("8.5 work hours per sol"), UTDOps::PhaseHours(2, T.Constants), 17.f);
	FTDOpsState Ops = UTDOps::MakeState();
	Ops.Done.Add(TEXT("SuitCheck"), 1);
	Ops.Done.Add(TEXT("OffloadRover"), 1);
	TestTrue(TEXT("first spacewalk"), UTDOps::Plan(Ops, T.Tasks, TEXT("SurveyClays"), 1, T.Design, T.Options, TDOpsTest::Exploration, 2, false, T.Constants));
	const TArray<int32> Team = UTDOps::CrewOn(Ops, TEXT("SurveyClays"));
	TestEqual(TEXT("the same crew member cannot add a 4 h spacewalk to a 2.5 h one"),
		UTDOps::CanPlan(Ops, T.Tasks, TEXT("BuildBerm"), Team[0], T.Design, T.Options, TDOpsTest::Exploration, 2, false, T.Constants), ETDTaskBlock::NoEvaHours);
	return true;
}

/** No spacewalks in a dust storm. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDOpsStorm, "TD.Ops.NoSpacewalkInDust", TDOpsTest::Flags)
bool FTDOpsStorm::RunTest(const FString&)
{
	const TDOpsTest::FTables T;
	if (!T.Valid(*this)) { return false; }
	FTDOpsState Ops = UTDOps::MakeState();
	Ops.Done.Add(TEXT("SuitCheck"), 1);
	Ops.Done.Add(TEXT("OffloadRover"), 1);
	TestEqual(TEXT("survey blocked in a storm"),
		UTDOps::CanPlan(Ops, T.Tasks, TEXT("SurveyClays"), 1, T.Design, T.Options, TDOpsTest::Exploration, 4, true, T.Constants), ETDTaskBlock::Storm);
	return true;
}

/** Effects reach the mission: water from clay raises AIR, the berm adds radiation relief, the survey opens extraction. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDOpsEffects, "TD.Ops.WorkChangesTheBase", TDOpsTest::Flags)
bool FTDOpsEffects::RunTest(const FString&)
{
	const TDOpsTest::FTables T;
	if (!T.Valid(*this)) { return false; }
	FTDOpsState Ops = UTDOps::MakeState();
	FTDMissionState State;
	State.Levels = { 2, 2, 2 };
	const FTDTaskRow* Survey = T.Tasks->FindRow<FTDTaskRow>(TEXT("SurveyClays"), TEXT("Test"));
	const FTDTaskRow* Water = T.Tasks->FindRow<FTDTaskRow>(TEXT("ExtractWater"), TEXT("Test"));
	const FTDTaskRow* Berm = T.Tasks->FindRow<FTDTaskRow>(TEXT("BuildBerm"), TEXT("Test"));
	if (!Survey || !Water || !Berm) { AddError(TEXT("task rows missing")); return false; }
	Ops.Done.Add(TEXT("SuitCheck"), 1);
	Ops.Done.Add(TEXT("OffloadRover"), 1);
	Ops.Done.Add(TEXT("SurveyClays"), 1);
	TestEqual(TEXT("no extraction before water-bearing clay is found"),
		UTDOps::CanPlan(Ops, T.Tasks, TEXT("ExtractWater"), 0, T.Design, T.Options, 3, 3, false, T.Constants), ETDTaskBlock::NeedsFirst);
	UTDOps::ApplyEffect(*Survey, Ops, State, T.Constants);
	TestTrue(TEXT("survey finds clay"), Ops.bWaterFound);
	UTDOps::ApplyEffect(*Water, Ops, State, T.Constants);
	TestEqual(TEXT("AIR +1"), State.Levels[static_cast<int32>(ETDSystem::Air)], 3);
	UTDOps::ApplyEffect(*Berm, Ops, State, T.Constants);
	TestEqual(TEXT("relief +1"), State.OpsRadiationRelief, 1);
	return true;
}

/** NASA HEOMD-415: after a 5-sol traverse the rover recharges about 2 sols; no rover work in the next phase. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDOpsRover, "TD.Ops.RoverRechargesAfterTraverse", TDOpsTest::Flags)
bool FTDOpsRover::RunTest(const FString&)
{
	const TDOpsTest::FTables T;
	if (!T.Valid(*this)) { return false; }
	const FTDTaskRow* Trip = T.Tasks->FindRow<FTDTaskRow>(TEXT("RoverExpedition"), TEXT("Test"));
	if (!Trip) { AddError(TEXT("RoverExpedition row missing")); return false; }
	FTDOpsState Ops = UTDOps::MakeState();
	FTDMissionState State;
	State.Levels = { 2, 2, 2 };
	State.PhaseIndex = 3;
	UTDOps::ApplyEffect(*Trip, Ops, State, T.Constants);
	TestEqual(TEXT("two science walks"), State.ScienceWalks, 2);
	TestEqual(TEXT("rover charging next phase"), Ops.RoverChargingPhase, 4);
	Ops.Done.Add(TEXT("SuitCheck"), 1);
	Ops.Done.Add(TEXT("OffloadRover"), 1);
	TestEqual(TEXT("no traverse while charging"),
		UTDOps::CanPlan(Ops, T.Tasks, TEXT("RoverExpedition"), 2, T.Design, T.Options, 4, 7, false, T.Constants), ETDTaskBlock::RoverCharging);
	return true;
}

/** The rest-sol rule can be broken, at a cost: a medical flag and fewer work hours next phase. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDOpsFatigue, "TD.Ops.BrokenRestRuleTires", TDOpsTest::Flags)
bool FTDOpsFatigue::RunTest(const FString&)
{
	const TDOpsTest::FTables T;
	if (!T.Valid(*this)) { return false; }
	FTDOpsState Ops = UTDOps::MakeState();
	Ops.Done.Add(TEXT("SuitCheck"), 1);
	Ops.Done.Add(TEXT("OffloadRover"), 1);
	UTDOps::Plan(Ops, T.Tasks, TEXT("SurveyClays"), 1, T.Design, T.Options, TDOpsTest::Exploration, 2, false, T.Constants);
	TestFalse(TEXT("no override: blocked"), UTDOps::Plan(Ops, T.Tasks, TEXT("BuildBerm"), 1, T.Design, T.Options, TDOpsTest::Exploration, 2, false, T.Constants));
	TestTrue(TEXT("override: planned"), UTDOps::Plan(Ops, T.Tasks, TEXT("BuildBerm"), 1, T.Design, T.Options, TDOpsTest::Exploration, 2, false, T.Constants, true));
	TestTrue(TEXT("medical flag"), Ops.RestBroken.Contains(1));
	UTDOps::Finish(Ops, T.Tasks);
	TestEqual(TEXT("tired next phase"), Ops.Fatigue[1], 1);
	TestEqual(TEXT("counted"), Ops.RestBreaks >= 1, true);
	TestTrue(TEXT("fewer hours"), UTDOps::CrewPhaseHours(Ops, 1, 2, T.Constants) < UTDOps::PhaseHours(2, T.Constants));
	return true;
}

#endif
