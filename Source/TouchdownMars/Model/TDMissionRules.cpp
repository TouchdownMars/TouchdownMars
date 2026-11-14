// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Model/TDMissionRules.h"

#include "Engine/DataTable.h"
#include "Math/RandomStream.h"
#include "TouchdownMars.h"

namespace
{
	constexpr int32 AirIndex = static_cast<int32>(ETDSystem::Air);
	constexpr int32 GreenhouseIndex = static_cast<int32>(ETDSystem::Greenhouse);
	constexpr int32 ShelterIndex = static_cast<int32>(ETDSystem::Shelter);
	constexpr int32 NeverArrives = 99;

	const TCHAR* SystemName(int32 SystemIndex)
	{
		switch (SystemIndex)
		{
		case AirIndex: return TEXT("Air");
		case GreenhouseIndex: return TEXT("Greenhouse");
		default: return TEXT("Shelter");
		}
	}

	bool ReadConstant(const UDataTable* Table, const TCHAR* Key, int32& OutValue, FString& OutError)
	{
		const FTDConstantRow* Row = Table->FindRow<FTDConstantRow>(FName(Key), TEXT("TDMissionRules"), false);
		if (!Row)
		{
			OutError = FString::Printf(TEXT("DT_TD_Constants has no row %s"), Key);
			return false;
		}
		OutValue = FMath::RoundToInt(Row->Value);
		return true;
	}

	/** Same seed + phase always gives the same stream, independent of earlier calls. */
	FRandomStream MakePhaseStream(int32 Seed, int32 PhaseIndex, int32 Salt)
	{
		return FRandomStream(Seed * 97 + PhaseIndex * 7919 + Salt);
	}

	struct FStamps
	{
		bool bCrew = false;
		bool bBase = false;
		bool bEmergency = false;
		bool bBonus = false;
	};

	FStamps ComputeStamps(const FTDRulesConfig& Config, const FTDMissionState& State, bool bLiftOff)
	{
		FStamps Stamps;
		bool bAllWorking = true;
		for (int32 Level : State.Levels)
		{
			bAllWorking &= Level >= 2;
		}
		Stamps.bCrew = bLiftOff && State.StormExposures == 0;
		Stamps.bBase = bLiftOff && bAllWorking;
		Stamps.bEmergency = bLiftOff && State.LowestEventLevel >= 2;
		Stamps.bBonus = State.bScienceDone || State.bHarvest;
		return Stamps;
	}
}

bool UTDMissionRules::MakeConfig(const UDataTable* PhasesTable, const UDataTable* AgeBandsTable, const UDataTable* ConstantsTable,
	ETDAgeBand Band, FTDRulesConfig& OutConfig, FString& OutError)
{
	if (!PhasesTable || !AgeBandsTable || !ConstantsTable)
	{
		OutError = TEXT("Missing DataTable (run Scripts/import_data.py)");
		return false;
	}

	OutConfig = FTDRulesConfig();
	OutConfig.Phases.SetNum(TD::NumPhases);
	TArray<bool> bFound;
	bFound.Init(false, TD::NumPhases);
	for (const TPair<FName, uint8*>& Pair : PhasesTable->GetRowMap())
	{
		const FTDPhaseRow* Row = reinterpret_cast<const FTDPhaseRow*>(Pair.Value);
		const int32 Index = static_cast<int32>(Row->Phase);
		OutConfig.Phases[Index] = *Row;
		bFound[Index] = true;
	}
	if (bFound.Contains(false))
	{
		OutError = TEXT("DT_TD_Phases must have one row per ETDPhase");
		return false;
	}

	bool bBandFound = false;
	for (const TPair<FName, uint8*>& Pair : AgeBandsTable->GetRowMap())
	{
		const FTDAgeBandRow* Row = reinterpret_cast<const FTDAgeBandRow*>(Pair.Value);
		if (Row->Band == Band)
		{
			OutConfig.BandRow = *Row;
			bBandFound = true;
		}
	}
	if (!bBandFound)
	{
		OutError = TEXT("DT_TD_AgeBands has no row for the band");
		return false;
	}

	OutConfig.StartLevels.SetNum(TD::NumSystems);
	return ReadConstant(ConstantsTable, TEXT("StartLevelAir"), OutConfig.StartLevels[AirIndex], OutError)
		&& ReadConstant(ConstantsTable, TEXT("StartLevelGreenhouse"), OutConfig.StartLevels[GreenhouseIndex], OutError)
		&& ReadConstant(ConstantsTable, TEXT("StartLevelShelter"), OutConfig.StartLevels[ShelterIndex], OutError)
		&& ReadConstant(ConstantsTable, TEXT("LevelMin"), OutConfig.LevelMin, OutError)
		&& ReadConstant(ConstantsTable, TEXT("LevelMax"), OutConfig.LevelMax, OutError)
		&& ReadConstant(ConstantsTable, TEXT("ShelterSafeLevel"), OutConfig.ShelterSafeLevel, OutError)
		&& ReadConstant(ConstantsTable, TEXT("HarvestMinLevel"), OutConfig.HarvestMinLevel, OutError);
}

TArray<FTDDeckEntry> UTDMissionRules::MakeDeck(const UDataTable* EventsTable)
{
	TArray<FTDDeckEntry> Deck;
	if (!EventsTable)
	{
		return Deck;
	}
	for (const TPair<FName, uint8*>& Pair : EventsTable->GetRowMap())
	{
		FTDDeckEntry& Entry = Deck.AddDefaulted_GetRef();
		Entry.EventId = Pair.Key;
		Entry.Row = *reinterpret_cast<const FTDEventRow*>(Pair.Value);
	}
	Deck.Sort([](const FTDDeckEntry& A, const FTDDeckEntry& B) { return A.EventId.LexicalLess(B.EventId); });
	return Deck;
}

FTDMissionState UTDMissionRules::NewMission(const FTDRulesConfig& Config, ETDAgeBand Band, int32 Seed)
{
	FTDMissionState State;
	State.Seed = FMath::Clamp(Seed, 1000, 9999);
	State.Band = Band;
	State.Levels = Config.StartLevels;
	State.CellsTotal = Config.BandRow.Cells;
	State.Allocation.Init(0, TD::NumSystems);
	State.LowestEventLevel = Config.LevelMax;
	const TArray<int32> Playable = GetPlayablePhases(Config, Band);
	State.PhaseIndex = Playable.Num() > 0 ? Playable[0] : 0;
	return State;
}

TArray<int32> UTDMissionRules::GetPlayablePhases(const FTDRulesConfig& Config, ETDAgeBand Band)
{
	TArray<int32> Result;
	const bool bMergeFirstTwo = Config.BandRow.Phases < TD::NumPhases;
	for (int32 Index = 0; Index < TD::NumPhases; ++Index)
	{
		if (bMergeFirstTwo && Index == static_cast<int32>(ETDPhase::FirstWalk))
		{
			continue;
		}
		Result.Add(Index);
	}
	return Result;
}

void UTDMissionRules::StartPhase(const FTDRulesConfig& Config, FTDMissionState& State, const TArray<FTDDeckEntry>& Deck)
{
	State.Levels[GreenhouseIndex] = FMath::Clamp(State.Levels[GreenhouseIndex] + State.PendingGreenhouseDelta, Config.LevelMin, Config.LevelMax);
	State.PendingGreenhouseDelta = 0;
	State.Allocation.Init(0, TD::NumSystems);
	State.EventChoice = -1;
	State.bCalledHomeThisPhase = false;

	if (State.PhaseIndex == static_cast<int32>(ETDPhase::Return))
	{
		State.bHarvest = State.Levels[GreenhouseIndex] >= Config.HarvestMinLevel;
	}

	State.CurrentEventId = DrawEvent(Config, State, Deck);
	if (!State.CurrentEventId.IsNone())
	{
		State.DrawnEventIds.Add(State.CurrentEventId);
	}
}

bool UTDMissionRules::IsSystemLocked(const FTDRulesConfig& Config, const FTDMissionState& State, ETDSystem System)
{
	return State.Band == ETDAgeBand::Cadet && Config.BandRow.SystemsDecided < TD::NumSystems
		&& Config.Phases[State.PhaseIndex].CadetLockedSystem == System;
}

int32 UTDMissionRules::GetCellsPlaced(const FTDMissionState& State)
{
	int32 Total = 0;
	for (int32 Cells : State.Allocation)
	{
		Total += Cells;
	}
	return Total;
}

bool UTDMissionRules::CanPlaceCell(const FTDRulesConfig& Config, const FTDMissionState& State, ETDSystem System)
{
	return State.Result == ETDMissionResult::InProgress && !IsSystemLocked(Config, State, System)
		&& GetCellsPlaced(State) < State.CellsTotal;
}

bool UTDMissionRules::CanEndPhase(const FTDRulesConfig& Config, const FTDMissionState& State)
{
	return State.Result == ETDMissionResult::InProgress && GetCellsPlaced(State) == State.CellsTotal
		&& (State.CurrentEventId.IsNone() || State.EventChoice >= 0);
}

TArray<int32> UTDMissionRules::GetDemand(const FTDRulesConfig& Config, const FTDMissionState& State, const FTDEventRow* Event)
{
	const FTDPhaseRow& Phase = Config.Phases[State.PhaseIndex];
	TArray<int32> Demand = { Phase.DemandAir, Phase.DemandGreenhouse, Phase.DemandShelter };
	if (Event && State.EventChoice >= 0)
	{
		const bool bA = State.EventChoice == 0;
		Demand[AirIndex] += bA ? Event->ChoiceADemandAir : Event->ChoiceBDemandAir;
		Demand[GreenhouseIndex] += bA ? Event->ChoiceADemandGreenhouse : Event->ChoiceBDemandGreenhouse;
		Demand[ShelterIndex] += bA ? Event->ChoiceADemandShelter : Event->ChoiceBDemandShelter;
	}
	for (int32 Index = 0; Index < TD::NumSystems; ++Index)
	{
		if (IsSystemLocked(Config, State, static_cast<ETDSystem>(Index)))
		{
			Demand[Index] = 0;
		}
	}
	return Demand;
}

int32 UTDMissionRules::GetLevelChange(int32 Cells, int32 Demand)
{
	const int32 Delta = Cells - Demand;
	if (Delta >= 1) return 1;
	if (Delta == 0) return 0;
	if (Delta == -1) return -1;
	return -2;
}

FTDPhaseOutcome UTDMissionRules::EndPhase(const FTDRulesConfig& Config, FTDMissionState& State, const FTDEventRow* Event)
{
	FTDPhaseOutcome Outcome;
	Outcome.LevelChanges.Init(0, TD::NumSystems);

	const TArray<int32> Demand = GetDemand(Config, State, Event);
	TArray<int32> Changes;
	Changes.Init(0, TD::NumSystems);
	for (int32 Index = 0; Index < TD::NumSystems; ++Index)
	{
		if (!IsSystemLocked(Config, State, static_cast<ETDSystem>(Index)))
		{
			Changes[Index] = GetLevelChange(State.Allocation[Index], Demand[Index]);
		}
	}

	// AIR and SHELTER change now; GREENHOUSE changes at the start of the next phase (plants grow slowly).
	for (int32 Index : { AirIndex, ShelterIndex })
	{
		const int32 Before = State.Levels[Index];
		State.Levels[Index] = FMath::Clamp(Before + Changes[Index], Config.LevelMin, Config.LevelMax);
		Outcome.LevelChanges[Index] = State.Levels[Index] - Before;
	}
	State.PendingGreenhouseDelta = Changes[GreenhouseIndex];
	Outcome.PendingGreenhouseDelta = Changes[GreenhouseIndex];

	bool bExposure = false;
	if (Event && State.EventChoice >= 0)
	{
		const ETDChoiceExtra Extra = State.EventChoice == 0 ? Event->ChoiceAExtra : Event->ChoiceBExtra;
		bExposure |= Extra == ETDChoiceExtra::StormExposure || Extra == ETDChoiceExtra::ScienceWithExposure;
		State.bScienceDone |= Extra == ETDChoiceExtra::ScienceBonus || Extra == ETDChoiceExtra::ScienceWithExposure;
		bExposure |= Event->Family == ETDEventFamily::Radiation && State.Levels[ShelterIndex] < Config.ShelterSafeLevel;

		const int32 ProjectedGreenhouse = FMath::Clamp(State.Levels[GreenhouseIndex] + State.PendingGreenhouseDelta, Config.LevelMin, Config.LevelMax);
		const int32 Lowest = FMath::Min3(State.Levels[AirIndex], ProjectedGreenhouse, State.Levels[ShelterIndex]);
		State.LowestEventLevel = FMath::Min(State.LowestEventLevel, Lowest);
	}
	if (bExposure)
	{
		++State.StormExposures;
		Outcome.bStormExposure = true;
	}

	const bool bMergedArrival = State.PhaseIndex == static_cast<int32>(ETDPhase::Arrival) && Config.BandRow.Phases < TD::NumPhases;
	State.SolsElapsed = bMergedArrival
		? Config.Phases[static_cast<int32>(ETDPhase::FirstWalk)].SolEnd
		: Config.Phases[State.PhaseIndex].SolEnd;

	if (State.Levels[AirIndex] <= Config.LevelMin)
	{
		State.Result = ETDMissionResult::LostAir;
		Outcome.bLost = true;
		Outcome.CauseKey = TEXT("Cause_Air_Out");
		return Outcome;
	}

	if (bExposure)
	{
		Outcome.CauseKey = TEXT("Cause_Storm_Exposure");
		return Outcome;
	}

	TArray<int32> Shown = Outcome.LevelChanges;
	Shown[GreenhouseIndex] = Outcome.PendingGreenhouseDelta;
	int32 Worst = INDEX_NONE;
	int32 Best = INDEX_NONE;
	for (int32 Index = 0; Index < TD::NumSystems; ++Index)
	{
		if (Shown[Index] < 0 && (Worst == INDEX_NONE || Shown[Index] < Shown[Worst])) Worst = Index;
		if (Shown[Index] > 0 && (Best == INDEX_NONE || Shown[Index] > Shown[Best])) Best = Index;
	}
	if (Worst != INDEX_NONE)
	{
		Outcome.CauseKey = FName(FString::Printf(TEXT("Cause_%s_Down"), SystemName(Worst)));
	}
	else if (Best != INDEX_NONE)
	{
		Outcome.CauseKey = FName(FString::Printf(TEXT("Cause_%s_Up"), SystemName(Best)));
	}
	else
	{
		Outcome.CauseKey = TEXT("Cause_Steady");
	}
	return Outcome;
}

bool UTDMissionRules::AdvancePhase(const FTDRulesConfig& Config, FTDMissionState& State, const TArray<FTDDeckEntry>& Deck)
{
	if (State.Result != ETDMissionResult::InProgress)
	{
		return false;
	}
	const TArray<int32> Playable = GetPlayablePhases(Config, State.Band);
	const int32 Position = Playable.IndexOfByKey(State.PhaseIndex);
	if (Position == INDEX_NONE || Position + 1 >= Playable.Num())
	{
		// Lift-off: apply the last greenhouse change so the report shows the base as the crew leaves it.
		State.Levels[GreenhouseIndex] = FMath::Clamp(State.Levels[GreenhouseIndex] + State.PendingGreenhouseDelta, Config.LevelMin, Config.LevelMax);
		State.PendingGreenhouseDelta = 0;
		const FStamps Stamps = ComputeStamps(Config, State, true);
		State.Result = (Stamps.bCrew && Stamps.bBase && Stamps.bEmergency && Stamps.bBonus) ? ETDMissionResult::Full : ETDMissionResult::Minimum;
		return false;
	}
	State.PhaseIndex = Playable[Position + 1];
	StartPhase(Config, State, Deck);
	return true;
}

FName UTDMissionRules::DrawEvent(const FTDRulesConfig& Config, const FTDMissionState& State, const TArray<FTDDeckEntry>& Deck)
{
	const ETDEventFamily Family = Config.Phases[State.PhaseIndex].EventFamily;
	if (Family == ETDEventFamily::None)
	{
		return NAME_None;
	}

	TArray<const FTDDeckEntry*> SameFamily;
	TArray<const FTDDeckEntry*> Other;
	for (const FTDDeckEntry& Entry : Deck)
	{
		const bool bBandOk = Entry.Row.Bands.Num() == 0 || Entry.Row.Bands.Contains(State.Band);
		const bool bPhaseOk = Entry.Row.AllowedPhasesMask == 0 || (Entry.Row.AllowedPhasesMask & (1 << State.PhaseIndex)) != 0;
		if (!bBandOk || !bPhaseOk || State.DrawnEventIds.Contains(Entry.EventId))
		{
			continue;
		}
		(Entry.Row.Family == Family ? SameFamily : Other).Add(&Entry);
	}
	const TArray<const FTDDeckEntry*>& Pool = SameFamily.Num() > 0 ? SameFamily : Other;
	if (Pool.Num() == 0)
	{
		return NAME_None;
	}
	FRandomStream Stream = MakePhaseStream(State.Seed, State.PhaseIndex, 13);
	return Pool[Stream.RandRange(0, Pool.Num() - 1)]->EventId;
}

bool UTDMissionRules::ChoiceHasCost(const FTDEventRow& Row, bool bChoiceA)
{
	const int32 Air = bChoiceA ? Row.ChoiceADemandAir : Row.ChoiceBDemandAir;
	const int32 Greenhouse = bChoiceA ? Row.ChoiceADemandGreenhouse : Row.ChoiceBDemandGreenhouse;
	const int32 Shelter = bChoiceA ? Row.ChoiceADemandShelter : Row.ChoiceBDemandShelter;
	const ETDChoiceExtra Extra = bChoiceA ? Row.ChoiceAExtra : Row.ChoiceBExtra;
	return Air > 0 || Greenhouse > 0 || Shelter > 0
		|| Extra == ETDChoiceExtra::StormExposure || Extra == ETDChoiceExtra::ScienceWithExposure;
}

bool UTDMissionRules::CallHome(const FTDRulesConfig& Config, FTDMissionState& State)
{
	if (State.Result != ETDMissionResult::InProgress || State.bCalledHomeThisPhase || Config.Phases[State.PhaseIndex].bIsBlackout)
	{
		return false;
	}
	const TArray<int32> Playable = GetPlayablePhases(Config, State.Band);
	const int32 Position = Playable.IndexOfByKey(State.PhaseIndex);
	const int32 ArrivePosition = Position + FMath::Max(0, Config.BandRow.AdviceDelayPhases);

	FTDPendingMessage& Message = State.PendingMessages.AddDefaulted_GetRef();
	Message.SentPhase = State.PhaseIndex;
	Message.ArrivePhase = Playable.IsValidIndex(ArrivePosition) ? Playable[ArrivePosition] : NeverArrives;
	Message.TextKey = FName(FString::Printf(TEXT("Capcom_Reply_%s"), *StaticEnum<ETDPhase>()->GetNameStringByValue(State.PhaseIndex)));
	State.bCalledHomeThisPhase = true;
	return true;
}

TArray<FTDPendingMessage> UTDMissionRules::TakeDueMessages(const FTDRulesConfig& Config, FTDMissionState& State)
{
	TArray<FTDPendingMessage> Due;
	if (Config.Phases[State.PhaseIndex].bIsBlackout)
	{
		return Due;
	}
	for (int32 Index = State.PendingMessages.Num() - 1; Index >= 0; --Index)
	{
		if (State.PendingMessages[Index].ArrivePhase <= State.PhaseIndex)
		{
			Due.Insert(State.PendingMessages[Index], 0);
			State.PendingMessages.RemoveAt(Index);
		}
	}
	return Due;
}

FTDMissionReport UTDMissionRules::MakeReport(const FTDRulesConfig& Config, const FTDMissionState& State)
{
	FTDMissionReport Report;
	Report.Result = State.Result;
	const bool bLiftOff = State.Result == ETDMissionResult::Minimum || State.Result == ETDMissionResult::Full;
	const FStamps Stamps = ComputeStamps(Config, State, bLiftOff);
	Report.bCrew = Stamps.bCrew;
	Report.bBase = Stamps.bBase;
	Report.bEmergency = Stamps.bEmergency;
	Report.bBonus = Stamps.bBonus;
	Report.bMessageStillOnItsWay = State.PendingMessages.Num() > 0;
	Report.LostInPhase = State.Result == ETDMissionResult::LostAir ? State.PhaseIndex : -1;
	return Report;
}

FTDBalanceReport UTDMissionRules::Simulate(const FTDRulesConfig& Config, const TArray<FTDDeckEntry>& Deck, ETDAgeBand Band,
	ETDAllocationPolicy Policy, int32 FirstSeed, int32 NumSeeds)
{
	FTDBalanceReport Report;
	Report.LossesByPhase.Init(0, TD::NumPhases);

	for (int32 Seed = FirstSeed; Seed < FirstSeed + NumSeeds; ++Seed)
	{
		FTDMissionState State = NewMission(Config, Band, Seed);
		StartPhase(Config, State, Deck);
		bool bAnyEvent = false;

		while (State.Result == ETDMissionResult::InProgress)
		{
			FRandomStream Stream = MakePhaseStream(Seed, State.PhaseIndex, 131);
			const FTDEventRow* Event = FindEvent(Deck, State.CurrentEventId);
			if (Event)
			{
				bAnyEvent = true;
				if (Policy == ETDAllocationPolicy::Random)
				{
					State.EventChoice = Stream.RandRange(0, 1);
				}
				else
				{
					const int32 CostA = Event->ChoiceADemandAir + Event->ChoiceADemandGreenhouse + Event->ChoiceADemandShelter;
					const int32 CostB = Event->ChoiceBDemandAir + Event->ChoiceBDemandGreenhouse + Event->ChoiceBDemandShelter;
					State.EventChoice = CostB < CostA ? 1 : 0;
				}
			}

			TArray<int32> Open;
			for (int32 Index = 0; Index < TD::NumSystems; ++Index)
			{
				if (!IsSystemLocked(Config, State, static_cast<ETDSystem>(Index)))
				{
					Open.Add(Index);
				}
			}
			int32 CellsLeft = State.CellsTotal;
			if (Policy == ETDAllocationPolicy::Random)
			{
				for (; CellsLeft > 0; --CellsLeft)
				{
					++State.Allocation[Open[Stream.RandRange(0, Open.Num() - 1)]];
				}
			}
			else
			{
				const TArray<int32> Demand = GetDemand(Config, State, Event);
				Open.Sort([&State](int32 A, int32 B) { return State.Levels[A] < State.Levels[B]; });
				for (int32 Index : Open)
				{
					const int32 Give = FMath::Min(Demand[Index], CellsLeft);
					State.Allocation[Index] += Give;
					CellsLeft -= Give;
				}
				for (int32 Turn = 0; CellsLeft > 0; ++Turn, --CellsLeft)
				{
					++State.Allocation[Open[Turn % Open.Num()]];
				}
			}

			const int32 Phase = State.PhaseIndex;
			const FTDPhaseOutcome Outcome = EndPhase(Config, State, Event);
			if (Outcome.bLost)
			{
				++Report.Losses;
				++Report.LossesByPhase[Phase];
				break;
			}
			AdvancePhase(Config, State, Deck);
		}

		++Report.Missions;
		Report.MissionsWithEvents += bAnyEvent ? 1 : 0;
		Report.FullMissions += State.Result == ETDMissionResult::Full ? 1 : 0;
		Report.MinimumMissions += State.Result == ETDMissionResult::Minimum ? 1 : 0;
	}
	return Report;
}

FTDBalanceReport UTDMissionRules::SimulateFromTables(const UDataTable* PhasesTable, const UDataTable* AgeBandsTable,
	const UDataTable* ConstantsTable, const UDataTable* EventsTable, ETDAgeBand Band, ETDAllocationPolicy Policy,
	int32 FirstSeed, int32 NumSeeds)
{
	FTDRulesConfig Config;
	FString Error;
	if (!MakeConfig(PhasesTable, AgeBandsTable, ConstantsTable, Band, Config, Error))
	{
		UE_LOG(LogTouchdownMars, Error, TEXT("SimulateFromTables: %s"), *Error);
		return FTDBalanceReport();
	}
	return Simulate(Config, MakeDeck(EventsTable), Band, Policy, FirstSeed, NumSeeds);
}

const FTDEventRow* UTDMissionRules::FindEvent(const TArray<FTDDeckEntry>& Deck, FName EventId)
{
	if (EventId.IsNone())
	{
		return nullptr;
	}
	const FTDDeckEntry* Entry = Deck.FindByPredicate([EventId](const FTDDeckEntry& E) { return E.EventId == EventId; });
	return Entry ? &Entry->Row : nullptr;
}
