// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Systems/TDMissionSubsystem.h"

#include "Engine/DataTable.h"
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
	if (Seed < 1000 || Seed > 9999)
	{
		Seed = FMath::RandRange(1000, 9999);
	}
	State = UTDMissionRules::NewMission(Config, Band, Seed);
	bHasMission = true;
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

	if (Outcome.bLost)
	{
		HandleMissionEnded();
		return true;
	}
	if (UTDMissionRules::AdvancePhase(Config, State, Deck))
	{
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
