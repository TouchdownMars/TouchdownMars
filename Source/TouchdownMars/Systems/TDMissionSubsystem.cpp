// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Systems/TDMissionSubsystem.h"

#include "Engine/DataTable.h"
#include "Actors/TDSceneActors.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Systems/TDWalkGrid.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Systems/TDDonkiClient.h"
#include "Systems/TDSaveGame.h"
#include "TouchdownMars.h"

namespace
{
	template <typename T>
	T* LoadTable(const TCHAR* Name)
	{
		return LoadObject<T>(nullptr, *FString::Printf(TEXT("/Game/TouchdownMars/Data/%s.%s"), Name, Name));
	}
}

bool UTDMissionSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE || World->WorldType == EWorldType::Editor);
}

void UTDMissionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Settings = Cast<UTDSaveGame>(UGameplayStatics::LoadGameFromSlot(UTDSaveGame::SlotName(), 0));
	if (!Settings)
	{
		Settings = Cast<UTDSaveGame>(UGameplayStatics::CreateSaveGameObject(UTDSaveGame::StaticClass()));
	}
	if (Settings->bHasLastReport)
	{
		PreviousReport = Settings->LastReport;
		bHasPreviousReport = true;
	}
}

bool UTDMissionSubsystem::LoadTables(FString& OutError)
{
	PhasesTable = LoadTable<UDataTable>(TEXT("DT_TD_Phases"));
	AgeBandsTable = LoadTable<UDataTable>(TEXT("DT_TD_AgeBands"));
	ConstantsTable = LoadTable<UDataTable>(TEXT("DT_TD_Constants"));
	EventsTable = LoadTable<UDataTable>(TEXT("DT_TD_Events"));
	TextTable = LoadTable<UDataTable>(TEXT("DT_TD_Text"));
	SourcesTable = LoadTable<UDataTable>(TEXT("DT_TD_Sources"));
	DonkiArchiveTable = LoadTable<UDataTable>(TEXT("DT_TD_DonkiArchive"));
	OptionsTable = LoadTable<UDataTable>(TEXT("DT_TD_MissionOptions"));
	ObjectivesTable = LoadTable<UDataTable>(TEXT("DT_TD_Objectives"));
	WeatherTable = LoadTable<UDataTable>(TEXT("DT_TD_MarsWeather"));
	RadTable = LoadTable<UDataTable>(TEXT("DT_TD_RadDose"));
	QuakeTable = LoadTable<UDataTable>(TEXT("DT_TD_Marsquakes"));
	bRadCached = false;
	RadQuietDays.Reset();
	RadEventDays.Reset();
	ApplyMedaHeating();
	TasksTable = LoadTable<UDataTable>(TEXT("DT_TD_Tasks"));
	SitesTable = LoadTable<UDataTable>(TEXT("DT_TD_Sites"));
	if (!PhasesTable || !AgeBandsTable || !ConstantsTable || !EventsTable || !TextTable || !SourcesTable || !DonkiArchiveTable)
	{
		OutError = TEXT("DT_TD_* tables missing; run Scripts/import_data.py");
		return false;
	}
	return true;
}

bool UTDMissionSubsystem::StartMission(ETDAgeBand Band, int32 Seed)
{
	FString Error;
	if (!LoadTables(Error) || !UTDMissionRules::MakeConfig(PhasesTable, AgeBandsTable, ConstantsTable, Band, Config, Error))
	{
		UE_LOG(LogTouchdownMars, Error, TEXT("StartMission failed: %s"), *Error);
		return false;
	}
	Deck = UTDMissionRules::MakeDeck(EventsTable);
	if (bHasFlownDesign)
	{
		Config.Design = UTDDesignRules::MakeEffects(FlownBudget);
	}
	if (Seed < 1000 || Seed > 9999)
	{
		Seed = FMath::RandRange(1000, 9999);
	}
	State = UTDMissionRules::NewMission(Config, Band, Seed);
	bHasMission = true;
	Ops = UTDOps::MakeState();
	OpsReport.Reset();
	bOpsRunning = false;
	RefreshOpsScene();
	CalledOut.Reset();
	bCalledFar = false;
	if (UWorld* World = GetWorld(); World && World->IsGameWorld())
	{
		World->GetTimerManager().SetTimer(EscortTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { TickEscort(); }), 1.f, true);
	}
	if (Settings)
	{
		Settings->LastSeed = Seed;
	}
	UTDMissionRules::StartPhase(Config, State, Deck);
	BeginCurrentPhase();
	return true;
}

bool UTDMissionSubsystem::StartNewMission()
{
	return StartMission(GetPreferredBand(), 0);
}

void UTDMissionSubsystem::BeginCurrentPhase()
{
	OnPhaseStarted.Broadcast(static_cast<ETDPhase>(State.PhaseIndex));
	TArray<ETDSystem> Changed = { ETDSystem::Greenhouse };
	OnLevelsChanged.Broadcast(State, Changed);
	DeliverDueMessages();
	if (!State.CurrentEventId.IsNone())
	{
		OnEventDrawn.Broadcast(State.CurrentEventId);
	}
	OnAllocationChanged.Broadcast(State);
}

void UTDMissionSubsystem::DeliverDueMessages()
{
	for (const FTDPendingMessage& Message : UTDMissionRules::TakeDueMessages(Config, State))
	{
		PlaySfx(TEXT("RadioIn"));
		LastCapcomKey = Message.TextKey;
		OnMessageArrived.Broadcast(GetText(Message.TextKey));
	}
}

bool UTDMissionSubsystem::PlaceCell(ETDSystem System)
{
	if (!bHasMission || !UTDMissionRules::CanPlaceCell(Config, State, System))
	{
		return false;
	}
	++State.Allocation[static_cast<int32>(System)];
	PlaySfx(TEXT("UI_Confirm"));
	OnAllocationChanged.Broadcast(State);
	return true;
}

bool UTDMissionSubsystem::RemoveCell(ETDSystem System)
{
	const int32 Index = static_cast<int32>(System);
	if (!IsMissionRunning() || State.Allocation[Index] <= 0)
	{
		return false;
	}
	--State.Allocation[Index];
	PlaySfx(TEXT("UI_Back"));
	OnAllocationChanged.Broadcast(State);
	return true;
}

bool UTDMissionSubsystem::CanEndPhase() const
{
	return bHasMission && UTDMissionRules::CanEndPhase(Config, State);
}

bool UTDMissionSubsystem::ChooseEventOption(bool bOptionA)
{
	if (!IsMissionRunning() || State.CurrentEventId.IsNone() || State.EventChoice >= 0)
	{
		return false;
	}
	State.EventChoice = bOptionA ? 0 : 1;
	PlaySfx(TEXT("CardFlip"));
	OnAllocationChanged.Broadcast(State);
	return true;
}

bool UTDMissionSubsystem::EndPhase()
{
	if (!CanEndPhase())
	{
		return false;
	}
	const bool bWasDustPhase = State.bDustPhase;
	const FName EventId = State.CurrentEventId;
	const FTDPhaseOutcome Outcome = UTDMissionRules::EndPhase(Config, State, UTDMissionRules::FindEvent(Deck, EventId));

	TArray<ETDSystem> Changed;
	for (int32 Index = 0; Index < TD::NumSystems; ++Index)
	{
		if (Outcome.LevelChanges[Index] != 0)
		{
			Changed.Add(static_cast<ETDSystem>(Index));
		}
	}
	OnLevelsChanged.Broadcast(State, Changed);
	int32 Net = Outcome.PendingGreenhouseDelta;
	for (int32 Change : Outcome.LevelChanges) { Net += Change; }
	PlaySfx(Outcome.bStormExposure || Outcome.bLost ? TEXT("Alarm") : (Net < 0 ? TEXT("SystemDown") : TEXT("SystemUp")));
	LastCauseKey = Outcome.CauseKey;
	const FText Cause = GetText(Outcome.CauseKey);
	OnPhaseResolved.Broadcast(Cause);
	if (!EventId.IsNone())
	{
		OnEventResolved.Broadcast(EventId, Cause);
	}

	AccumulateDose(EventId, Outcome.bStormExposure);
	if (Outcome.bLost)
	{
		HandleMissionEnded();
		return true;
	}
	if (UTDMissionRules::AdvancePhase(Config, State, Deck))
	{
		// Dust settles on the solar arrays after a storm: cells stay down until the crew cleans them.
		const FTDMissionDesign OpsDesign = bHasFlownDesign ? FlownDesign : UTDDesignRules::MakeDefaultDesign(OptionsTable, TEXT("FirstLight"));
		if (bWasDustPhase && UTDDesignRules::GetPickCount(OpsDesign, TEXT("SolarField")) > 0)
		{
			const double PerStorm = GetConstant(TEXT("SolarDustCellsAfterStorm"));
			Ops.SolarDustCells = FMath::Max(1, FMath::RoundToInt(PerStorm * (Ops.bWeatherStation ? 0.5 : 1.0)));
		}
		ApplySolarDust();
		BeginCurrentPhase();
	}
	else
	{
		TArray<ETDSystem> Greenhouse = { ETDSystem::Greenhouse };
		OnLevelsChanged.Broadcast(State, Greenhouse);
		HandleMissionEnded();
	}
	return true;
}

void UTDMissionSubsystem::HandleMissionEnded()
{
	const FTDMissionReport Report = GetReport();
	if (State.Result != ETDMissionResult::LostAir) { PlaySfx(TEXT("LiftOff")); }
	if (bHasFlownDesign)
	{
		// Keep the flight for the post-mission comparison (this session only; nothing leaves the machine).
		FTDFlightRecord& Flight = Flights.AddDefaulted_GetRef();
		Flight.DesignCode = GetDesignCode();
		Flight.ObjectiveId = FlownDesign.ObjectiveId;
		Flight.Seed = State.Seed;
		Flight.Score = GetScore();
		Flight.Budget = FlownBudget;
		Flight.Design = FlownDesign;
		Flight.ScienceWalks = State.ScienceWalks;
		Flight.GoodDecisions = State.GoodDecisions;
		Flight.Decisions = State.Decisions;
		UE_LOG(LogTouchdownMars, Display, TEXT("Design: flight %s scored %d (success %d, science %d, efficiency %d, decisions %d)"),
			*Flight.DesignCode, Flight.Score.Total, Flight.Score.Success, Flight.Score.Science, Flight.Score.Efficiency, Flight.Score.Decisions);
	}
	OnMissionEnded.Broadcast(State.Result);
	// Store after broadcasting so the report screen can still compare with the mission before this one.
	if (Settings)
	{
		Settings->LastReport = Report;
		Settings->bHasLastReport = true;
		SaveSettings();
	}
}

bool UTDMissionSubsystem::CallHome()
{
	if (!bHasMission || !UTDMissionRules::CallHome(Config, State))
	{
		return false;
	}
	PlaySfx(TEXT("UI_Confirm"));
	OnAllocationChanged.Broadcast(State);
	DeliverDueMessages();
	return true;
}

void UTDMissionSubsystem::PlaySfx(FName Token)
{
	if (!IsSoundOn() || !GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}
	TObjectPtr<USoundBase>& Sound = SoundCache.FindOrAdd(Token);
	if (!Sound)
	{
		const FString Name = TEXT("SFX_TD_") + Token.ToString();
		Sound = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/TouchdownMars/Audio/%s.%s"), *Name, *Name));
	}
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), Sound);
	}
}

void UTDMissionSubsystem::PlaySfxPitched(FName Token, float Pitch, float Volume)
{
	if (!IsSoundOn() || !GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}
	TObjectPtr<USoundBase>& Sound = SoundCache.FindOrAdd(Token);
	if (!Sound)
	{
		const FString Name = TEXT("SFX_TD_") + Token.ToString();
		Sound = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/TouchdownMars/Audio/%s.%s"), *Name, *Name));
	}
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), Sound, Volume, Pitch);
	}
}

void UTDMissionSubsystem::PushComms(const FText& Line)
{
	if (Line.IsEmpty() || (Comms.Num() > 0 && Comms.Last().EqualTo(Line)))
	{
		return;
	}
	Comms.Add(Line);
	if (Comms.Num() > 30)
	{
		Comms.RemoveAt(0);
	}
	LastCommsTime = FPlatformTime::Seconds();
}

bool UTDMissionSubsystem::CanCallHome() const
{
	return IsMissionRunning() && !State.bCalledHomeThisPhase && Config.Phases.IsValidIndex(State.PhaseIndex)
		&& !Config.Phases[State.PhaseIndex].bIsBlackout;
}

bool UTDMissionSubsystem::RestartSameSeed()
{
	if (!bHasMission)
	{
		return false;
	}
	if (State.Result != ETDMissionResult::InProgress)
	{
		PreviousReport = GetReport();
		bHasPreviousReport = true;
	}
	return StartMission(State.Band, State.Seed);
}

FTDMissionReport UTDMissionSubsystem::GetReport() const
{
	return bHasMission ? UTDMissionRules::MakeReport(Config, State) : FTDMissionReport();
}

bool UTDMissionSubsystem::GetPreviousReport(FTDMissionReport& OutReport) const
{
	OutReport = PreviousReport;
	return bHasPreviousReport;
}

FName UTDMissionSubsystem::GetPatchKey() const
{
	const FTDMissionReport Report = GetReport();
	if (State.bScienceDone && State.StormExposures > 0) return TEXT("Patch_StormChaser");
	if (State.bHarvest) return TEXT("Patch_GreenThumb");
	if (Report.bCrew && Report.bEmergency) return TEXT("Patch_CarefulPlanner");
	return TEXT("Patch_SteadyHands");
}

TArray<int32> UTDMissionSubsystem::GetPlayablePhases() const
{
	return bHasMission ? UTDMissionRules::GetPlayablePhases(Config, State.Band) : TArray<int32>();
}

void UTDMissionSubsystem::GetPhaseSols(int32 PhaseIndex, int32& OutSolStart, int32& OutSolEnd) const
{
	OutSolStart = 0;
	OutSolEnd = 0;
	if (!bHasMission || !Config.Phases.IsValidIndex(PhaseIndex))
	{
		return;
	}
	OutSolStart = Config.Phases[PhaseIndex].SolStart;
	OutSolEnd = Config.Phases[PhaseIndex].SolEnd;
	if (PhaseIndex == static_cast<int32>(ETDPhase::Arrival) && Config.BandRow.Phases < TD::NumPhases)
	{
		OutSolEnd = Config.Phases[static_cast<int32>(ETDPhase::FirstWalk)].SolEnd;
	}
}

const FTDPhaseRow* UTDMissionSubsystem::GetPhaseRow(int32 PhaseIndex) const
{
	return bHasMission && Config.Phases.IsValidIndex(PhaseIndex) ? &Config.Phases[PhaseIndex] : nullptr;
}

const FTDEventRow* UTDMissionSubsystem::GetCurrentEvent() const
{
	return UTDMissionRules::FindEvent(Deck, State.CurrentEventId);
}

const FTDEventRow* UTDMissionSubsystem::GetEvent(FName EventId) const
{
	return UTDMissionRules::FindEvent(Deck, EventId);
}

const FTDSourceRow* UTDMissionSubsystem::GetSource(FName SourceId) const
{
	return SourcesTable ? SourcesTable->FindRow<FTDSourceRow>(SourceId, TEXT("TDMissionSubsystem"), false) : nullptr;
}

TArray<int32> UTDMissionSubsystem::GetDemand() const
{
	return bHasMission ? UTDMissionRules::GetDemand(Config, State, UTDMissionRules::FindEvent(Deck, State.CurrentEventId)) : TArray<int32>();
}

bool UTDMissionSubsystem::IsSystemLocked(ETDSystem System) const
{
	return bHasMission && UTDMissionRules::IsSystemLocked(Config, State, System);
}

FText UTDMissionSubsystem::GetText(FName Key) const
{
	const UDataTable* Table = TextTable ? TextTable.Get() : LoadTable<UDataTable>(TEXT("DT_TD_Text"));
	if (Table)
	{
		if (const FTDTextRow* Row = Table->FindRow<FTDTextRow>(Key, TEXT("TDMissionSubsystem"), false))
		{
			return FText::FromString(GetLanguage() == ETDLanguage::TR ? Row->TR : Row->EN);
		}
	}
	return FText::FromName(Key);
}

FText UTDMissionSubsystem::GetBandText(FName Key) const
{
	if (bHasMission && State.Band == ETDAgeBand::Cadet)
	{
		const FName CadetKey(Key.ToString() + TEXT("_Cadet"));
		if (HasText(CadetKey))
		{
			return GetText(CadetKey);
		}
	}
	return GetText(Key);
}

bool UTDMissionSubsystem::HasText(FName Key) const
{
	return TextTable && TextTable->FindRow<FTDTextRow>(Key, TEXT("TDMissionSubsystem"), false) != nullptr;
}

void UTDMissionSubsystem::SaveSettings()
{
	if (Settings)
	{
		UGameplayStatics::SaveGameToSlot(Settings, UTDSaveGame::SlotName(), 0);
	}
	OnSettingsChanged.Broadcast();
}

void UTDMissionSubsystem::SetLanguage(ETDLanguage NewLanguage)
{
	Settings->Language = NewLanguage;
	SaveSettings();
	if (bHasMission)
	{
		OnAllocationChanged.Broadcast(State);
	}
}

ETDLanguage UTDMissionSubsystem::GetLanguage() const
{
	return Settings ? Settings->Language : ETDLanguage::EN;
}

void UTDMissionSubsystem::SetPreferredBand(ETDAgeBand NewBand)
{
	Settings->Band = NewBand;
	SaveSettings();
}

ETDAgeBand UTDMissionSubsystem::GetPreferredBand() const
{
	return Settings ? Settings->Band : ETDAgeBand::Commander;
}

void UTDMissionSubsystem::SetSoundOn(bool bOn)
{
	Settings->bSoundOn = bOn;
	SaveSettings();
}

bool UTDMissionSubsystem::IsSoundOn() const
{
	return !Settings || Settings->bSoundOn;
}

void UTDMissionSubsystem::SetReduceMotion(bool bOn)
{
	Settings->bReduceMotion = bOn;
	SaveSettings();
}

bool UTDMissionSubsystem::IsReduceMotion() const
{
	return Settings && Settings->bReduceMotion;
}

void UTDMissionSubsystem::SetOutcomeShotsEnabled(bool bOn)
{
	Settings->bOutcomeShotsEnabled = bOn;
	SaveSettings();
}

bool UTDMissionSubsystem::IsOutcomeShotsEnabled() const
{
	return !Settings || Settings->bOutcomeShotsEnabled;
}

void UTDMissionSubsystem::SetFullscreen(bool bOn)
{
	Settings->bFullscreen = bOn;
	if (GEngine && GEngine->GetGameUserSettings())
	{
		UGameUserSettings* User = GEngine->GetGameUserSettings();
		User->SetFullscreenMode(bOn ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
		User->ApplySettings(false);
	}
	SaveSettings();
}

bool UTDMissionSubsystem::IsFullscreen() const
{
	return Settings && Settings->bFullscreen;
}

int32 UTDMissionSubsystem::GetConstantInt(const TCHAR* Key, int32 Fallback) const
{
	if (ConstantsTable)
	{
		if (const FTDConstantRow* Row = ConstantsTable->FindRow<FTDConstantRow>(FName(Key), TEXT("TDMissionSubsystem"), false))
		{
			return FMath::RoundToInt(Row->Value);
		}
	}
	return Fallback;
}

void UTDMissionSubsystem::RequestSpaceWeather()
{
	if (bHasSpaceWeather)
	{
		OnSpaceWeatherReady.Broadcast(SpaceWeather);
		return;
	}
	if (bSpaceWeatherRequested)
	{
		return;
	}
	FString Error;
	if (!DonkiArchiveTable && !LoadTables(Error))
	{
		return;
	}
	bSpaceWeatherRequested = true;
	DonkiClient = NewObject<UTDDonkiClient>(this);
	TWeakObjectPtr<UTDMissionSubsystem> WeakThis(this);
	DonkiClient->Fetch(DonkiArchiveTable, static_cast<float>(GetConstantInt(TEXT("DonkiTimeoutSeconds"), 5)), GetConstantInt(TEXT("DonkiRangeDays"), 30),
		UTDDonkiClient::FOnDone::CreateLambda([WeakThis](const FTDSpaceWeather& Weather)
		{
			if (UTDMissionSubsystem* Self = WeakThis.Get())
			{
				Self->SpaceWeather = Weather;
				Self->bHasSpaceWeather = true;
				Self->OnSpaceWeatherReady.Broadcast(Weather);
			}
		}));
}

FString UTDMissionSubsystem::GetDebugString() const
{
	if (!bHasMission)
	{
		return TEXT("No mission");
	}
	const TArray<int32> Demand = GetDemand();
	return FString::Printf(
		TEXT("Seed %d  %s  Phase %s  Sol %d\nAIR %d (cells %d/%d)  GREENHOUSE %d%+d (cells %d/%d)  SHELTER %d (cells %d/%d)\nEvent %s choice %d  Exposures %d  Messages %d  Result %s"),
		State.Seed, *StaticEnum<ETDAgeBand>()->GetNameStringByValue(static_cast<int64>(State.Band)),
		*StaticEnum<ETDPhase>()->GetNameStringByValue(State.PhaseIndex), State.SolsElapsed,
		State.Levels[0], State.Allocation[0], Demand[0],
		State.Levels[1], State.PendingGreenhouseDelta, State.Allocation[1], Demand[1],
		State.Levels[2], State.Allocation[2], Demand[2],
		*State.CurrentEventId.ToString(), State.EventChoice, State.StormExposures, State.PendingMessages.Num(),
		*StaticEnum<ETDMissionResult>()->GetNameStringByValue(static_cast<int64>(State.Result)));
}

const FTDWeatherRow* UTDMissionSubsystem::GetWeather(int32 MissionSol) const
{
	return WeatherTable ? WeatherTable->FindRow<FTDWeatherRow>(FName(*FString::Printf(TEXT("Sol%d"), MissionSol)), TEXT("TDMissionSubsystem"), false) : nullptr;
}

// Mission design (Act 1)

void UTDMissionSubsystem::RefreshDesignScene(const FTDMissionDesign& ForDesign) const
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	static const FString Prefix(TEXT("DesignShow_"));
	int32 Shown = 0;
	int32 Hidden = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		bool bTagged = false;
		bool bShow = false;
		for (const FName& Tag : Actor->Tags)
		{
			FString Rest = Tag.ToString();
			if (!Rest.RemoveFromStart(Prefix))
			{
				continue;
			}
			bTagged = true;
			FString OptionId, Index;
			if (Rest.Split(TEXT("_"), &OptionId, &Index, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
			{
				bShow |= UTDDesignRules::GetPickCount(ForDesign, FName(*OptionId)) >= FCString::Atoi(*Index);
			}
		}
		if (bTagged)
		{
			// The surface shows the base the team designed: fission units, solar fields, relay antenna, shelter, spares.
			Actor->SetActorHiddenInGame(!bShow);
			Actor->SetActorEnableCollision(bShow);
			(bShow ? Shown : Hidden) += 1;
		}
	}
	UE_LOG(LogTouchdownMars, Display, TEXT("Design: base scene %d parts shown, %d hidden"), Shown, Hidden);
}

bool UTDMissionSubsystem::BeginDesign(FName ObjectiveId, bool bRedesign)
{
	FString Error;
	if (!LoadTables(Error) || !OptionsTable || !ObjectivesTable)
	{
		UE_LOG(LogTouchdownMars, Error, TEXT("BeginDesign failed: %s"), Error.IsEmpty() ? TEXT("design tables missing") : *Error);
		return false;
	}
	if (ObjectivesTable->FindRow<FTDObjectiveRow>(ObjectiveId, TEXT("TDMissionSubsystem"), false) == nullptr)
	{
		return false;
	}
	const bool bRealistic = Design.bRealisticModel;
	// A redesign starts from the last flown design, so the team changes what the report pointed at.
	if (bRedesign && bHasFlownDesign && FlownDesign.ObjectiveId == ObjectiveId)
	{
		Design = FlownDesign;
		Design.SignedOff.Init(false, TD::NumRoles);
	}
	else
	{
		Design = UTDDesignRules::MakeDefaultDesign(OptionsTable, ObjectiveId);
		Design.bRealisticModel = bRealistic;
	}
	DesignSeed = bRedesign && bHasMission ? State.Seed : FMath::RandRange(1000, 9999);
	bDesigning = true;
	RefreshDesignScene(Design);
	UE_LOG(LogTouchdownMars, Display, TEXT("Design: begin %s, seed %d"), *ObjectiveId.ToString(), DesignSeed);
	OnDesignChanged.Broadcast();
	return true;
}

void UTDMissionSubsystem::ApplyFirstFlightStart()
{
	if (!bDesigning)
	{
		return;
	}
	UTDDesignRules::SetPick(Design, OptionsTable, TEXT("FalconHeavy"), 0);
	UTDDesignRules::SetPick(Design, OptionsTable, TEXT("Kilopower"), 0);
	UTDDesignRules::SetPick(Design, OptionsTable, TEXT("MastcamZ"), 0);
	UTDDesignRules::SetPick(Design, OptionsTable, TEXT("SolarField"), 1);
	Design.bRealisticModel = true;
	Design.SignedOff.Init(false, TD::NumRoles);
	RefreshDesignScene(Design);
	OnDesignChanged.Broadcast();
}

void UTDMissionSubsystem::LogOnboarding(const FString& Event, const FString& Detail) const
{
	const double Seconds = FirstFlight.StartSeconds > 0.0 ? FPlatformTime::Seconds() - FirstFlight.StartSeconds : 0.0;
	const FString Line = FString::Printf(TEXT("%s,%.1f,%d,%s,%s\n"), *FDateTime::Now().ToIso8601(), Seconds, Flights.Num(), *Event, *Detail.Replace(TEXT(","), TEXT(";")));
	const FString Path = FPaths::ProjectSavedDir() / TEXT("Logs/TD_FirstFlight.csv");
	FFileHelper::SaveStringToFile(Line, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
	UE_LOG(LogTouchdownMars, Display, TEXT("FirstFlight: %s %s (%.1f s)"), *Event, *Detail, Seconds);
}

bool UTDMissionSubsystem::SetDesignPick(FName OptionId, int32 Count)
{
	if (!bDesigning || !UTDDesignRules::SetPick(Design, OptionsTable, OptionId, Count))
	{
		return false;
	}
	PlaySfx(Count > 0 ? TEXT("UI_Confirm") : TEXT("UI_Back"));
	RefreshDesignScene(Design);
	OnDesignChanged.Broadcast();
	return true;
}

void UTDMissionSubsystem::SetRealisticModel(bool bRealistic)
{
	if (Design.bRealisticModel != bRealistic)
	{
		Design.bRealisticModel = bRealistic;
		Design.SignedOff.Init(false, TD::NumRoles);
		OnDesignChanged.Broadcast();
	}
}

void UTDMissionSubsystem::SetRoleGo(ETDRole Role, bool bGo)
{
	const int32 Index = static_cast<int32>(Role);
	if (Design.SignedOff.Num() != TD::NumRoles)
	{
		Design.SignedOff.Init(false, TD::NumRoles);
	}
	// A role can only call GO on a design that fits the hard limits.
	Design.SignedOff[Index] = bGo && GetDesignBudget().bLaunchable;
	PlaySfx(Design.SignedOff[Index] ? TEXT("UI_Confirm") : TEXT("UI_Back"));
	OnDesignChanged.Broadcast();
}

FTDDesignBudget UTDMissionSubsystem::GetDesignBudget() const
{
	return UTDDesignRules::Evaluate(Design, OptionsTable, ObjectivesTable, ConstantsTable);
}

bool UTDMissionSubsystem::LaunchDesign()
{
	const FTDDesignBudget Budget = GetDesignBudget();
	if (!bDesigning || Budget.ConceptMaturity < 4)
	{
		return false;
	}
	FlownDesign = Design;
	FlownBudget = Budget;
	bHasFlownDesign = true;
	bDesigning = false;
	const ETDAgeBand Band = Design.bRealisticModel ? ETDAgeBand::Commander : ETDAgeBand::Cadet;
	UE_LOG(LogTouchdownMars, Display, TEXT("Design: launch %s, %.1f t of %.1f t, %d cells (%d in dust), %.0f of %.0f MUSD"),
		*GetDesignCode(), Budget.MassKg / 1000.0, Budget.CapacityKg / 1000.0, Budget.Cells, Budget.DustCells, Budget.CostM, Budget.BudgetM);
	OnDesignLaunched.Broadcast();
	return StartMission(Band, DesignSeed);
}

const FTDObjectiveRow* UTDMissionSubsystem::GetObjective() const
{
	const FName Id = bDesigning || !bHasFlownDesign ? Design.ObjectiveId : FlownDesign.ObjectiveId;
	return ObjectivesTable ? ObjectivesTable->FindRow<FTDObjectiveRow>(Id, TEXT("TDMissionSubsystem"), false) : nullptr;
}

double UTDMissionSubsystem::GetConstant(FName Key) const
{
	return UTDDesignRules::Constant(ConstantsTable, Key);
}

const FTDConstantRow* UTDMissionSubsystem::GetConstantRow(FName Key) const
{
	return ConstantsTable && !Key.IsNone() ? ConstantsTable->FindRow<FTDConstantRow>(Key, TEXT("TDMissionSubsystem"), false) : nullptr;
}

FTDMissionScore UTDMissionSubsystem::GetScore() const
{
	return UTDDesignRules::Score(FlownDesign, FlownBudget, GetObjective(), State, GetReport(), State.ScienceWalks,
		State.GoodDecisions, State.Decisions, ConstantsTable);
}

FString UTDMissionSubsystem::GetDesignCode() const
{
	return UTDDesignRules::MakeDesignCode(bDesigning ? Design : FlownDesign, OptionsTable);
}

bool UTDMissionSubsystem::IsRoleGoalMet(ETDRole Role) const
{
	return UTDDesignRules::IsRoleGoalMet(Role, GetDesignBudget(), GetObjective(), ConstantsTable);
}

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"
namespace
{
	/** td.AutoPlay: play the rest of the mission with the cheaper event choice and cover-the-lowest cell policy (tests only). */
	FAutoConsoleCommandWithWorld GTDAutoPlayCommand(TEXT("td.AutoPlay"), TEXT("Play the current mission to the end (development builds)"),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			UTDMissionSubsystem* Mission = World ? World->GetSubsystem<UTDMissionSubsystem>() : nullptr;
			for (int32 Guard = 0; Mission && Mission->IsMissionRunning() && Guard < 20; ++Guard)
			{
				const FTDMissionState& S = Mission->GetState();
				if (const FTDEventRow* Event = Mission->GetCurrentEvent(); Event && S.EventChoice < 0)
				{
					const int32 CostA = Event->ChoiceADemandAir + Event->ChoiceADemandGreenhouse + Event->ChoiceADemandShelter;
					const int32 CostB = Event->ChoiceBDemandAir + Event->ChoiceBDemandGreenhouse + Event->ChoiceBDemandShelter;
					Mission->ChooseEventOption(CostA <= CostB);
				}
				for (int32 Cell = 0; Cell < 20 && !Mission->CanEndPhase(); ++Cell)
				{
					const TArray<int32> Demand = Mission->GetDemand();
					const FTDMissionState& Now = Mission->GetState();
					int32 Best = 0;
					int32 BestGap = INT32_MIN;
					for (int32 Index = 0; Index < TD::NumSystems; ++Index)
					{
						const int32 Gap = Demand[Index] - Now.Allocation[Index] - Now.Levels[Index];
						if (!Mission->IsSystemLocked(static_cast<ETDSystem>(Index)) && Gap > BestGap)
						{
							BestGap = Gap;
							Best = Index;
						}
					}
					if (!Mission->PlaceCell(static_cast<ETDSystem>(Best)))
					{
						break;
					}
				}
				Mission->EndPhase();
			}
		}));
}
#endif

void UTDMissionSubsystem::UseRecommendedDesign()
{
	if (!bDesigning)
	{
		return;
	}
	Design = UTDDesignRules::MakeRecommendedDesign(OptionsTable, ObjectivesTable, Design.ObjectiveId, Design.bRealisticModel);
	PlaySfx(TEXT("UI_Confirm"));
	RefreshDesignScene(Design);
	OnDesignChanged.Broadcast();
}

bool UTDMissionSubsystem::ApplyFix(const FTDDesignIssueLine& Issue)
{
	const FTDDesignFix Fix = UTDDesignRules::SuggestFix(Issue, Design, OptionsTable, ConstantsTable);
	if (!Fix.bValid)
	{
		return false;
	}
	if (Fix.OptionId == TEXT("FalconHeavy"))
	{
		UTDDesignRules::SetPick(Design, OptionsTable, TEXT("Falcon9"), 0);
	}
	return SetDesignPick(Fix.OptionId, Fix.NewCount);
}

void UTDMissionSubsystem::SetAllRolesGo()
{
	const bool bOk = GetDesignBudget().bLaunchable;
	Design.SignedOff.Init(bOk, TD::NumRoles);
	PlaySfx(bOk ? TEXT("UI_Confirm") : TEXT("UI_Back"));
	OnDesignChanged.Broadcast();
}

// ---------------------------------------------------------------- Base operations (gdd.md "Base operations")

namespace
{
	FTDMissionDesign OpsDesignOf(bool bFlown, const FTDMissionDesign& Flown, const UDataTable* Options)
	{
		return bFlown ? Flown : UTDDesignRules::MakeDefaultDesign(Options, TEXT("FirstLight"));
	}
}

int32 UTDMissionSubsystem::GetCurrentPhaseSols() const
{
	int32 Start = 0;
	int32 End = 0;
	GetPhaseSols(State.PhaseIndex, Start, End);
	return FMath::Max(1, End - Start + 1);
}

float UTDMissionSubsystem::GetPhaseHours() const
{
	return UTDOps::PhaseHours(GetCurrentPhaseSols(), ConstantsTable);
}

float UTDMissionSubsystem::GetPhaseEvaHours() const
{
	int32 Start = 0;
	int32 End = 0;
	GetPhaseSols(State.PhaseIndex, Start, End);
	const int32 FirstEva = FMath::RoundToInt(GetConstant(TEXT("FirstEvaSol")));
	const int32 EvaSols = FMath::Max(0, End - FMath::Max(Start, FirstEva) + 1);
	return UTDOps::PhaseEvaHours(EvaSols, ConstantsTable);
}

float UTDMissionSubsystem::GetPlannedHours(int32 CrewIndex, bool bEvaOnly) const
{
	float Hours = 0.f;
	if (!Ops.Crew.IsValidIndex(CrewIndex) || !TasksTable)
	{
		return 0.f;
	}
	for (const FName& Id : Ops.Crew[CrewIndex].Tasks)
	{
		if (const FTDTaskRow* Row = TasksTable->FindRow<FTDTaskRow>(Id, TEXT("TDMissionSubsystem"), false))
		{
			if (!bEvaOnly || Row->bEva)
			{
				Hours += UTDOps::TaskHours(*Row, ConstantsTable);
			}
		}
	}
	return Hours;
}

ETDTaskBlock UTDMissionSubsystem::GetTaskBlock(FName TaskId, int32 CrewIndex) const
{
	if (!HasOps())
	{
		return ETDTaskBlock::WrongPhase;
	}
	return UTDOps::CanPlan(Ops, TasksTable, TaskId, CrewIndex, OpsDesignOf(bHasFlownDesign, FlownDesign, OptionsTable), OptionsTable,
		State.PhaseIndex, GetCurrentPhaseSols(), State.bDustPhase, ConstantsTable);
}

float UTDMissionSubsystem::GetCrewPhaseHours(int32 CrewIndex) const
{
	return UTDOps::CrewPhaseHours(Ops, CrewIndex, GetCurrentPhaseSols(), ConstantsTable);
}

bool UTDMissionSubsystem::PlanTask(FName TaskId, int32 CrewIndex, bool bOverrideRest)
{
	if (!HasOps() || bOpsRunning)
	{
		return false;
	}
	const bool bOk = UTDOps::Plan(Ops, TasksTable, TaskId, CrewIndex, OpsDesignOf(bHasFlownDesign, FlownDesign, OptionsTable), OptionsTable,
		State.PhaseIndex, GetCurrentPhaseSols(), State.bDustPhase, ConstantsTable, bOverrideRest);
	PlaySfx(bOk ? TEXT("UI_Confirm") : TEXT("UI_Back"));
	OnOpsChanged.Broadcast();
	return bOk;
}

void UTDMissionSubsystem::UnplanTask(FName TaskId)
{
	if (bOpsRunning)
	{
		return;
	}
	UTDOps::Unplan(Ops, TaskId);
	UTDOps::RecomputeRestBroken(Ops, TasksTable, GetCurrentPhaseSols(), ConstantsTable);
	PlaySfx(TEXT("UI_Back"));
	OnOpsChanged.Broadcast();
}

void UTDMissionSubsystem::AutoPlanTasks()
{
	if (!HasOps() || bOpsRunning)
	{
		return;
	}
	UTDOps::AutoPlan(Ops, TasksTable, OpsDesignOf(bHasFlownDesign, FlownDesign, OptionsTable), OptionsTable, State.PhaseIndex,
		GetCurrentPhaseSols(), State.bDustPhase, ConstantsTable);
	PlaySfx(TEXT("UI_Confirm"));
	OnOpsChanged.Broadcast();
}

void UTDMissionSubsystem::ClearPlan()
{
	if (bOpsRunning)
	{
		return;
	}
	for (FTDCrewPlan& Crew : Ops.Crew)
	{
		Crew.Tasks.Reset();
	}
	Ops.RestBroken.Reset();
	OnOpsChanged.Broadcast();
}

FText UTDMissionSubsystem::GetCrewName(int32 CrewIndex) const
{
	return GetText(FName(*FString::Printf(TEXT("CrewName_%d"), CrewIndex)));
}

FVector UTDMissionSubsystem::GetSitePosition(FName Site) const
{
	const FTDSiteRow* Row = SitesTable ? SitesTable->FindRow<FTDSiteRow>(Site, TEXT("TDMissionSubsystem"), false) : nullptr;
	return Row ? FVector(Row->X, Row->Y, 0.f) : FVector::ZeroVector;
}

void UTDMissionSubsystem::ApplySolarDust()
{
	if (Ops.SolarDustCells > 0 && !State.bDustPhase)
	{
		State.CellsTotal = FMath::Max(1, State.CellsTotal - Ops.SolarDustCells);
	}
}

bool UTDMissionSubsystem::RunPhase()
{
	if (bOpsRunning || !CanEndPhase())
	{
		return false;
	}
	float MaxHours = 0.f;
	for (int32 Index = 0; Index < Ops.Crew.Num(); ++Index)
	{
		MaxHours = FMath::Max(MaxHours, GetPlannedHours(Index, false));
	}
	if (!HasOps() || MaxHours <= 0.f)
	{
		OpsReport.Reset();
		return EndPhase();
	}
	// Time-lapse: the busiest crew member's plan takes about 14 s; walking is sped up to match.
	const float SecondsPerHour = FMath::Clamp(14.f / MaxHours, 0.4f, 2.5f);
	const float WalkCmPerSec = 450.f;
	{
		TArray<FString> Plan;
		for (int32 Index = 0; Index < Ops.Crew.Num(); ++Index)
		{
			TArray<FString> Ids;
			for (const FName& Id : Ops.Crew[Index].Tasks) { Ids.Add(Id.ToString()); }
			Plan.Add(FString::Printf(TEXT("%d:[%s]"), Index, *FString::Join(Ids, TEXT(","))));
		}
		UE_LOG(LogTouchdownMars, Display, TEXT("Ops: run phase %d (dust %d): %s"), State.PhaseIndex, State.bDustPhase ? 1 : 0, *FString::Join(Plan, TEXT(" ")));
	}
	if (Escort.IsValid())
	{
		Escort->StopEscort();
		Escort.Reset();
	}
	if (PhaseEvaHours.Num() < Ops.Crew.Num())
	{
		PhaseEvaHours.SetNumZeroed(Ops.Crew.Num());
	}
	for (int32 Index = 0; Index < Ops.Crew.Num(); ++Index)
	{
		PhaseEvaHours[Index] += GetPlannedHours(Index, true);
	}
	bOpsRunning = true;
	OpsReport.Reset();
	OpsCrewRunning = 0;
	bool bExpedition = false;
	for (const FTDCrewPlan& Crew : Ops.Crew)
	{
		for (const FName& Id : Crew.Tasks)
		{
			const FTDTaskRow* Row = TasksTable->FindRow<FTDTaskRow>(Id, TEXT("TDMissionSubsystem"), false);
			bExpedition |= Row && Row->Effect == ETDTaskEffect::Expedition;
		}
	}
	if (bExpedition)
	{
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(TEXT("OpsShow_OffloadRover")))
			{
				It->SetActorHiddenInGame(true); // the rover is out on the traverse
			}
		}
	}
	OnOpsChanged.Broadcast();

	// AI crew 1-3 are the level's crew members (the AIR, GREENHOUSE and SHELTER keepers).
	TMap<ETDSystem, ATDCrewMember*> ByCrewSystem;
	for (TActorIterator<ATDCrewMember> It(GetWorld()); It; ++It)
	{
		ByCrewSystem.Add(It->System, *It);
	}
	const ETDSystem CrewSystems[] = { ETDSystem::Air, ETDSystem::Air, ETDSystem::Greenhouse, ETDSystem::Shelter };
	TWeakObjectPtr<UTDMissionSubsystem> WeakThis(this);
	for (int32 CrewIndex = 1; CrewIndex < Ops.Crew.Num() && CrewIndex < 4; ++CrewIndex)
	{
		ATDCrewMember* Actor = ByCrewSystem.FindRef(CrewSystems[CrewIndex]);
		if (!Actor || Ops.Crew[CrewIndex].Tasks.Num() == 0)
		{
			continue;
		}
		const float Z = Actor->GetActorLocation().Z;
		if (!WalkGrid.IsValid())
		{
			WalkGrid = MakeShared<FTDWalkGrid>();
			WalkGrid->Build(GetWorld(), FBox2D(FVector2D(-2600.f, -4200.f), FVector2D(7600.f, 4200.f)), 100.f);
		}
		TSharedPtr<FTDWalkGrid> Grid = WalkGrid;
		auto PathPoints = [Grid, Z](const FVector2D& From, const FVector2D& To)
		{
			TArray<FVector> Points;
			for (const FVector2D& P : Grid->FindPath(From, To))
			{
				Points.Add(FVector(P.X, P.Y, Z));
			}
			if (Points.Num() == 0)
			{
				Points.Add(FVector(To.X, To.Y, Z)); // no path: walk straight
			}
			return Points;
		};
		TArray<FTDCrewStep> Steps;
		FVector2D At(Actor->GetActorLocation().X, Actor->GetActorLocation().Y);
		for (const FName& Id : Ops.Crew[CrewIndex].Tasks)
		{
			const FTDTaskRow* Row = TasksTable->FindRow<FTDTaskRow>(Id, TEXT("TDMissionSubsystem"), false);
			const FTDSiteRow* Site = Row ? SitesTable->FindRow<FTDSiteRow>(Row->Site, TEXT("TDMissionSubsystem"), false) : nullptr;
			if (!Row || !Site)
			{
				continue;
			}
			const FVector2D SitePos(Site->X, Site->Y);
			FTDCrewStep& Step = Steps.AddDefaulted_GetRef();
			Step.Points = PathPoints(At, SitePos);
			Step.Look = FVector(Site->LookX, Site->LookY, Z);
			Step.WorkSeconds = UTDOps::TaskHours(*Row, ConstantsTable) * SecondsPerHour;
			Step.bInside = Site->bInside;
			Step.Status = GetText(Row->NameKey);
			At = SitePos;
		}
		const FVector Home = Actor->GetHomeLocation();
		FTDCrewStep& Back = Steps.AddDefaulted_GetRef();
		Back.Points = PathPoints(At, FVector2D(Home.X, Home.Y));
		Back.Status = GetText(TEXT("Ops_Returning"));
		++OpsCrewRunning;
		Actor->Say(GetText(FName(*FString::Printf(TEXT("Bark_Start_%d"), CrewIndex))), 4.f);
		Actor->RunSteps(MoveTemp(Steps), WalkCmPerSec, [WeakThis]()
		{
			if (WeakThis.IsValid() && --WeakThis->OpsCrewRunning <= 0)
			{
				WeakThis->FinishRun();
			}
		});
	}
	if (OpsCrewRunning == 0)
	{
		FinishRun();
		return true;
	}
	GetWorld()->GetTimerManager().SetTimer(OpsSafetyTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { FinishRun(); }), 90.f, false);
	return true;
}

void UTDMissionSubsystem::FinishRun()
{
	if (!bOpsRunning)
	{
		return;
	}
	bOpsRunning = false;
	GetWorld()->GetTimerManager().ClearTimer(OpsSafetyTimer);
	TMap<FName, TArray<int32>> Who;
	for (int32 Index = 0; Index < Ops.Crew.Num(); ++Index)
	{
		for (const FName& Id : Ops.Crew[Index].Tasks)
		{
			Who.FindOrAdd(Id).Add(Index);
		}
	}
	const TArray<int32> Broke = Ops.RestBroken;
	const TArray<FName> Finished = UTDOps::Finish(Ops, TasksTable);
	RefreshOpsScene();
	OpsReport.Reset();
	for (int32 Index : Broke)
	{
		OpsReport.Add(FText::Format(GetText(TEXT("Ops_RestBroken")), GetCrewName(Index),
			FText::AsNumber(FMath::RoundToInt((1.0 - GetConstant(TEXT("FatigueWorkShare"))) * 100.0))));
	}
	for (const FName& Id : Finished)
	{
		const FTDTaskRow* Row = TasksTable ? TasksTable->FindRow<FTDTaskRow>(Id, TEXT("TDMissionSubsystem"), false) : nullptr;
		if (!Row)
		{
			continue;
		}
		UTDOps::ApplyEffect(*Row, Ops, State, ConstantsTable);
		if (Row->Effect == ETDTaskEffect::Seismic)
		{
			PostQuakeRecord();
		}
		TArray<FString> Names;
		for (int32 Index : Who.FindRef(Id))
		{
			Names.Add(GetCrewName(Index).ToString());
		}
		const FString EffectName = StaticEnum<ETDTaskEffect>()->GetNameStringByValue(static_cast<int64>(Row->Effect));
		OpsReport.Add(FText::Format(GetText(TEXT("Ops_Done")), FText::FromString(FString::Join(Names, TEXT(", "))), GetText(Row->NameKey),
			GetText(FName(*FString::Printf(TEXT("Effect_%s"), *EffectName)))));
	}
	UE_LOG(LogTouchdownMars, Display, TEXT("Ops: phase %d finished %d task(s)"), State.PhaseIndex, Finished.Num());
	TArray<ETDSystem> All = { ETDSystem::Air, ETDSystem::Greenhouse, ETDSystem::Shelter };
	OnLevelsChanged.Broadcast(State, All);
	OnOpsReport.Broadcast(OpsReport);
	EndPhase();
	OnOpsChanged.Broadcast();
}

bool UTDMissionSubsystem::CompleteOwnTask(FName TaskId)
{
	if (bOpsRunning || !Ops.Crew.IsValidIndex(0) || !Ops.Crew[0].Tasks.Contains(TaskId) || !TasksTable)
	{
		return false;
	}
	const FTDTaskRow* Row = TasksTable->FindRow<FTDTaskRow>(TaskId, TEXT("TDMissionSubsystem"), false);
	if (!Row)
	{
		return false;
	}
	TArray<FString> Names;
	for (int32 Index : UTDOps::CrewOn(Ops, TaskId))
	{
		Names.Add(GetCrewName(Index).ToString());
	}
	AddOwnEvaHours(TaskId);
	UTDOps::Unplan(Ops, TaskId);
	Ops.Done.FindOrAdd(TaskId) += 1;
	RefreshOpsScene();
	UTDOps::ApplyEffect(*Row, Ops, State, ConstantsTable);
	if (Row->Effect == ETDTaskEffect::Seismic)
	{
		PostQuakeRecord();
	}
	const FString EffectName = StaticEnum<ETDTaskEffect>()->GetNameStringByValue(static_cast<int64>(Row->Effect));
	const FText Line = FText::Format(GetText(TEXT("Ops_Done")), FText::FromString(FString::Join(Names, TEXT(", "))), GetText(Row->NameKey),
		GetText(FName(*FString::Printf(TEXT("Effect_%s"), *EffectName))));
	OpsReport.Add(Line);
	PlaySfx(TEXT("SystemUp"));
	TArray<ETDSystem> All = { ETDSystem::Air, ETDSystem::Greenhouse, ETDSystem::Shelter };
	OnLevelsChanged.Broadcast(State, All);
	OnMessageArrived.Broadcast(Line);
	OnOpsChanged.Broadcast();
	return true;
}

void UTDMissionSubsystem::DevFillPhase()
{
	if (!IsMissionRunning())
	{
		return;
	}
	if (const FTDEventRow* Event = GetCurrentEvent(); Event && State.EventChoice < 0)
	{
		const int32 CostA = Event->ChoiceADemandAir + Event->ChoiceADemandGreenhouse + Event->ChoiceADemandShelter;
		const int32 CostB = Event->ChoiceBDemandAir + Event->ChoiceBDemandGreenhouse + Event->ChoiceBDemandShelter;
		ChooseEventOption(CostA <= CostB);
	}
	for (int32 Cell = 0; Cell < 20 && !CanEndPhase(); ++Cell)
	{
		const TArray<int32> Demand = GetDemand();
		int32 Best = 0;
		int32 BestGap = INT32_MIN;
		for (int32 Index = 0; Index < TD::NumSystems; ++Index)
		{
			const int32 Gap = Demand[Index] - State.Allocation[Index] - State.Levels[Index];
			if (!IsSystemLocked(static_cast<ETDSystem>(Index)) && Gap > BestGap)
			{
				BestGap = Gap;
				Best = Index;
			}
		}
		if (!PlaceCell(static_cast<ETDSystem>(Best)))
		{
			break;
		}
	}
}

void UTDMissionSubsystem::RefreshOpsScene()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	const FTDMissionDesign& ForDesign = bHasFlownDesign ? FlownDesign : Design;
	int32 Shown = 0;
	int32 Hidden = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		bool bOps = false;
		bool bOpsShow = true;
		bool bDesignTagged = false;
		bool bDesignShow = false;
		for (const FName& Tag : Actor->Tags)
		{
			FString Rest = Tag.ToString();
			if (Rest.RemoveFromStart(TEXT("OpsShow_")))
			{
				bOps = true;
				bOpsShow &= UTDOps::IsDone(Ops, FName(*Rest));
			}
			else if (Rest.RemoveFromStart(TEXT("OpsHide_")))
			{
				bOps = true;
				bOpsShow &= !UTDOps::IsDone(Ops, FName(*Rest));
			}
			else if (Rest.RemoveFromStart(TEXT("DesignShow_")))
			{
				bDesignTagged = true;
				FString OptionId, Index;
				if (Rest.Split(TEXT("_"), &OptionId, &Index, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
				{
					bDesignShow |= UTDDesignRules::GetPickCount(ForDesign, FName(*OptionId)) >= FCString::Atoi(*Index);
				}
			}
		}
		if (!bOps)
		{
			continue;
		}
		// A deployed part shows only when the crew has done its task (and the design brought it).
		const bool bShow = bOpsShow && (!bDesignTagged || bDesignShow);
		Actor->SetActorHiddenInGame(!bShow);
		Actor->SetActorEnableCollision(bShow);
		(bShow ? Shown : Hidden) += 1;
	}
	WalkGrid.Reset(); // deployed parts change where the crew can walk
	UE_LOG(LogTouchdownMars, Display, TEXT("Ops: scene %d deployed parts shown, %d waiting"), Shown, Hidden);
}

void UTDMissionSubsystem::TickEscort()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Player = PC ? PC->GetPawn() : nullptr;
	if (!Player || !IsMissionRunning() || bOpsRunning || !SitesTable)
	{
		return;
	}
	const FVector Hab = GetSitePosition(TEXT("Hab"));
	const FVector P = Player->GetActorLocation();
	const float FromHab = FVector2D::Distance(FVector2D(P), FVector2D(Hab));
	const float OutsideCm = static_cast<float>(GetConstant(TEXT("EscortStartDistanceCm")));
	// Never alone outside (NASA EVA rule): an idle crew member suits up and joins the player away from the hub.
	if (FromHab > OutsideCm && !Escort.IsValid())
	{
		ATDCrewMember* Best = nullptr;
		float BestDistance = TNumericLimits<float>::Max();
		for (TActorIterator<ATDCrewMember> It(World); It; ++It)
		{
			if (!It->IsOnTask() && !It->IsWalking())
			{
				const float D = FVector::Dist2D(It->GetActorLocation(), P);
				if (D < BestDistance)
				{
					BestDistance = D;
					Best = *It;
				}
			}
		}
		UE_LOG(LogTouchdownMars, Display, TEXT("Ops: player %.0f cm from the hub, escort candidate %s"), FromHab, Best ? *Best->GetName() : TEXT("none"));
		if (Best)
		{
			Escort = Best;
			Best->StartEscort(Player);
			Best->Say(GetText(TEXT("Escort_Join")), 4.f);
		}
	}
	else if (FromHab < OutsideCm * 0.5f && Escort.IsValid())
	{
		Escort->Say(GetText(TEXT("Escort_Leave")), 3.f);
		Escort->StopEscort();
		Escort.Reset();
		bCalledFar = false;
	}
	if (!Escort.IsValid())
	{
		return;
	}
	// Callouts only about what the player can see right now (Ellie's rule): the nearest site in reach, once.
	FName Nearest;
	float NearestDistance = static_cast<float>(GetConstant(TEXT("EscortCalloutDistanceCm")));
	for (const FName& Site : SitesTable->GetRowNames())
	{
		const float D = FVector2D::Distance(FVector2D(P), FVector2D(GetSitePosition(Site)));
		if (D < NearestDistance && !CalledOut.Contains(Site) && GetText(FName(*FString::Printf(TEXT("Callout_%s"), *Site.ToString()))).ToString() != FString::Printf(TEXT("Callout_%s"), *Site.ToString()))
		{
			NearestDistance = D;
			Nearest = Site;
		}
	}
	if (!Nearest.IsNone() && Escort->GetBark().IsEmpty())
	{
		CalledOut.Add(Nearest);
		Escort->Say(GetText(FName(*FString::Printf(TEXT("Callout_%s"), *Nearest.ToString()))), 6.f);
		return;
	}
	if (State.bDustPhase && !bCalledDust && Escort->GetBark().IsEmpty())
	{
		bCalledDust = true;
		Escort->Say(GetText(TEXT("Escort_Dust")), 6.f);
		return;
	}
	if (!State.bDustPhase)
	{
		bCalledDust = false;
	}
	if (FromHab > static_cast<float>(GetConstant(TEXT("EscortFarDistanceCm"))) && !bCalledFar && Escort->GetBark().IsEmpty())
	{
		bCalledFar = true;
		Escort->Say(GetText(TEXT("Escort_Far")), 6.f);
	}
}

// Crew radiation dose (Curiosity RAD) and the seismometer study (InSight MQS catalogue) ------------------------------

void UTDMissionSubsystem::CacheRadDays() const
{
	if (bRadCached || !RadTable)
	{
		return;
	}
	bRadCached = true;
	TArray<FTDRadDoseRow*> Rows;
	RadTable->GetAllRows<FTDRadDoseRow>(TEXT("TDMissionSubsystem"), Rows);
	Rows.Sort([](const FTDRadDoseRow& A, const FTDRadDoseRow& B) { return A.UTCDate < B.UTCDate; });
	double QuietSum = 0.0;
	for (const FTDRadDoseRow* Row : Rows)
	{
		(Row->bEvent ? RadEventDays : RadQuietDays).Add(Row);
		if (!Row->bEvent)
		{
			QuietSum += Row->DoseRateBSiRtgCorr;
		}
	}
	RadQuietMean = RadQuietDays.Num() > 0 ? QuietSum / RadQuietDays.Num() : 0.0;
}

double UTDMissionSubsystem::GetShelterDoseFactor() const
{
	// Game assumptions (constants.csv, designer values): how much of the outside rate reaches the crew in the storm shelter.
	const FTDMissionDesign D = bHasFlownDesign ? FlownDesign : UTDDesignRules::MakeDefaultDesign(OptionsTable, TEXT("FirstLight"));
	double Factor = GetConstant(TEXT("RadShelterFactorBasic"));
	if (UTDDesignRules::GetPickCount(D, TEXT("ShelterWaterWall")) > 0)
	{
		Factor = GetConstant(TEXT("RadShelterFactorWaterWall"));
	}
	if (UTDDesignRules::GetPickCount(D, TEXT("ShelterRegolith")) > 0)
	{
		Factor = GetConstant(TEXT("RadShelterFactorRegolith"));
	}
	if (State.OpsRadiationRelief > 0)
	{
		Factor *= GetConstant(TEXT("RadBermFactor"));
	}
	return Factor;
}

double UTDMissionSubsystem::GetEventPeakRate() const
{
	CacheRadDays();
	return RadEventDays.Num() > 0 ? RadEventDays[0]->DoseRateBSiRtgCorr : 0.0;
}

double UTDMissionSubsystem::ForecastEventDayDose(bool bOutside) const
{
	// One crew member on the event day (RAD, 11 Sep 2017): a full EVA outside, or the day in the shelter.
	const double Rate = GetEventPeakRate();
	const double Eva = GetConstant(TEXT("EvaHoursPerEvaSol"));
	if (bOutside)
	{
		return Rate * (Eva / 24.0 + (24.0 - Eva) / 24.0 * GetConstant(TEXT("RadHabitatFactor")));
	}
	return Rate * GetShelterDoseFactor();
}

void UTDMissionSubsystem::AccumulateDose(FName EventId, bool bExposed)
{
	CacheRadDays();
	if (RadQuietDays.Num() == 0)
	{
		return;
	}
	const int32 CrewCount = FMath::Max(4, Ops.Crew.Num());
	if (State.CrewDoseUGy.Num() < CrewCount)
	{
		State.CrewDoseUGy.SetNumZeroed(CrewCount);
	}
	int32 SolStart = 0;
	int32 SolEnd = 0;
	GetPhaseSols(State.PhaseIndex, SolStart, SolEnd);
	const int32 Sols = FMath::Max(1, SolEnd - SolStart + 1);
	const FTDEventRow* Event = GetEvent(EventId);
	const bool bRadEvent = Event && Event->Family == ETDEventFamily::Radiation && RadEventDays.Num() > 0;
	const double Habitat = GetConstant(TEXT("RadHabitatFactor"));
	const double Shelter = GetShelterDoseFactor();
	const double EvaPerSortie = GetConstant(TEXT("EvaHoursPerEvaSol"));

	double MaxPhase = 0.0;
	for (int32 Crew = 0; Crew < CrewCount; ++Crew)
	{
		double EvaLeft = PhaseEvaHours.IsValidIndex(Crew) ? PhaseEvaHours[Crew] : 0.0;
		double Dose = 0.0;
		// Shelter days (event day and the day after): no EVA; else the EVA hours spread over the other sols.
		const int32 Blocked = bRadEvent && !bExposed ? FMath::Min(2, Sols) : 0;
		const int32 FreeSols = FMath::Max(1, Sols - Blocked);
		for (int32 Index = 0; Index < Sols; ++Index)
		{
			// Each sol is one day of the RAD record: the solar event days when the card is a radiation event, else quiet days.
			const FTDRadDoseRow* Day = bRadEvent && RadEventDays.IsValidIndex(Index) ? RadEventDays[Index]
				: RadQuietDays[(SolStart - 1 + Index) % RadQuietDays.Num()];
			const bool bShelterDay = Index < Blocked;
			double Eva = 0.0;
			if (!bShelterDay)
			{
				Eva = (bRadEvent && bExposed && Index == 0) ? FMath::Min(EvaLeft, EvaPerSortie) : EvaLeft / FreeSols;
				if (bRadEvent && bExposed && Index == 0)
				{
					EvaLeft -= Eva;
				}
			}
			Eva = FMath::Clamp(Eva, 0.0, 24.0);
			const double Inside = bShelterDay ? Shelter : Habitat;
			Dose += Day->DoseRateBSiRtgCorr * (Eva / 24.0 + (24.0 - Eva) / 24.0 * Inside);
		}
		State.CrewDoseUGy[Crew] += Dose;
		MaxPhase = FMath::Max(MaxPhase, Dose);
	}
	const FText Line = FText::Format(GetText(bRadEvent ? (bExposed ? TEXT("Dose_PhaseEventOut") : TEXT("Dose_PhaseEventShelter")) : TEXT("Dose_Phase")),
		FText::AsNumber(SolStart), FText::AsNumber(SolEnd), FText::AsNumber(FMath::RoundToInt(MaxPhase)));
	OpsReport.Add(Line);
	OnMessageArrived.Broadcast(Line);
	UE_LOG(LogTouchdownMars, Display, TEXT("Dose: phase %d sols %d-%d radiation event %d exposed %d, highest crew dose %.0f uGy (RAD), mission max %.0f uGy"),
		State.PhaseIndex, SolStart, SolEnd, bRadEvent ? 1 : 0, bExposed ? 1 : 0, MaxPhase, GetMaxCrewDoseUGy());
	PhaseEvaHours.Reset();
}

double UTDMissionSubsystem::GetMaxCrewDoseUGy() const
{
	double Max = 0.0;
	for (double Dose : State.CrewDoseUGy)
	{
		Max = FMath::Max(Max, Dose);
	}
	return Max;
}

double UTDMissionSubsystem::GetDoseEquivalentPerUGy() const
{
	// Estimate: mSv per uGy, scaled so a quiet RAD day equals the average Mars-surface dose equivalent (George 2024).
	CacheRadDays();
	return RadQuietMean > 0.0 ? GetConstant(TEXT("MarsSurfaceDose")) / RadQuietMean : 0.0;
}

FText UTDMissionSubsystem::GetDoseLine(int32 CrewIndex) const
{
	const double UGy = CrewIndex < 0 ? GetMaxCrewDoseUGy() : (State.CrewDoseUGy.IsValidIndex(CrewIndex) ? State.CrewDoseUGy[CrewIndex] : 0.0);
	const double MSv = UGy * GetDoseEquivalentPerUGy();
	const double Limit = GetConstant(TEXT("CareerLimit"));
	FNumberFormattingOptions Two;
	Two.MinimumFractionalDigits = 2;
	Two.MaximumFractionalDigits = 2;
	FNumberFormattingOptions One;
	One.MaximumFractionalDigits = 1;
	return FText::Format(GetText(TEXT("Dose_Line")), FText::AsNumber(UGy / 1000.0, &Two), FText::AsNumber(MSv, &One),
		FText::AsNumber(Limit > 0.0 ? MSv / Limit * 100.0 : 0.0, &Two));
}

void UTDMissionSubsystem::AddOwnEvaHours(FName TaskId)
{
	const FTDTaskRow* Row = TasksTable ? TasksTable->FindRow<FTDTaskRow>(TaskId, TEXT("TDMissionSubsystem"), false) : nullptr;
	if (!Row || !Row->bEva)
	{
		return;
	}
	if (PhaseEvaHours.Num() < 4)
	{
		PhaseEvaHours.SetNumZeroed(4);
	}
	PhaseEvaHours[0] += UTDOps::TaskHours(*Row, ConstantsTable);
}

const FTDMarsquakeRow* UTDMissionSubsystem::GetQuakeRecord(int32 Index) const
{
	if (!QuakeTable)
	{
		return nullptr;
	}
	TArray<FTDMarsquakeRow*> Rows;
	QuakeTable->GetAllRows<FTDMarsquakeRow>(TEXT("TDMissionSubsystem"), Rows);
	Rows.Sort([](const FTDMarsquakeRow& A, const FTDMarsquakeRow& B) { return A.Rank < B.Rank; });
	return Rows.IsValidIndex(Index) ? Rows[Index] : nullptr;
}

FText UTDMissionSubsystem::GetQuakeRecordText(int32 Index) const
{
	const FTDMarsquakeRow* Row = GetQuakeRecord(Index);
	if (!Row || !QuakeTable)
	{
		return FText::GetEmpty();
	}
	FName Name;
	for (const TPair<FName, uint8*>& Pair : QuakeTable->GetRowMap())
	{
		if (reinterpret_cast<const FTDMarsquakeRow*>(Pair.Value) == Row)
		{
			Name = Pair.Key;
		}
	}
	FNumberFormattingOptions One;
	One.MinimumFractionalDigits = 1;
	One.MaximumFractionalDigits = 1;
	FNumberFormattingOptions NoGroup;
	NoGroup.UseGrouping = true;
	NoGroup.MaximumFractionalDigits = 0;
	// The catalogue calls two of the biggest events meteorite impacts, not quakes.
	const bool bImpact = Row->QuakeMLEventType.Contains(TEXT("meteorite"));
	return FText::Format(GetText(bImpact ? TEXT("Quake_RecordImpact") : TEXT("Quake_Record")), FText::FromName(Name),
		FText::FromString(Row->OriginTimeUTC.Left(10)), FText::AsNumber(Row->Mw, &One), FText::AsNumber(Row->DistanceKm, &NoGroup),
		FText::FromString(Row->RegionName));
}

void UTDMissionSubsystem::PostQuakeRecord()
{
	const int32 Index = State.QuakeRecordsStudied - 1;
	const FText Line = GetQuakeRecordText(Index);
	if (!Line.IsEmpty())
	{
		OpsReport.Add(Line);
		OnMessageArrived.Broadcast(Line);
		UE_LOG(LogTouchdownMars, Display, TEXT("Quake: %s"), *Line.ToString());
	}
}

// Habitat heating from Perseverance MEDA ------------------------------------------------------------------------------

double UTDMissionSubsystem::GetHeatingKw(int32 SolStart, int32 SolEnd, double* OutMeanAirC) const
{
	double Sum = 0.0;
	int32 Count = 0;
	for (int32 Sol = SolStart; Sol <= SolEnd; ++Sol)
	{
		if (const FTDWeatherRow* Row = GetWeather(Sol))
		{
			Sum += (Row->AirMinC + Row->AirMaxC) * 0.5;
			++Count;
		}
	}
	if (Count == 0)
	{
		return 0.0;
	}
	const double MeanAir = Sum / Count;
	if (OutMeanAirC)
	{
		*OutMeanAirC = MeanAir;
	}
	// Game model: heat loss = conductance x (cabin temperature - outside air); both are designer values.
	return FMath::Max(0.0, GetConstant(TEXT("HabitatHeatLossKwPerK")) * (GetConstant(TEXT("HabitatCabinC")) - MeanAir));
}

void UTDMissionSubsystem::ApplyMedaHeating()
{
	if (!ConstantsTable || !WeatherTable)
	{
		return;
	}
	double MeanAir = 0.0;
	const double Heating = GetHeatingKw(1, 31, &MeanAir);
	const double CellKw = GetConstant(TEXT("CellKw"));
	const double Base = GetConstant(TEXT("BaseLoadKw"));
	FTDConstantRow* MinCells = ConstantsTable->FindRow<FTDConstantRow>(TEXT("MinCells"), TEXT("TDMissionSubsystem"), false);
	FTDConstantRow* HeatingRow = ConstantsTable->FindRow<FTDConstantRow>(TEXT("HeatingKw"), TEXT("TDMissionSubsystem"), false);
	if (!MinCells || !HeatingRow || CellKw <= 0.0 || Base <= 0.0 || Heating <= 0.0)
	{
		return;
	}
	// The cells a design must carry = equipment load + heating from the real Jezero air (rounded up to whole cells).
	HeatingRow->Value = FMath::RoundToDouble(Heating * 10.0) / 10.0;
	MinCells->Value = FMath::CeilToDouble((Base + Heating) / CellKw);
	UE_LOG(LogTouchdownMars, Display, TEXT("Heating: MEDA mean air %.1f C -> heater %.2f kW, base %.1f kW, design needs %.0f cells"),
		MeanAir, Heating, Base, MinCells->Value);
}
