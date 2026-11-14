// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
// Automation tests TD.Design.* for the mission design layer (Act 1 and the debrief score). Run headless:
// UnrealEditor-Cmd.exe TouchdownMars.uproject -ExecCmds="Automation RunTests TD.Design;Quit" -unattended -nullrhi -log

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/DataTable.h"
#include "Model/TDMissionDesign.h"
#include "Model/TDMissionRules.h"

namespace TDDesignTest
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	const UDataTable* Load(const TCHAR* Name)
	{
		return LoadObject<UDataTable>(nullptr, *FString::Printf(TEXT("/Game/TouchdownMars/Data/%s.%s"), Name, Name));
	}

	struct FTables
	{
		const UDataTable* Options = Load(TEXT("DT_TD_MissionOptions"));
		const UDataTable* Objectives = Load(TEXT("DT_TD_Objectives"));
		const UDataTable* Constants = Load(TEXT("DT_TD_Constants"));
		bool Valid(FAutomationTestBase& Test) const
		{
			const bool bOk = Options && Objectives && Constants;
			if (!bOk)
			{
				Test.AddError(TEXT("Design DataTables missing; run Scripts/import_data.py"));
			}
			return bOk;
		}
	};

	bool HasIssue(const FTDDesignBudget& Budget, ETDDesignIssue Issue)
	{
		return Budget.Issues.ContainsByPredicate([Issue](const FTDDesignIssueLine& L) { return L.Issue == Issue; });
	}

	/** Two Falcon Heavy, three fission units and the defaults: the reference design of the tests. */
	FTDMissionDesign Reference(const FTables& T)
	{
		FTDMissionDesign Design = UTDDesignRules::MakeDefaultDesign(T.Options, TEXT("FirstLight"));
		UTDDesignRules::SetPick(Design, T.Options, TEXT("FalconHeavy"), 2);
		UTDDesignRules::SetPick(Design, T.Options, TEXT("Kilopower"), 3);
		return Design;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDesignDefaultBlocked, "TD.Design.DefaultDesignIsBlocked", TDDesignTest::Flags)
bool FTDDesignDefaultBlocked::RunTest(const FString&)
{
	const TDDesignTest::FTables T;
	if (!T.Valid(*this)) { return false; }
	const FTDMissionDesign Design = UTDDesignRules::MakeDefaultDesign(T.Options, TEXT("FirstLight"));
	const FTDDesignBudget Budget = UTDDesignRules::Evaluate(Design, T.Options, T.Objectives, T.Constants);
	// The default teaches the two core limits at once: not enough launch capacity, not enough power.
	TestTrue(TEXT("over mass"), TDDesignTest::HasIssue(Budget, ETDDesignIssue::OverMass));
	TestTrue(TEXT("power short"), TDDesignTest::HasIssue(Budget, ETDDesignIssue::PowerShort));
	TestFalse(TEXT("not launchable"), Budget.bLaunchable);
	TestEqual(TEXT("CML 2 (all slots filled)"), Budget.ConceptMaturity, 2);
	// Consumables: BVAD (0.895 + 3.217 + 2.39 kg) x 4 crew x 31 sols = 806.2 kg.
	TestTrue(TEXT("consumables from BVAD"), FMath::IsNearlyEqual(Budget.ConsumablesKg, 806.248, 0.5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDesignReferenceFlies, "TD.Design.ReferenceDesignLaunches", TDDesignTest::Flags)
bool FTDDesignReferenceFlies::RunTest(const FString&)
{
	const TDDesignTest::FTables T;
	if (!T.Valid(*this)) { return false; }
	FTDMissionDesign Design = TDDesignTest::Reference(T);
	FTDDesignBudget Budget = UTDDesignRules::Evaluate(Design, T.Options, T.Objectives, T.Constants);
	TestTrue(TEXT("launchable"), Budget.bLaunchable);
	TestEqual(TEXT("6 cells"), Budget.Cells, 6);
	TestEqual(TEXT("fission ignores dust"), Budget.DustCells, 6);
	TestTrue(TEXT("capacity 2 x 16.8 t"), FMath::IsNearlyEqual(Budget.CapacityKg, 33600.0, 1.0));
	TestEqual(TEXT("CML 3 before the poll"), Budget.ConceptMaturity, 3);
	Design.SignedOff.Init(true, TD::NumRoles);
	Budget = UTDDesignRules::Evaluate(Design, T.Options, T.Objectives, T.Constants);
	TestEqual(TEXT("CML 4 after four GO"), Budget.ConceptMaturity, 4);
	// Any change clears the poll.
	UTDDesignRules::SetPick(Design, T.Options, TEXT("SuperCam"), 1);
	TestFalse(TEXT("change clears GO"), Design.SignedOff[0]);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDesignSolarDust, "TD.Design.SolarLosesPowerInDust", TDDesignTest::Flags)
bool FTDDesignSolarDust::RunTest(const FString&)
{
	const TDDesignTest::FTables T;
	if (!T.Valid(*this)) { return false; }
	FTDMissionDesign Design = TDDesignTest::Reference(T);
	UTDDesignRules::SetPick(Design, T.Options, TEXT("Kilopower"), 1);
	UTDDesignRules::SetPick(Design, T.Options, TEXT("SolarField"), 1);
	const FTDDesignBudget Budget = UTDDesignRules::Evaluate(Design, T.Options, T.Objectives, T.Constants);
	TestEqual(TEXT("clear sky 6 cells"), Budget.Cells, 6);
	TestTrue(TEXT("dust storm far fewer cells"), Budget.DustCells < Budget.Cells - 2);
	TestFalse(TEXT("engineer goal open"), UTDDesignRules::IsRoleGoalMet(ETDRole::Engineer, Budget, nullptr, T.Constants));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDesignRelayData, "TD.Design.RelayCarriesTheData", TDDesignTest::Flags)
bool FTDDesignRelayData::RunTest(const FString&)
{
	const TDDesignTest::FTables T;
	if (!T.Valid(*this)) { return false; }
	FTDMissionDesign Design = TDDesignTest::Reference(T);
	FTDDesignBudget Direct = UTDDesignRules::Evaluate(Design, T.Options, T.Objectives, T.Constants);
	TestTrue(TEXT("direct link short of data"), TDDesignTest::HasIssue(Direct, ETDDesignIssue::DataShort));
	UTDDesignRules::SetPick(Design, T.Options, TEXT("RelayOrbiter"), 1);
	const FTDDesignBudget Relay = UTDDesignRules::Evaluate(Design, T.Options, T.Objectives, T.Constants);
	TestFalse(TEXT("relay carries the data"), TDDesignTest::HasIssue(Relay, ETDDesignIssue::DataShort));
	TestTrue(TEXT("relay is single pick: direct link removed"), UTDDesignRules::GetPickCount(Design, TEXT("DirectLink")) == 0);
	// The orbiter stays in orbit: its mass needs no landing system.
	const double OrbiterKg = UTDDesignRules::Constant(T.Constants, TEXT("RelayOrbiterMass"));
	TestTrue(TEXT("no landing system for the orbiter"), Relay.LandingSystemKg < Direct.LandingSystemKg);
	TestTrue(TEXT("orbiter counted once"), Relay.MassKg - Direct.MassKg < OrbiterKg);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDesignEffectsTest, "TD.Design.EffectsReachTheSurface", TDDesignTest::Flags)
bool FTDDesignEffectsTest::RunTest(const FString&)
{
	const TDDesignTest::FTables T;
	if (!T.Valid(*this)) { return false; }
	FString Error;
	FTDRulesConfig Config;
	if (!UTDMissionRules::MakeConfig(TDDesignTest::Load(TEXT("DT_TD_Phases")), TDDesignTest::Load(TEXT("DT_TD_AgeBands")), T.Constants,
		ETDAgeBand::Commander, Config, Error))
	{
		AddError(Error);
		return false;
	}
	FTDMissionDesign Design = TDDesignTest::Reference(T);
	UTDDesignRules::SetPick(Design, T.Options, TEXT("Kilopower"), 4);
	UTDDesignRules::SetPick(Design, T.Options, TEXT("ShelterWaterWall"), 1);
	UTDDesignRules::SetPick(Design, T.Options, TEXT("SparesKit"), 1);
	Config.Design = UTDDesignRules::MakeEffects(UTDDesignRules::Evaluate(Design, T.Options, T.Objectives, T.Constants));
	const FTDMissionState State = UTDMissionRules::NewMission(Config, ETDAgeBand::Commander, 1234);
	TestEqual(TEXT("cells from the design"), State.CellsTotal, 8);
	TestEqual(TEXT("water wall raises the shelter"), State.Levels[static_cast<int32>(ETDSystem::Shelter)], Config.StartLevels[static_cast<int32>(ETDSystem::Shelter)] + 1);
	TestEqual(TEXT("spares charge"), State.RepairChargesLeft, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDesignScoreGate, "TD.Design.EfficiencyNeedsSuccess", TDDesignTest::Flags)
bool FTDDesignScoreGate::RunTest(const FString&)
{
	const TDDesignTest::FTables T;
	if (!T.Valid(*this)) { return false; }
	const FTDMissionDesign Design = TDDesignTest::Reference(T);
	const FTDDesignBudget Budget = UTDDesignRules::Evaluate(Design, T.Options, T.Objectives, T.Constants);
	const FTDObjectiveRow* Objective = T.Objectives->FindRow<FTDObjectiveRow>(TEXT("FirstLight"), TEXT("Test"));
	FTDMissionState State;
	State.Levels = { 0, 2, 2 };
	State.Result = ETDMissionResult::LostAir;
	FTDMissionReport Report;
	Report.Result = State.Result;
	const FTDMissionScore Lost = UTDDesignRules::Score(Design, Budget, Objective, State, Report, 0, 1, 2, T.Constants);
	TestEqual(TEXT("no efficiency without success"), Lost.Efficiency, 0);
	State.Result = ETDMissionResult::Full;
	State.Levels = { 3, 3, 3 };
	Report.Result = State.Result;
	Report.bCrew = true;
	const FTDMissionScore Won = UTDDesignRules::Score(Design, Budget, Objective, State, Report, 1, 2, 2, T.Constants);
	TestTrue(TEXT("efficiency counts after success"), Won.Efficiency > 0);
	TestEqual(TEXT("success 100"), Won.Success, 100);
	TestEqual(TEXT("four why keys"), Won.WhyKeys.Num(), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDesignItemTooBig, "TD.Design.ModulesAreNotSplit", TDDesignTest::Flags)
bool FTDDesignItemTooBig::RunTest(const FString&)
{
	const TDDesignTest::FTables T;
	if (!T.Valid(*this)) { return false; }
	FTDMissionDesign Design = TDDesignTest::Reference(T);
	UTDDesignRules::SetPick(Design, T.Options, TEXT("Falcon9"), 6); // replaces Falcon Heavy: 6 x 4.02 t
	const FTDDesignBudget Budget = UTDDesignRules::Evaluate(Design, T.Options, T.Objectives, T.Constants);
	// 24.1 t of capacity, but a fission unit with its landing hardware (about 5.5 t) fits in no single Falcon 9.
	TestTrue(TEXT("item too big for one launch"), TDDesignTest::HasIssue(Budget, ETDDesignIssue::ItemTooBig));
	TestFalse(TEXT("not launchable"), Budget.bLaunchable);
	TestEqual(TEXT("six packages"), Budget.Packages.Num(), 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDesignDataStorage, "TD.Design.DataWaitsInStorage", TDDesignTest::Flags)
bool FTDDesignDataStorage::RunTest(const FString&)
{
	const TDDesignTest::FTables T;
	if (!T.Valid(*this)) { return false; }
	FTDMissionDesign Design = TDDesignTest::Reference(T);
	UTDDesignRules::SetPick(Design, T.Options, TEXT("RelayOrbiter"), 1);
	const FTDDesignBudget Budget = UTDDesignRules::Evaluate(Design, T.Options, T.Objectives, T.Constants);
	// Conjunction stops every link for 7 sols; the data waits and goes home after it, so nothing is lost.
	TestTrue(TEXT("made over the stay"), FMath::IsNearlyEqual(Budget.DataMadeTotalMb, Budget.DataMadeMb * 31.0, 0.1));
	TestTrue(TEXT("all data home"), FMath::IsNearlyEqual(Budget.DataReturnedMb, Budget.DataMadeTotalMb, 0.1));
	TestEqual(TEXT("nothing lost"), Budget.DataLostMb, 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDesignWalkNeedsInstrument, "TD.Design.ScienceWalkNeedsInstrument", TDDesignTest::Flags)
bool FTDDesignWalkNeedsInstrument::RunTest(const FString&)
{
	const TDDesignTest::FTables T;
	if (!T.Valid(*this)) { return false; }
	FString Error;
	FTDRulesConfig Config;
	if (!UTDMissionRules::MakeConfig(TDDesignTest::Load(TEXT("DT_TD_Phases")), TDDesignTest::Load(TEXT("DT_TD_AgeBands")), T.Constants,
		ETDAgeBand::Commander, Config, Error))
	{
		AddError(Error);
		return false;
	}
	FTDMissionDesign Design = TDDesignTest::Reference(T);
	UTDDesignRules::SetPick(Design, T.Options, TEXT("MastcamZ"), 0);
	Config.Design = UTDDesignRules::MakeEffects(UTDDesignRules::Evaluate(Design, T.Options, T.Objectives, T.Constants));
	FTDMissionState State = UTDMissionRules::NewMission(Config, ETDAgeBand::Commander, 1234);
	State.PhaseIndex = 2;
	State.Allocation = { 2, 2, 2 };
	FTDEventRow Card;
	Card.Family = ETDEventFamily::Radiation;
	Card.ChoiceBExtra = ETDChoiceExtra::ScienceBonus;
	State.EventChoice = 1;
	UTDMissionRules::EndPhase(Config, State, &Card);
	TestEqual(TEXT("no instrument, no science walk"), State.ScienceWalks, 0);
	return true;
}

#endif

/** The first-flight script (docs/gdd.md "First flight") can always be finished: each beat's red line appears and has a fix. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDesignFirstFlightPath, "TD.Design.FirstFlightIsSolvable", TDDesignTest::Flags)
bool FTDDesignFirstFlightPath::RunTest(const FString&)
{
	const TDDesignTest::FTables T;
	if (!T.Valid(*this)) { return false; }
	const int32 MinCells = FMath::RoundToInt(UTDDesignRules::Constant(T.Constants, TEXT("MinCells")));
	auto Eval = [&T](const FTDMissionDesign& D) { return UTDDesignRules::Evaluate(D, T.Options, T.Objectives, T.Constants); };

	// Start: one solar field, no rocket, no instrument (UTDMissionSubsystem::ApplyFirstFlightStart).
	FTDMissionDesign Design = UTDDesignRules::MakeDefaultDesign(T.Options, TEXT("FirstLight"));
	Design.bRealisticModel = true;
	UTDDesignRules::SetPick(Design, T.Options, TEXT("FalconHeavy"), 0);
	UTDDesignRules::SetPick(Design, T.Options, TEXT("Kilopower"), 0);
	UTDDesignRules::SetPick(Design, T.Options, TEXT("MastcamZ"), 0);
	UTDDesignRules::SetPick(Design, T.Options, TEXT("SolarField"), 1);
	TestTrue(TEXT("start: needs an instrument"), TDDesignTest::HasIssue(Eval(Design), ETDDesignIssue::TooFewInstruments));

	// Beat 2-3: an instrument makes more data than the direct link sends.
	UTDDesignRules::SetPick(Design, T.Options, TEXT("MastcamZ"), 1);
	TestTrue(TEXT("data beat has something to show"), TDDesignTest::HasIssue(Eval(Design), ETDDesignIssue::DataShort));

	// Beat 4: one rocket is too small, a second one fixes it.
	UTDDesignRules::SetPick(Design, T.Options, TEXT("FalconHeavy"), 1);
	TestTrue(TEXT("one Falcon Heavy is too small"), TDDesignTest::HasIssue(Eval(Design), ETDDesignIssue::OverMass));
	UTDDesignRules::SetPick(Design, T.Options, TEXT("FalconHeavy"), 2);
	TestFalse(TEXT("two Falcon Heavy carry it"), TDDesignTest::HasIssue(Eval(Design), ETDDesignIssue::OverMass));

	// Beat 5: one solar field is short of cells, a second gives enough on a clear sol.
	TestTrue(TEXT("one solar field is short"), Eval(Design).Cells < MinCells);
	UTDDesignRules::SetPick(Design, T.Options, TEXT("SolarField"), 2);
	TestTrue(TEXT("two solar fields: enough in clear sky"), Eval(Design).Cells >= MinCells);

	// Beat 6: the storm preview fails on solar alone (the planned cheap failure) ...
	TestTrue(TEXT("solar alone fails the storm"), Eval(Design).DustCells < MinCells);
	// ... and two fission units pass it while the design still launches.
	UTDDesignRules::SetPick(Design, T.Options, TEXT("Kilopower"), 2);
	const FTDDesignBudget Final = Eval(Design);
	TestTrue(TEXT("fission holds the storm"), Final.DustCells >= MinCells);
	TestTrue(TEXT("final design launches"), Final.bLaunchable);
	return true;
}
