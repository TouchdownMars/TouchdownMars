// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
// Automation tests TD.Model.*. Run headless:
// UnrealEditor-Cmd.exe TouchdownMars.uproject -ExecCmds="Automation RunTests TD.Model;Quit" -unattended -nullrhi -log

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/DataTable.h"
#include "Model/TDMissionRules.h"

namespace TDTest
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	const UDataTable* Load(const TCHAR* Name)
	{
		return LoadObject<UDataTable>(nullptr, *FString::Printf(TEXT("/Game/TouchdownMars/Data/%s.%s"), Name, Name));
	}

	/** Config from the real DataTables (the CSV is the source of truth). */
	bool MakeConfig(FAutomationTestBase& Test, ETDAgeBand Band, FTDRulesConfig& OutConfig)
	{
		FString Error;
		const bool bOk = UTDMissionRules::MakeConfig(Load(TEXT("DT_TD_Phases")), Load(TEXT("DT_TD_AgeBands")), Load(TEXT("DT_TD_Constants")), Band, OutConfig, Error);
		if (!bOk)
		{
			Test.AddError(FString::Printf(TEXT("Config: %s"), *Error));
		}
		return bOk;
	}

	FTDDeckEntry MakeCard(const TCHAR* Id, ETDEventFamily Family, int32 AShelter, ETDChoiceExtra AExtra, int32 BAir, ETDChoiceExtra BExtra)
	{
		FTDDeckEntry Entry;
		Entry.EventId = Id;
		Entry.Row.Family = Family;
		Entry.Row.ChoiceADemandShelter = AShelter;
		Entry.Row.ChoiceAExtra = AExtra;
		Entry.Row.ChoiceBDemandAir = BAir;
		Entry.Row.ChoiceBExtra = BExtra;
		return Entry;
	}

	/** Two cards per event family, all with a cost on both choices. */
	TArray<FTDDeckEntry> FixtureDeck()
	{
		TArray<FTDDeckEntry> Deck;
		const ETDEventFamily Families[] = { ETDEventFamily::Power, ETDEventFamily::Radiation, ETDEventFamily::Comms, ETDEventFamily::Equipment };
		int32 Number = 0;
		for (ETDEventFamily Family : Families)
		{
			for (int32 Copy = 0; Copy < 2; ++Copy)
			{
				const FString Id = FString::Printf(TEXT("Fixture%d"), Number++);
				Deck.Add(MakeCard(*Id, Family, 1, ETDChoiceExtra::None, 0, ETDChoiceExtra::StormExposure));
			}
		}
		return Deck;
	}

	/** Places all cells: each unlocked system gets its demand first, extra cells go round-robin. */
	void PlaceDemand(const FTDRulesConfig& Config, FTDMissionState& State, const FTDEventRow* Event)
	{
		State.Allocation.Init(0, TD::NumSystems);
		const TArray<int32> Demand = UTDMissionRules::GetDemand(Config, State, Event);
		int32 Left = State.CellsTotal;
		TArray<int32> Open;
		for (int32 Index = 0; Index < TD::NumSystems; ++Index)
		{
			if (!UTDMissionRules::IsSystemLocked(Config, State, static_cast<ETDSystem>(Index)))
			{
				Open.Add(Index);
			}
		}
		for (int32 Index : Open)
		{
			const int32 Give = FMath::Min(Demand[Index], Left);
			State.Allocation[Index] = Give;
			Left -= Give;
		}
		for (int32 Turn = 0; Left > 0; ++Turn, --Left)
		{
			++State.Allocation[Open[Turn % Open.Num()]];
		}
	}

	/** Plays a whole mission with PlaceDemand and choice A; returns the final state. */
	FTDMissionState PlayMission(const FTDRulesConfig& Config, const TArray<FTDDeckEntry>& Deck, ETDAgeBand Band, int32 Seed)
	{
		FTDMissionState State = UTDMissionRules::NewMission(Config, Band, Seed);
		UTDMissionRules::StartPhase(Config, State, Deck);
		while (State.Result == ETDMissionResult::InProgress)
		{
			const FTDEventRow* Event = UTDMissionRules::FindEvent(Deck, State.CurrentEventId);
			if (Event)
			{
				State.EventChoice = 0;
			}
			PlaceDemand(Config, State, Event);
			UTDMissionRules::EndPhase(Config, State, Event);
			UTDMissionRules::AdvancePhase(Config, State, Deck);
		}
		return State;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDModelAllocationTableTest, "TD.Model.Allocation.Table", TDTest::Flags)
bool FTDModelAllocationTableTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("cells = demand + 2 → +1"), UTDMissionRules::GetLevelChange(4, 2), 1);
	TestEqual(TEXT("cells = demand + 1 → +1"), UTDMissionRules::GetLevelChange(3, 2), 1);
	TestEqual(TEXT("cells = demand → 0"), UTDMissionRules::GetLevelChange(2, 2), 0);
	TestEqual(TEXT("cells = demand - 1 → -1"), UTDMissionRules::GetLevelChange(1, 2), -1);
	TestEqual(TEXT("cells = demand - 2 → -2"), UTDMissionRules::GetLevelChange(0, 2), -2);
	TestEqual(TEXT("cells = demand - 4 → -2"), UTDMissionRules::GetLevelChange(0, 4), -2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDModelAllocationClampTest, "TD.Model.Allocation.Clamp", TDTest::Flags)
bool FTDModelAllocationClampTest::RunTest(const FString& Parameters)
{
	FTDRulesConfig Config;
	if (!TDTest::MakeConfig(*this, ETDAgeBand::Commander, Config)) return false;

	// Everything into SHELTER for the whole game: SHELTER must stop at LevelMax, never go above.
	FTDMissionState State = UTDMissionRules::NewMission(Config, ETDAgeBand::Commander, 1234);
	TArray<FTDDeckEntry> NoDeck;
	UTDMissionRules::StartPhase(Config, State, NoDeck);
	for (int32 Phase = 0; Phase < 3 && State.Result == ETDMissionResult::InProgress; ++Phase)
	{
		State.Allocation = { 2, 0, State.CellsTotal - 2 };
		UTDMissionRules::EndPhase(Config, State, nullptr);
		TestTrue(TEXT("SHELTER <= LevelMax"), State.Levels[2] <= Config.LevelMax);
		UTDMissionRules::AdvancePhase(Config, State, NoDeck);
		TestTrue(TEXT("GREENHOUSE >= LevelMin"), State.Levels[1] >= Config.LevelMin);
	}
	TestEqual(TEXT("SHELTER clamped at max"), State.Levels[2], Config.LevelMax);
	TestEqual(TEXT("GREENHOUSE clamped at min"), State.Levels[1], Config.LevelMin);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDModelPhaseOneCannotBeLostTest, "TD.Model.PhaseOneCannotBeLost", TDTest::Flags)
bool FTDModelPhaseOneCannotBeLostTest::RunTest(const FString& Parameters)
{
	for (ETDAgeBand Band : { ETDAgeBand::Cadet, ETDAgeBand::Commander })
	{
		FTDRulesConfig Config;
		if (!TDTest::MakeConfig(*this, Band, Config)) return false;
		const int32 Cells = Config.BandRow.Cells;
		int32 Checked = 0;
		for (int32 Air = 0; Air <= Cells; ++Air)
		{
			for (int32 Greenhouse = 0; Air + Greenhouse <= Cells; ++Greenhouse)
			{
				FTDMissionState State = UTDMissionRules::NewMission(Config, Band, 4321);
				TArray<FTDDeckEntry> NoDeck;
				UTDMissionRules::StartPhase(Config, State, NoDeck);
				State.Allocation = { Air, Greenhouse, Cells - Air - Greenhouse };
				const FTDPhaseOutcome Outcome = UTDMissionRules::EndPhase(Config, State, nullptr);
				TestFalse(FString::Printf(TEXT("%s phase 1 allocation %d/%d/%d lost"), Band == ETDAgeBand::Cadet ? TEXT("Cadet") : TEXT("Commander"), Air, Greenhouse, Cells - Air - Greenhouse), Outcome.bLost);
				++Checked;
			}
		}
		TestTrue(TEXT("allocations checked"), Checked > 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDModelSameSeedTest, "TD.Model.SameSeedSameGame", TDTest::Flags)
bool FTDModelSameSeedTest::RunTest(const FString& Parameters)
{
	FTDRulesConfig Config;
	if (!TDTest::MakeConfig(*this, ETDAgeBand::Commander, Config)) return false;
	const TArray<FTDDeckEntry> Deck = TDTest::FixtureDeck();
	for (int32 Seed : { 1000, 2468, 9999 })
	{
		const FTDMissionState A = TDTest::PlayMission(Config, Deck, ETDAgeBand::Commander, Seed);
		const FTDMissionState B = TDTest::PlayMission(Config, Deck, ETDAgeBand::Commander, Seed);
		TestEqual(TEXT("same events"), A.DrawnEventIds, B.DrawnEventIds);
		TestEqual(TEXT("same levels"), A.Levels, B.Levels);
		TestEqual(TEXT("same result"), static_cast<int32>(A.Result), static_cast<int32>(B.Result));
		TestEqual(TEXT("same exposures"), A.StormExposures, B.StormExposures);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDModelDeckGuaranteeTest, "TD.Model.DeckGuarantees", TDTest::Flags)
bool FTDModelDeckGuaranteeTest::RunTest(const FString& Parameters)
{
	const TArray<FTDDeckEntry> Deck = TDTest::FixtureDeck();
	for (ETDAgeBand Band : { ETDAgeBand::Commander, ETDAgeBand::Cadet })
	{
		FTDRulesConfig Config;
		if (!TDTest::MakeConfig(*this, Band, Config)) return false;
		int32 Failures = 0;
		for (int32 Seed = 1000; Seed < 2000; ++Seed)
		{
			const FTDMissionState State = TDTest::PlayMission(Config, Deck, Band, Seed);
			TSet<ETDEventFamily> Families;
			for (const FName& Id : State.DrawnEventIds)
			{
				Families.Add(UTDMissionRules::FindEvent(Deck, Id)->Family);
			}
			TSet<FName> Unique(State.DrawnEventIds);
			const bool bPowerNeeded = Band == ETDAgeBand::Commander; // Cadet merges FirstWalk (the power phase) into Arrival.
			const bool bOk = Families.Contains(ETDEventFamily::Radiation) && Families.Contains(ETDEventFamily::Comms)
				&& (!bPowerNeeded || Families.Contains(ETDEventFamily::Power)) && Unique.Num() == State.DrawnEventIds.Num();
			Failures += bOk ? 0 : 1;
		}
		TestEqual(FString::Printf(TEXT("%s seeds without guaranteed families or with a repeated card"), Band == ETDAgeBand::Cadet ? TEXT("Cadet") : TEXT("Commander")), Failures, 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDModelNoFreeChoiceTest, "TD.Model.NoFreeChoice", TDTest::Flags)
bool FTDModelNoFreeChoiceTest::RunTest(const FString& Parameters)
{
	// The checker itself must catch a free choice.
	const FTDDeckEntry Free = TDTest::MakeCard(TEXT("Free"), ETDEventFamily::Power, 0, ETDChoiceExtra::ScienceBonus, 1, ETDChoiceExtra::None);
	TestFalse(TEXT("a science-only choice with no demand is free"), UTDMissionRules::ChoiceHasCost(Free.Row, true));

	// Every real card: both choices cost something (pillar P1).
	const TArray<FTDDeckEntry> Deck = UTDMissionRules::MakeDeck(TDTest::Load(TEXT("DT_TD_Events")));
	if (Deck.Num() == 0)
	{
		AddWarning(TEXT("DT_TD_Events has no rows yet (cards arrive in Phase 6)"));
	}
	for (const FTDDeckEntry& Entry : Deck)
	{
		TestTrue(FString::Printf(TEXT("%s choice A costs something"), *Entry.EventId.ToString()), UTDMissionRules::ChoiceHasCost(Entry.Row, true));
		TestTrue(FString::Printf(TEXT("%s choice B costs something"), *Entry.EventId.ToString()), UTDMissionRules::ChoiceHasCost(Entry.Row, false));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDModelAdviceStateTest, "TD.Model.AdviceRemembersTheStateAtSend", TDTest::Flags)
bool FTDModelAdviceStateTest::RunTest(const FString& Parameters)
{
	TArray<FTDDeckEntry> NoDeck;
	FTDRulesConfig Commander;
	if (!TDTest::MakeConfig(*this, ETDAgeBand::Commander, Commander)) return false;
	FTDMissionState State = UTDMissionRules::NewMission(Commander, ETDAgeBand::Commander, 2222);
	UTDMissionRules::StartPhase(Commander, State, NoDeck);
	// AIR lowest when the call leaves.
	State.Levels[static_cast<int32>(ETDSystem::Air)] = 1;
	State.Levels[static_cast<int32>(ETDSystem::Greenhouse)] = 3;
	State.Levels[static_cast<int32>(ETDSystem::Shelter)] = 3;
	TestTrue(TEXT("call home"), UTDMissionRules::CallHome(Commander, State));
	TestEqual(TEXT("weakest at send is AIR"), State.PendingMessages.Last().WeakestSystemAtSend, static_cast<int32>(ETDSystem::Air));
	TDTest::PlaceDemand(Commander, State, nullptr);
	UTDMissionRules::EndPhase(Commander, State, nullptr);
	UTDMissionRules::AdvancePhase(Commander, State, NoDeck);
	// The base changed meanwhile: SHELTER is now the weakest.
	State.Levels[static_cast<int32>(ETDSystem::Air)] = 4;
	State.Levels[static_cast<int32>(ETDSystem::Shelter)] = 1;
	const TArray<FTDPendingMessage> Due = UTDMissionRules::TakeDueMessages(Commander, State);
	TestEqual(TEXT("one reply next phase"), Due.Num(), 1);
	if (Due.Num() == 1)
	{
		TestEqual(TEXT("reply still names AIR"), Due[0].WeakestSystemAtSend, static_cast<int32>(ETDSystem::Air));
		TestNotEqual(TEXT("now another system is weakest (advice is stale)"), UTDMissionRules::WeakestSystem(State), Due[0].WeakestSystemAtSend);
	}
	// Ties: AIR before SHELTER before GREENHOUSE.
	State.Levels[static_cast<int32>(ETDSystem::Air)] = 2;
	State.Levels[static_cast<int32>(ETDSystem::Greenhouse)] = 2;
	State.Levels[static_cast<int32>(ETDSystem::Shelter)] = 2;
	TestEqual(TEXT("tie goes to AIR"), UTDMissionRules::WeakestSystem(State), static_cast<int32>(ETDSystem::Air));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDModelDelayTest, "TD.Model.MissionControlDelay", TDTest::Flags)
bool FTDModelDelayTest::RunTest(const FString& Parameters)
{
	TArray<FTDDeckEntry> NoDeck;

	// Commander on Mars: reply one phase later.
	FTDRulesConfig Commander;
	if (!TDTest::MakeConfig(*this, ETDAgeBand::Commander, Commander)) return false;
	FTDMissionState State = UTDMissionRules::NewMission(Commander, ETDAgeBand::Commander, 1111);
	UTDMissionRules::StartPhase(Commander, State, NoDeck);
	TestTrue(TEXT("call home in phase 1"), UTDMissionRules::CallHome(Commander, State));
	TestFalse(TEXT("only once per phase"), UTDMissionRules::CallHome(Commander, State));
	TestEqual(TEXT("not delivered in the same phase"), UTDMissionRules::TakeDueMessages(Commander, State).Num(), 0);
	TDTest::PlaceDemand(Commander, State, nullptr);
	UTDMissionRules::EndPhase(Commander, State, nullptr);
	UTDMissionRules::AdvancePhase(Commander, State, NoDeck);
	TestEqual(TEXT("delivered at the start of the next phase"), UTDMissionRules::TakeDueMessages(Commander, State).Num(), 1);

	// Blackout phase: no call, no delivery.
	const int32 HardWeek = static_cast<int32>(ETDPhase::HardWeek);
	while (State.PhaseIndex < HardWeek - 1)
	{
		TDTest::PlaceDemand(Commander, State, nullptr);
		UTDMissionRules::EndPhase(Commander, State, nullptr);
		UTDMissionRules::AdvancePhase(Commander, State, NoDeck);
	}
	TestTrue(TEXT("call home before the blackout"), UTDMissionRules::CallHome(Commander, State));
	TDTest::PlaceDemand(Commander, State, nullptr);
	UTDMissionRules::EndPhase(Commander, State, nullptr);
	UTDMissionRules::AdvancePhase(Commander, State, NoDeck);
	TestEqual(TEXT("in the blackout phase"), State.PhaseIndex, HardWeek);
	TestFalse(TEXT("no call during the blackout"), UTDMissionRules::CallHome(Commander, State));
	TestEqual(TEXT("nothing delivered during the blackout"), UTDMissionRules::TakeDueMessages(Commander, State).Num(), 0);
	TDTest::PlaceDemand(Commander, State, nullptr);
	UTDMissionRules::EndPhase(Commander, State, nullptr);
	UTDMissionRules::AdvancePhase(Commander, State, NoDeck);
	TestEqual(TEXT("held message delivered after the blackout"), UTDMissionRules::TakeDueMessages(Commander, State).Num(), 1);

	// Cadet: reply in the same phase.
	FTDRulesConfig Cadet;
	if (!TDTest::MakeConfig(*this, ETDAgeBand::Cadet, Cadet)) return false;
	FTDMissionState CadetState = UTDMissionRules::NewMission(Cadet, ETDAgeBand::Cadet, 1111);
	UTDMissionRules::StartPhase(Cadet, CadetState, NoDeck);
	TestTrue(TEXT("Cadet call home"), UTDMissionRules::CallHome(Cadet, CadetState));
	TestEqual(TEXT("Cadet reply in the same phase"), UTDMissionRules::TakeDueMessages(Cadet, CadetState).Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDModelGreenhouseDelayTest, "TD.Model.GreenhouseDelayed", TDTest::Flags)
bool FTDModelGreenhouseDelayTest::RunTest(const FString& Parameters)
{
	FTDRulesConfig Config;
	if (!TDTest::MakeConfig(*this, ETDAgeBand::Commander, Config)) return false;
	TArray<FTDDeckEntry> NoDeck;
	FTDMissionState State = UTDMissionRules::NewMission(Config, ETDAgeBand::Commander, 2222);
	UTDMissionRules::StartPhase(Config, State, NoDeck);
	const int32 Before = State.Levels[1];
	State.Allocation = { 1, State.CellsTotal - 2, 1 }; // phase 1 demand is 1: GREENHOUSE gets a surplus
	UTDMissionRules::EndPhase(Config, State, nullptr);
	TestEqual(TEXT("GREENHOUSE unchanged at the end of the phase"), State.Levels[1], Before);
	TestEqual(TEXT("GREENHOUSE change pending"), State.PendingGreenhouseDelta, 1);
	UTDMissionRules::AdvancePhase(Config, State, NoDeck);
	TestEqual(TEXT("GREENHOUSE changed at the start of the next phase"), State.Levels[1], Before + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDModelRadiationTest, "TD.Model.RadiationExposure", TDTest::Flags)
bool FTDModelRadiationTest::RunTest(const FString& Parameters)
{
	FTDRulesConfig Config;
	if (!TDTest::MakeConfig(*this, ETDAgeBand::Commander, Config)) return false;
	TArray<FTDDeckEntry> Deck;
	const int32 Exploration = static_cast<int32>(ETDPhase::Exploration);
	Deck.Add(TDTest::MakeCard(TEXT("Storm"), ETDEventFamily::Radiation, 1, ETDChoiceExtra::None, 0, ETDChoiceExtra::ScienceWithExposure));
	Deck[0].Row.AllowedPhasesMask = 1 << Exploration;

	auto PlayToStorm = [&](int32 Seed)
	{
		FTDMissionState State = UTDMissionRules::NewMission(Config, ETDAgeBand::Commander, Seed);
		UTDMissionRules::StartPhase(Config, State, Deck);
		while (State.PhaseIndex < Exploration)
		{
			TDTest::PlaceDemand(Config, State, nullptr);
			UTDMissionRules::EndPhase(Config, State, nullptr);
			UTDMissionRules::AdvancePhase(Config, State, Deck);
		}
		return State;
	};

	// Shelter: covered demand, no exposure.
	FTDMissionState Safe = PlayToStorm(3333);
	TestEqual(TEXT("storm card drawn in Exploration"), Safe.CurrentEventId, FName(TEXT("Storm")));
	Safe.EventChoice = 0;
	TDTest::PlaceDemand(Config, Safe, &Deck[0].Row);
	UTDMissionRules::EndPhase(Config, Safe, &Deck[0].Row);
	TestEqual(TEXT("shelter choice: no exposure"), Safe.StormExposures, 0);
	TestFalse(TEXT("shelter choice: no science"), Safe.bScienceDone);

	// Keep working: exposure and science bonus.
	FTDMissionState Work = PlayToStorm(3333);
	Work.EventChoice = 1;
	TDTest::PlaceDemand(Config, Work, &Deck[0].Row);
	UTDMissionRules::EndPhase(Config, Work, &Deck[0].Row);
	TestEqual(TEXT("keep working: one exposure"), Work.StormExposures, 1);
	TestTrue(TEXT("keep working: science done"), Work.bScienceDone);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDModelCadetTest, "TD.Model.CadetBand", TDTest::Flags)
bool FTDModelCadetTest::RunTest(const FString& Parameters)
{
	FTDRulesConfig Config;
	if (!TDTest::MakeConfig(*this, ETDAgeBand::Cadet, Config)) return false;
	TestEqual(TEXT("Cadet plays 5 phases"), UTDMissionRules::GetPlayablePhases(Config, ETDAgeBand::Cadet).Num(), 5);

	TArray<FTDDeckEntry> NoDeck;
	FTDMissionState State = UTDMissionRules::NewMission(Config, ETDAgeBand::Cadet, 5555);
	UTDMissionRules::StartPhase(Config, State, NoDeck);
	TestEqual(TEXT("Cadet has 4 cells"), State.CellsTotal, 4);
	const ETDSystem Locked = Config.Phases[State.PhaseIndex].CadetLockedSystem;
	TestTrue(TEXT("locked system"), UTDMissionRules::IsSystemLocked(Config, State, Locked));
	TestFalse(TEXT("no cell on the locked system"), UTDMissionRules::CanPlaceCell(Config, State, Locked));
	TestEqual(TEXT("locked system demands nothing"), UTDMissionRules::GetDemand(Config, State, nullptr)[static_cast<int32>(Locked)], 0);

	const int32 LockedLevel = State.Levels[static_cast<int32>(Locked)];
	TDTest::PlaceDemand(Config, State, nullptr);
	UTDMissionRules::EndPhase(Config, State, nullptr);
	TestEqual(TEXT("merged Arrival ends at sol 5"), State.SolsElapsed, 5);
	TestEqual(TEXT("locked system level unchanged"), State.Levels[static_cast<int32>(Locked)], LockedLevel);
	UTDMissionRules::AdvancePhase(Config, State, NoDeck);
	TestEqual(TEXT("Cadet skips FirstWalk"), State.PhaseIndex, static_cast<int32>(ETDPhase::Exploration));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDModelLoseAndReportTest, "TD.Model.LoseAndReport", TDTest::Flags)
bool FTDModelLoseAndReportTest::RunTest(const FString& Parameters)
{
	FTDRulesConfig Config;
	if (!TDTest::MakeConfig(*this, ETDAgeBand::Commander, Config)) return false;
	TArray<FTDDeckEntry> NoDeck;

	// Starve AIR: lost, with the phase recorded.
	FTDMissionState State = UTDMissionRules::NewMission(Config, ETDAgeBand::Commander, 6666);
	UTDMissionRules::StartPhase(Config, State, NoDeck);
	bool bLost = false;
	for (int32 Guard = 0; Guard < 6 && !bLost; ++Guard)
	{
		State.Allocation = { 0, State.CellsTotal / 2, State.CellsTotal - State.CellsTotal / 2 };
		bLost = UTDMissionRules::EndPhase(Config, State, nullptr).bLost;
		if (!bLost) UTDMissionRules::AdvancePhase(Config, State, NoDeck);
	}
	TestTrue(TEXT("starving AIR loses"), bLost);
	TestEqual(TEXT("result LostAir"), static_cast<int32>(State.Result), static_cast<int32>(ETDMissionResult::LostAir));
	TestTrue(TEXT("lost after phase 1"), UTDMissionRules::MakeReport(Config, State).LostInPhase > 0);

	// Covering demand every phase with no events: lift-off, crew and base stamped.
	const FTDMissionState Done = TDTest::PlayMission(Config, NoDeck, ETDAgeBand::Commander, 7777);
	const FTDMissionReport Report = UTDMissionRules::MakeReport(Config, Done);
	TestTrue(TEXT("lift-off"), Done.Result == ETDMissionResult::Minimum || Done.Result == ETDMissionResult::Full);
	TestTrue(TEXT("crew stamp"), Report.bCrew);
	TestTrue(TEXT("base stamp"), Report.bBase);
	TestEqual(TEXT("31 sols"), Done.SolsElapsed, 31);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
