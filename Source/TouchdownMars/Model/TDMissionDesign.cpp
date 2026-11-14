// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Model/TDMissionDesign.h"

#include "Engine/DataTable.h"

namespace
{
	const TCHAR* Ctx = TEXT("TDDesignRules");

	const FTDDesignOptionRow* FindOption(const UDataTable* Options, FName OptionId)
	{
		return Options ? Options->FindRow<FTDDesignOptionRow>(OptionId, Ctx, false) : nullptr;
	}

	void AddIssue(FTDDesignBudget& Budget, ETDDesignIssue Issue, ETDRole Role, bool bBlocking, double A = 0.0, double B = 0.0,
		ETDDesignSlot Slot = ETDDesignSlot::Launcher, double C = 0.0)
	{
		FTDDesignIssueLine& Line = Budget.Issues.AddDefaulted_GetRef();
		Line.Issue = Issue;
		Line.Role = Role;
		Line.bBlocking = bBlocking;
		Line.ValueA = A;
		Line.ValueB = B;
		Line.ValueC = C;
		Line.Slot = Slot;
	}

	ETDRole SlotRole(ETDDesignSlot Slot)
	{
		switch (Slot)
		{
		case ETDDesignSlot::Launcher: return ETDRole::Planner;
		case ETDDesignSlot::Instrument: return ETDRole::Science;
		case ETDDesignSlot::Comms: return ETDRole::Comms;
		default: return ETDRole::Engineer;
		}
	}

	int32 Percent(double Value)
	{
		return FMath::Clamp(FMath::RoundToInt(Value * 100.0), 0, 100);
	}
}

double UTDDesignRules::Constant(const UDataTable* Constants, FName Key)
{
	if (!Constants || Key.IsNone())
	{
		return 0.0;
	}
	const FTDConstantRow* Row = Constants->FindRow<FTDConstantRow>(Key, Ctx, false);
	return Row ? Row->Value : 0.0;
}

bool UTDDesignRules::IsMultiPick(ETDDesignSlot Slot)
{
	return Slot == ETDDesignSlot::Power || Slot == ETDDesignSlot::Instrument;
}

FTDMissionDesign UTDDesignRules::MakeDefaultDesign(const UDataTable* Options, FName ObjectiveId)
{
	FTDMissionDesign Design;
	Design.ObjectiveId = ObjectiveId;
	Design.SignedOff.Init(false, TD::NumRoles);
	if (!Options)
	{
		return Design;
	}
	Options->ForeachRow<FTDDesignOptionRow>(Ctx, [&Design](const FName& Key, const FTDDesignOptionRow& Row)
	{
		if (Row.bDefault)
		{
			Design.Picks.Add({Key, 1});
		}
	});
	return Design;
}

int32 UTDDesignRules::GetPickCount(const FTDMissionDesign& Design, FName OptionId)
{
	const FTDDesignPick* Pick = Design.Picks.FindByPredicate([OptionId](const FTDDesignPick& P) { return P.OptionId == OptionId; });
	return Pick ? Pick->Count : 0;
}

bool UTDDesignRules::SetPick(FTDMissionDesign& Design, const UDataTable* Options, FName OptionId, int32 Count)
{
	const FTDDesignOptionRow* Row = FindOption(Options, OptionId);
	if (!Row)
	{
		return false;
	}
	Count = FMath::Clamp(Count, 0, FMath::Max(1, Row->MaxCount));
	if (GetPickCount(Design, OptionId) == Count)
	{
		return false;
	}
	if (!IsMultiPick(Row->Slot) && Count > 0)
	{
		// One option per single-pick slot: drop the others in the same slot.
		Design.Picks.RemoveAll([Options, Row](const FTDDesignPick& P)
		{
			const FTDDesignOptionRow* Other = FindOption(Options, P.OptionId);
			return Other && Other->Slot == Row->Slot;
		});
	}
	Design.Picks.RemoveAll([OptionId](const FTDDesignPick& P) { return P.OptionId == OptionId; });
	if (Count > 0)
	{
		Design.Picks.Add({OptionId, Count});
	}
	Design.SignedOff.Init(false, TD::NumRoles);
	return true;
}

FTDDesignBudget UTDDesignRules::Evaluate(const FTDMissionDesign& Design, const UDataTable* Options, const UDataTable* Objectives,
	const UDataTable* Constants)
{
	FTDDesignBudget Budget;
	double UnplacedKgForReview = 0.0;
	const FTDObjectiveRow* Objective = Objectives ? Objectives->FindRow<FTDObjectiveRow>(Design.ObjectiveId, Ctx, false) : nullptr;

	double SolarKw = 0.0;
	double OrbitKg = 0.0;
	TSet<ETDDesignSlot> Filled;
	struct FItem { FName Id; double Kg; bool bLanded; };
	TArray<FItem> Items;
	TArray<TPair<FName, double>> Launches;
	for (const FTDDesignPick& Pick : Design.Picks)
	{
		const FTDDesignOptionRow* Row = FindOption(Options, Pick.OptionId);
		if (!Row || Pick.Count <= 0)
		{
			continue;
		}
		Filled.Add(Row->Slot);
		const double N = Pick.Count;
		Budget.CostM += Constant(Constants, Row->CostKey) * N;
		if (Row->Slot == ETDDesignSlot::Launcher)
		{
			Budget.CapacityKg += Constant(Constants, Row->CapacityKey) * N;
			Budget.Launches += Pick.Count;
			Budget.bUnflown |= Row->bUnflown;
			for (int32 Index = 0; Index < Pick.Count; ++Index)
			{
				Launches.Add({Pick.OptionId, Constant(Constants, Row->CapacityKey)});
			}
			continue;
		}
		const double MassKg = Constant(Constants, Row->MassKey) * N;
		for (int32 Index = 0; Index < Pick.Count; ++Index)
		{
			Items.Add({Pick.OptionId, Constant(Constants, Row->MassKey), !Row->bNotLanded});
		}
		Budget.HardwareKg += MassKg;
		OrbitKg += Row->bNotLanded ? MassKg : 0.0;
		const double OutKw = Constant(Constants, Row->PowerOutKey) * N;
		Budget.PowerOutKw += OutKw;
		SolarKw += Row->bDustSensitive ? OutKw : 0.0;
		Budget.PowerDrawKw += Constant(Constants, Row->PowerDrawKey) * N;
		if (Row->Slot == ETDDesignSlot::Instrument)
		{
			Budget.Instruments += Pick.Count;
			Budget.DataMadeMb += Constant(Constants, Row->DataKey) * N;
			Budget.Science += Constant(Constants, Row->ScienceKey) * N;
		}
		else if (Row->Slot == ETDDesignSlot::Comms)
		{
			Budget.DataLinkMb += Constant(Constants, Row->DataKey) * N;
			Budget.bRelay |= Row->bRelay;
		}
		if (Row->Slot == ETDDesignSlot::Shelter)
		{
			Budget.StartBonusShelter += Row->StartLevelBonus;
			Budget.RadiationRelief += Row->RadiationRelief;
		}
		else if (Row->Slot == ETDDesignSlot::Greenhouse)
		{
			Budget.StartBonusGreenhouse += Row->StartLevelBonus;
		}
		Budget.RepairCharges += Row->RepairCharges * Pick.Count;
	}

	// Crew consumables for the stay: NASA BVAD per crew member per day x crew x sols.
	const double CrewDays = Constant(Constants, TEXT("CrewSize")) * Constant(Constants, TEXT("MissionSols"));
	const double PerDay = Constant(Constants, TEXT("O2PerPersonDay")) + Constant(Constants, TEXT("PotableWaterPerPersonDay"))
		+ Constant(Constants, TEXT("FoodPackagedPerPersonDay"));
	Budget.ConsumablesKg = CrewDays * PerDay;

	// Advanced model: every landed package carries its own cruise and landing hardware, a game assumption that
	// scales with the package payload (LandingHardwarePerPayloadKg). A relay orbiter stays in orbit: no landing hardware.
	Items.Add({TEXT("Consumables"), Budget.ConsumablesKg, true});
	const double HardwareRatio = Design.bRealisticModel ? Constant(Constants, TEXT("LandingHardwarePerPayloadKg")) : 0.0;
	auto Footprint = [HardwareRatio](const FItem& Item) { return Item.Kg * (Item.bLanded ? 1.0 + HardwareRatio : 1.0); };
	Items.Sort([&Footprint](const FItem& A, const FItem& B) { return Footprint(A) > Footprint(B); });
	Launches.Sort([](const TPair<FName, double>& A, const TPair<FName, double>& B) { return A.Value > B.Value; });
	for (const TPair<FName, double>& Launch : Launches)
	{
		FTDLaunchPackage& Package = Budget.Packages.AddDefaulted_GetRef();
		Package.LauncherId = Launch.Key;
		Package.CapacityKg = Launch.Value;
	}
	const double LargestLaunch = Launches.Num() > 0 ? Launches[0].Value : 0.0;
	double UnplacedKg = 0.0;
	for (const FItem& Item : Items)
	{
		const double Need = Footprint(Item);
		FTDLaunchPackage* Target = Budget.Packages.FindByPredicate([Need](const FTDLaunchPackage& P) { return P.UsedKg + Need <= P.CapacityKg + 0.01; });
		if (Target)
		{
			Target->UsedKg += Need;
			Target->LandingHardwareKg += Need - Item.Kg;
			Target->Items.Add(Item.Id);
			Budget.LandingSystemKg += Need - Item.Kg;
		}
		else
		{
			UnplacedKg += Need;
			Budget.LandingSystemKg += Need - Item.Kg;
			if (Need > LargestLaunch && LargestLaunch > 0.0)
			{
				FTDDesignIssueLine& Line = Budget.Issues.AddDefaulted_GetRef();
				Line.Issue = ETDDesignIssue::ItemTooBig;
				Line.Role = ETDRole::Planner;
				Line.bBlocking = true;
				Line.ValueA = Need / 1000.0;
				Line.ValueB = LargestLaunch / 1000.0;
				Line.OptionId = Item.Id;
			}
		}
	}
	Budget.MassKg = Budget.HardwareKg + Budget.ConsumablesKg + Budget.LandingSystemKg;

	// Data: instruments fill storage every sol; an open link sends up to its volume; nothing is sent in solar
	// conjunction; data is lost only when storage overflows.
	Budget.StorageMb = Constant(Constants, TEXT("DataStorageMb"));
	const int32 Sols = FMath::RoundToInt(Constant(Constants, TEXT("MissionSols")));
	const int32 BlackoutStart = FMath::RoundToInt(Constant(Constants, TEXT("ConjunctionSolStart")));
	const int32 BlackoutEnd = FMath::RoundToInt(Constant(Constants, TEXT("ConjunctionSolEnd")));
	double Buffer = 0.0;
	for (int32 Sol = 1; Sol <= Sols; ++Sol)
	{
		Buffer += Budget.DataMadeMb;
		Budget.DataMadeTotalMb += Budget.DataMadeMb;
		if (Sol < BlackoutStart || Sol > BlackoutEnd)
		{
			const double Sent = FMath::Min(Budget.DataLinkMb, Buffer);
			Buffer -= Sent;
			Budget.DataReturnedMb += Sent;
		}
		if (Buffer > Budget.StorageMb)
		{
			Budget.DataLostMb += Buffer - Budget.StorageMb;
			Buffer = Budget.StorageMb;
		}
	}
	Budget.DataStoredMb = Buffer;
	UnplacedKgForReview = UnplacedKg;

	// Power cells: the surface rules split whole cells; one cell is CellKw of steady power.
	const double CellKw = FMath::Max(0.1, Constant(Constants, TEXT("CellKw")));
	const double NetKw = Budget.PowerOutKw - Budget.PowerDrawKw;
	const double DustNetKw = NetKw - SolarKw * (1.0 - Constant(Constants, TEXT("DustStormSolarShare")));
	Budget.Cells = FMath::Max(0, FMath::RoundToInt(NetKw / CellKw));
	Budget.DustCells = FMath::Max(0, FMath::RoundToInt(DustNetKw / CellKw));

	Budget.BudgetM = Objective && !Objective->BudgetKey.IsNone() ? Constant(Constants, Objective->BudgetKey) : 0.0;
	const bool bSandbox = Objective && Objective->bSandbox;

	// Review list. Missing slots first, then hard limits, then role goals (warnings).
	for (int32 SlotIndex = 0; SlotIndex < TD::NumDesignSlots; ++SlotIndex)
	{
		const ETDDesignSlot Slot = static_cast<ETDDesignSlot>(SlotIndex);
		const bool bOptional = Slot == ETDDesignSlot::Spares || Slot == ETDDesignSlot::Instrument;
		if (!bOptional && !Filled.Contains(Slot))
		{
			AddIssue(Budget, ETDDesignIssue::MissingSlot, SlotRole(Slot), true, 0.0, 0.0, Slot);
		}
	}
	if (UnplacedKgForReview > 0.0 && !Budget.Issues.ContainsByPredicate([](const FTDDesignIssueLine& L) { return L.Issue == ETDDesignIssue::ItemTooBig; }))
	{
		AddIssue(Budget, ETDDesignIssue::OverMass, ETDRole::Planner, true, Budget.MassKg / 1000.0, Budget.CapacityKg / 1000.0, ETDDesignSlot::Launcher, UnplacedKgForReview / 1000.0);
	}
	if (!bSandbox && Budget.BudgetM > 0.0 && Budget.CostM > Budget.BudgetM)
	{
		AddIssue(Budget, ETDDesignIssue::OverBudget, ETDRole::Comms, true, Budget.CostM, Budget.BudgetM);
	}
	const int32 MinCells = FMath::RoundToInt(Constant(Constants, TEXT("MinCells")));
	if (Budget.Cells < MinCells)
	{
		AddIssue(Budget, ETDDesignIssue::PowerShort, ETDRole::Engineer, true, Budget.Cells, MinCells);
	}
	if (Objective && Budget.Instruments < Objective->MinInstruments)
	{
		AddIssue(Budget, ETDDesignIssue::TooFewInstruments, ETDRole::Science, true, Budget.Instruments, Objective->MinInstruments);
	}
	if (Budget.DataLostMb + Budget.DataStoredMb > 0.5)
	{
		AddIssue(Budget, ETDDesignIssue::DataShort, ETDRole::Comms, false, Budget.DataMadeMb, Budget.DataLinkMb, ETDDesignSlot::Comms,
			Budget.DataLostMb + Budget.DataStoredMb);
	}
	for (int32 RoleIndex = 0; RoleIndex < TD::NumRoles; ++RoleIndex)
	{
		if (!IsRoleGoalMet(static_cast<ETDRole>(RoleIndex), Budget, Objective, Constants))
		{
			AddIssue(Budget, ETDDesignIssue::RoleGoalMissed, static_cast<ETDRole>(RoleIndex), false);
		}
	}

	Budget.bLaunchable = !Budget.Issues.ContainsByPredicate([](const FTDDesignIssueLine& L) { return L.bBlocking; });

	// Concept maturity (JPL CML): 1 objective, 2 every slot filled, 3 fits the limits, 4 every role signed off.
	Budget.ConceptMaturity = 1;
	if (!Budget.Issues.ContainsByPredicate([](const FTDDesignIssueLine& L) { return L.Issue == ETDDesignIssue::MissingSlot; }))
	{
		Budget.ConceptMaturity = 2;
		if (Budget.bLaunchable)
		{
			Budget.ConceptMaturity = 3;
			bool bAllSigned = Design.SignedOff.Num() == TD::NumRoles;
			for (bool bSigned : Design.SignedOff)
			{
				bAllSigned &= bSigned;
			}
			Budget.ConceptMaturity = bAllSigned ? 4 : 3;
		}
	}
	return Budget;
}

bool UTDDesignRules::IsRoleGoalMet(ETDRole Role, const FTDDesignBudget& Budget, const FTDObjectiveRow* Objective, const UDataTable* Constants)
{
	switch (Role)
	{
	case ETDRole::Planner:
		// Keep a mass margin for growth (designer value; real projects carry one too).
		return Budget.CapacityKg > 0.0 && Budget.MassKg <= Budget.CapacityKg * (1.0 - Constant(Constants, TEXT("MassMarginGoal")));
	case ETDRole::Engineer:
		// Power through the dust storm without dropping below the minimum.
		return Budget.DustCells >= FMath::RoundToInt(Constant(Constants, TEXT("MinCells")));
	case ETDRole::Science:
		return Budget.Science >= Constant(Constants, TEXT("ScienceGoal")) && (!Objective || Budget.Instruments >= Objective->MinInstruments);
	case ETDRole::Comms:
		return (Budget.DataMadeTotalMb <= 0.0 || Budget.DataReturnedMb >= 0.95 * Budget.DataMadeTotalMb)
			&& (Budget.BudgetM <= 0.0 || Budget.CostM <= Budget.BudgetM);
	}
	return false;
}

FTDDesignEffects UTDDesignRules::MakeEffects(const FTDDesignBudget& Budget)
{
	FTDDesignEffects Effects;
	Effects.bValid = true;
	Effects.Cells = Budget.Cells;
	Effects.DustCells = Budget.DustCells;
	Effects.bRelay = Budget.bRelay;
	Effects.StartBonusShelter = Budget.StartBonusShelter;
	Effects.StartBonusGreenhouse = Budget.StartBonusGreenhouse;
	Effects.RadiationRelief = Budget.RadiationRelief;
	Effects.RepairCharges = Budget.RepairCharges;
	Effects.Instruments = Budget.Instruments;
	Effects.DataMadeMb = Budget.DataMadeMb;
	Effects.DataLinkMb = Budget.DataLinkMb;
	return Effects;
}

FTDMissionScore UTDDesignRules::Score(const FTDMissionDesign& Design, const FTDDesignBudget& Budget, const FTDObjectiveRow* Objective,
	const FTDMissionState& State, const FTDMissionReport& Report, int32 ScienceWalks, int32 GoodDecisions, int32 Decisions,
	const UDataTable* Constants)
{
	FTDMissionScore Score;
	Score.WhyKeys.Init(NAME_None, 4);
	const bool bLiftOff = State.Result == ETDMissionResult::Minimum || State.Result == ETDMissionResult::Full;

	// Success: the objective's acceptance criteria, each pass or fail.
	int32 Lowest = 99;
	for (int32 Level : State.Levels)
	{
		Lowest = FMath::Min(Lowest, Level);
	}
	auto Passed = [&](FName Key)
	{
		const FString K = Key.ToString();
		if (K == TEXT("Req_CrewHome")) return bLiftOff && Report.bCrew;
		if (K == TEXT("Req_BaseRunning")) return bLiftOff && Lowest >= 1;
		if (K == TEXT("Req_BaseLow")) return bLiftOff && Lowest >= 2;
		if (K == TEXT("Req_OneInstrument") || K == TEXT("Req_ThreeInstruments")) return Budget.Instruments >= (Objective ? Objective->MinInstruments : 1);
		if (K == TEXT("Req_TwoWalks")) return ScienceWalks >= (Objective ? Objective->MinScienceWalks : 0);
		return bLiftOff; // Req_Sandbox and unknown keys: lift-off
	};
	if (Objective)
	{
		for (const FName& Key : Objective->RequirementKeys)
		{
			Score.RequirementKeys.Add(Key);
			Score.RequirementMet.Add(Passed(Key));
		}
	}
	int32 Met = 0;
	for (bool bOk : Score.RequirementMet)
	{
		Met += bOk ? 1 : 0;
	}
	const int32 Total = FMath::Max(1, Score.RequirementMet.Num());
	const bool bAllMet = bLiftOff && Met == Score.RequirementMet.Num();
	Score.Success = bLiftOff ? Percent(double(Met) / Total) : 0;
	Score.WhyKeys[0] = !bLiftOff ? FName(TEXT("Why_Success_Lost")) : !Report.bCrew ? FName(TEXT("Why_Success_Exposure"))
		: bAllMet ? FName(TEXT("Why_Success_All")) : FName(TEXT("Why_Success_Base"));

	// Science: value of the instruments that flew, scaled by the share of their data the link could return,
	// plus the science walks done on the surface.
	const double ScienceGoal = FMath::Max(1.0, Constant(Constants, TEXT("ScienceGoal")));
	const double Returned = Budget.DataMadeTotalMb > 0.0 ? FMath::Min(1.0, Budget.DataReturnedMb / Budget.DataMadeTotalMb) : 0.0;
	const double WalkValue = Constant(Constants, TEXT("ScienceWalkValue")) * ScienceWalks;
	Score.Science = bLiftOff || ScienceWalks > 0 ? Percent((Budget.Science * Returned + WalkValue) / (ScienceGoal * 2.0)) : 0;
	Score.WhyKeys[1] = Budget.Instruments == 0 ? FName(TEXT("Why_Science_None")) : Returned < 1.0 ? FName(TEXT("Why_Science_Link"))
		: ScienceWalks == 0 ? FName(TEXT("Why_Science_NoWalk")) : FName(TEXT("Why_Science_Good"));

	// Efficiency: unused launch capacity and budget are waste; so are cells left over the demand.
	const double MassUse = Budget.CapacityKg > 0.0 ? Budget.MassKg / Budget.CapacityKg : 0.0;
	const double MoneyUse = Budget.BudgetM > 0.0 ? Budget.CostM / Budget.BudgetM : 1.0;
	const double MassScore = MassUse <= 1.0 ? 1.0 - FMath::Abs(0.85 - MassUse) : 0.0; // a 15 % margin is ideal
	const double MoneyScore = MoneyUse <= 1.0 ? 1.0 - 0.5 * MoneyUse : 0.0;
	// Efficiency only counts when the mission succeeded; otherwise an empty rocket would score best.
	Score.Efficiency = bAllMet ? Percent(0.6 * MassScore + 0.4 * MoneyScore) : 0;
	Score.WhyKeys[2] = MassUse < 0.6 ? FName(TEXT("Why_Eff_EmptyRocket")) : MoneyUse > 0.9 ? FName(TEXT("Why_Eff_Expensive"))
		: FName(TEXT("Why_Eff_Good"));

	// Decisions: event choices that followed crew-then-base-then-science and sheltering on time.
	Score.Decisions = Decisions > 0 ? Percent(double(GoodDecisions) / Decisions) : 100;
	Score.WhyKeys[3] = State.StormExposures > 0 ? FName(TEXT("Why_Dec_Exposure")) : Score.Decisions >= 75 ? FName(TEXT("Why_Dec_Good"))
		: FName(TEXT("Why_Dec_Mixed"));

	const int32 WS = Objective ? Objective->WeightSuccess : 40;
	const int32 WSc = Objective ? Objective->WeightScience : 20;
	const int32 WE = Objective ? Objective->WeightEfficiency : 20;
	const int32 WD = Objective ? Objective->WeightDecisions : 20;
	const int32 WSum = FMath::Max(1, WS + WSc + WE + WD);
	Score.Total = (Score.Success * WS + Score.Science * WSc + Score.Efficiency * WE + Score.Decisions * WD) / WSum;

	for (int32 RoleIndex = 0; RoleIndex < TD::NumRoles; ++RoleIndex)
	{
		Score.RoleGoalsMet += IsRoleGoalMet(static_cast<ETDRole>(RoleIndex), Budget, Objective, Constants) ? 1 : 0;
	}
	return Score;
}

FString UTDDesignRules::MakeDesignCode(const FTDMissionDesign& Design, const UDataTable* Options)
{
	TArray<FString> Parts;
	for (int32 SlotIndex = 0; SlotIndex < TD::NumDesignSlots; ++SlotIndex)
	{
		for (const FTDDesignPick& Pick : Design.Picks)
		{
			const FTDDesignOptionRow* Row = FindOption(Options, Pick.OptionId);
			if (Row && static_cast<int32>(Row->Slot) == SlotIndex)
			{
				Parts.Add(Pick.Count > 1 ? FString::Printf(TEXT("%s%d"), *Row->Code, Pick.Count) : Row->Code);
			}
		}
	}
	return FString::Join(Parts, TEXT("-"));
}

FTDMissionDesign UTDDesignRules::MakeRecommendedDesign(const UDataTable* Options, const UDataTable* Objectives, FName ObjectiveId, bool bRealisticModel)
{
	FTDMissionDesign Design;
	Design.ObjectiveId = ObjectiveId;
	Design.bRealisticModel = bRealisticModel;
	Design.SignedOff.Init(false, TD::NumRoles);
	const FTDObjectiveRow* Objective = Objectives ? Objectives->FindRow<FTDObjectiveRow>(ObjectiveId, Ctx, false) : nullptr;
	if (!Objective || Objective->RecommendedPicks.IsEmpty())
	{
		return MakeDefaultDesign(Options, ObjectiveId);
	}
	TArray<FString> Parts;
	Objective->RecommendedPicks.ParseIntoArray(Parts, TEXT(";"));
	for (const FString& Part : Parts)
	{
		FString Id, Count;
		if (!Part.Split(TEXT(":"), &Id, &Count))
		{
			Id = Part;
			Count = TEXT("1");
		}
		if (FindOption(Options, FName(*Id.TrimStartAndEnd())))
		{
			Design.Picks.Add({FName(*Id.TrimStartAndEnd()), FMath::Max(1, FCString::Atoi(*Count))});
		}
	}
	return Design;
}

FTDDesignFix UTDDesignRules::SuggestFix(const FTDDesignIssueLine& Issue, const FTDMissionDesign& Design, const UDataTable* Options, const UDataTable* Constants)
{
	FTDDesignFix Fix;
	auto Set = [&Fix](FName Id, int32 Count) { Fix.bValid = true; Fix.OptionId = Id; Fix.NewCount = Count; };
	switch (Issue.Issue)
	{
	case ETDDesignIssue::OverMass:
	case ETDDesignIssue::ItemTooBig:
	{
		// One more Falcon Heavy (it also replaces Falcon 9s, which are too small for a fission unit with its landing hardware).
		const int32 Heavy = GetPickCount(Design, TEXT("FalconHeavy"));
		const FTDDesignOptionRow* Row = FindOption(Options, TEXT("FalconHeavy"));
		if (Row && Heavy < Row->MaxCount)
		{
			Set(TEXT("FalconHeavy"), Heavy + 1);
		}
		break;
	}
	case ETDDesignIssue::PowerShort:
	{
		const int32 Units = GetPickCount(Design, TEXT("Kilopower"));
		const FTDDesignOptionRow* Row = FindOption(Options, TEXT("Kilopower"));
		if (Row && Units < Row->MaxCount)
		{
			Set(TEXT("Kilopower"), Units + 1);
		}
		break;
	}
	case ETDDesignIssue::TooFewInstruments:
	{
		// The unpicked instrument with the most science per kg.
		FName Best;
		double BestValue = -1.0;
		if (Options)
		{
			Options->ForeachRow<FTDDesignOptionRow>(Ctx, [&](const FName& Key, const FTDDesignOptionRow& Row)
			{
				if (Row.Slot == ETDDesignSlot::Instrument && GetPickCount(Design, Key) == 0)
				{
					const double Value = Constant(Constants, Row.ScienceKey) / FMath::Max(0.1, Constant(Constants, Row.MassKey));
					if (Value > BestValue)
					{
						BestValue = Value;
						Best = Key;
					}
				}
			});
		}
		if (!Best.IsNone())
		{
			Set(Best, 1);
		}
		break;
	}
	case ETDDesignIssue::DataShort:
		if (GetPickCount(Design, TEXT("RelayOrbiter")) == 0)
		{
			Set(TEXT("RelayOrbiter"), 1);
		}
		break;
	case ETDDesignIssue::MissingSlot:
		if (Options)
		{
			Options->ForeachRow<FTDDesignOptionRow>(Ctx, [&](const FName& Key, const FTDDesignOptionRow& Row)
			{
				if (!Fix.bValid && Row.Slot == Issue.Slot && Row.bDefault)
				{
					Set(Key, 1);
				}
			});
		}
		break;
	default:
		break;
	}
	return Fix;
}
