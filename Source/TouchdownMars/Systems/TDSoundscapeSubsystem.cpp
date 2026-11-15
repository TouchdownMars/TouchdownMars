// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Systems/TDSoundscapeSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LandscapeProxy.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWave.h"
#include "Actors/TDAstronautCharacter.h"
#include "Actors/TDPlayerController.h"
#include "Systems/TDCutsceneSubsystem.h"
#include "Systems/TDMissionSubsystem.h"
#include "Systems/TDSkySubsystem.h"

namespace TDSound
{
	// Mix levels (look values). The helmet layers sit close and quiet; the outside air is faint.
	constexpr float SuitFan = 0.30f;
	constexpr float BreathCalm = 0.24f;
	constexpr float BreathHeavy = 0.34f;
	constexpr float Wind = 0.30f;
	constexpr float StormAir = 0.40f;
	constexpr float StormBuffet = 0.85f;
	constexpr float VisorPatter = 0.60f;
	constexpr float MusicExplore = 0.30f;
	constexpr float MusicStorm = 0.36f;
	constexpr float MusicDesign = 0.28f;
	constexpr float MusicLiftOff = 0.55f;
	constexpr float RadioDuck = 0.5f;
	// Outside air low-pass (Hz): thin CO2 damps the highs, so even the wind is dull.
	constexpr float WindCutoff = 1400.f;
	constexpr float StormCutoff = 900.f;

	float SmoothStep(float A, float B, float X)
	{
		const float T = FMath::Clamp((X - A) / (B - A), 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}
}

bool UTDSoundscapeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

TStatId UTDSoundscapeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDSoundscapeSubsystem, STATGROUP_Tickables);
}

void UTDSoundscapeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	for (int32 Index = 1; Index <= 4; ++Index)
	{
		if (USoundWave* Wave = LoadWave(FString::Printf(TEXT("SFX_TD_Storm_VisorHit_%02d"), Index)))
		{
			VisorHits.Add(Wave);
		}
	}
	for (int32 Index = 1; Index <= 3; ++Index)
	{
		if (USoundWave* Wave = LoadWave(FString::Printf(TEXT("SFX_TD_Suit_Move_%02d"), Index)))
		{
			SuitMoves.Add(Wave);
		}
	}
}

USoundWave* UTDSoundscapeSubsystem::LoadWave(const FString& Name)
{
	return LoadObject<USoundWave>(nullptr, *FString::Printf(TEXT("/Game/TouchdownMars/Audio/%s.%s"), *Name, *Name));
}

USoundAttenuation* UTDSoundscapeSubsystem::GetMarsAir()
{
	static TWeakObjectPtr<USoundAttenuation> Cached;
	if (!Cached.IsValid())
	{
		USoundAttenuation* Settings = NewObject<USoundAttenuation>(GetTransientPackage());
		Settings->AddToRoot();
		FSoundAttenuationSettings& A = Settings->Attenuation;
		A.bAttenuate = true;
		A.bSpatialize = true;
		A.AttenuationShape = EAttenuationShape::Sphere;
		A.AttenuationShapeExtents = FVector(150.f);
		A.FalloffDistance = 2200.f;   // a machine fades out within about 25 m
		A.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
		A.dBAttenuationAtMax = -60.f;
		// High frequencies die first in CO2 at 6 mbar: duller with every metre.
		A.bAttenuateWithLPF = true;
		A.LPFRadiusMin = 100.f;
		A.LPFRadiusMax = 1500.f;
		A.LPFFrequencyAtMin = 7000.f;
		A.LPFFrequencyAtMax = 650.f;
		Cached = Settings;
	}
	return Cached.Get();
}

UAudioComponent* UTDSoundscapeSubsystem::AttachLoop(AActor* Owner, const TCHAR* Name, float Volume)
{
	USoundWave* Wave = Owner ? LoadWave(Name) : nullptr;
	if (!Wave)
	{
		return nullptr;
	}
	Wave->bLooping = true;
	UAudioComponent* Comp = UGameplayStatics::SpawnSoundAttached(Wave, Owner->GetRootComponent(), NAME_None, FVector(0.f, 0.f, 80.f),
		EAttachLocation::KeepRelativeOffset, false, Volume, 1.f, FMath::FRandRange(0.f, 20.f), GetMarsAir(), nullptr, false);
	return Comp;
}

UAudioComponent* UTDSoundscapeSubsystem::Loop2D(const TCHAR* Name)
{
	USoundWave* Wave = LoadWave(Name);
	if (!Wave)
	{
		return nullptr;
	}
	Wave->bLooping = true;
	// Start each loop at a different point so the layers never line up the same way twice.
	return UGameplayStatics::SpawnSound2D(GetWorld(), Wave, 0.f, 1.f, FMath::FRandRange(0.f, 20.f), nullptr, false, false);
}

void UTDSoundscapeSubsystem::BuildModuleHums()
{
	UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	if (!Mission)
	{
		return;
	}
	TArray<ALandscapeProxy*> Landscapes;
	for (TActorIterator<ALandscapeProxy> It(GetWorld()); It; ++It)
	{
		Landscapes.Add(*It);
	}
	// Machinery inside the modules, heard through the walls and the thin air only when standing close.
	struct FHum { const TCHAR* Site; const TCHAR* Sound; float Volume; float Pitch; };
	const FHum Hums[] = {
		{TEXT("HabInside"), TEXT("SFX_TD_Ambience_BaseHum"), 0.9f, 1.f},
		{TEXT("EclssInside"), TEXT("SFX_TD_World_PowerHum"), 0.6f, 1.f},
		{TEXT("IsruInside"), TEXT("SFX_TD_World_PowerHum"), 0.7f, 0.85f},
		{TEXT("LabInside"), TEXT("SFX_TD_World_PowerHum"), 0.35f, 1.12f},
	};
	for (const FHum& Hum : Hums)
	{
		USoundWave* Wave = LoadWave(Hum.Sound);
		if (!Wave)
		{
			continue;
		}
		Wave->bLooping = true;
		FVector At = Mission->GetSitePosition(Hum.Site);
		float Ground = 0.f;
		for (ALandscapeProxy* Proxy : Landscapes)
		{
			const TOptional<float> Z = Proxy->GetHeightAtLocation(At);
			if (Z.IsSet())
			{
				Ground = FMath::Max(Ground, Z.GetValue());
			}
		}
		At.Z = Ground + 150.f;
		if (UAudioComponent* Comp = UGameplayStatics::SpawnSoundAtLocation(GetWorld(), Wave, At, FRotator::ZeroRotator, 0.f, Hum.Pitch,
			FMath::FRandRange(0.f, 20.f), GetMarsAir(), nullptr, false))
		{
			Comp->ComponentTags.Add(FName(*FString::SanitizeFloat(Hum.Volume)));
			ModuleHums.Add(Comp);
		}
	}
}

void UTDSoundscapeSubsystem::PlayOneShot(USoundWave* Wave, float Volume, float Pitch) const
{
	if (Wave)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), Wave, Volume, Pitch);
	}
}

void UTDSoundscapeSubsystem::RadioCall(const UObject* WorldContext, float Seconds)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	UTDSoundscapeSubsystem* Self = World ? World->GetSubsystem<UTDSoundscapeSubsystem>() : nullptr;
	const UTDMissionSubsystem* Mission = World ? World->GetSubsystem<UTDMissionSubsystem>() : nullptr;
	if (!Self || !Mission || !Mission->IsSoundOn())
	{
		return;
	}
	if (!Self->bRadioOpen)
	{
		Self->bRadioOpen = true;
		Self->PlayOneShot(LoadWave(TEXT("SFX_TD_Radio_Open")), 0.55f, FMath::FRandRange(0.97f, 1.03f));
	}
	Self->RadioCloseAt = FMath::Max(Self->RadioCloseAt, Self->Clock + Seconds);
}

void UTDSoundscapeSubsystem::LiftOffMusic(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (UTDSoundscapeSubsystem* Self = World ? World->GetSubsystem<UTDSoundscapeSubsystem>() : nullptr)
	{
		Self->StopCue(1.5f);
		Self->StartCue(EMusic::LiftOff, TEXT("MUS_TD_LiftOff"), TDSound::MusicLiftOff, 0.3f, false);
		Self->LiftOffUntil = Self->Clock + 30.0;
	}
}

void UTDSoundscapeSubsystem::StartCue(EMusic State, const TCHAR* Name, float Volume, float FadeIn, bool bLoop)
{
	MusicState = State;
	USoundWave* Wave = LoadWave(Name);
	if (!Wave)
	{
		return;
	}
	Wave->bLooping = bLoop;
	Music = UGameplayStatics::CreateSound2D(GetWorld(), Wave, 1.f, 1.f, 0.f, nullptr, false, true);
	UE_LOG(LogTemp, Display, TEXT("Soundscape: music %s"), Name);
	if (Music)
	{
		Music->bIsUISound = true;   // keeps playing on the pause menu and the design screen
		MusicBase = Volume;
		Music->SetVolumeMultiplier(Volume);
		Music->FadeIn(FadeIn, 1.f);
	}
}

void UTDSoundscapeSubsystem::StopCue(float FadeOut)
{
	if (Music)
	{
		Music->FadeOut(FadeOut, 0.f);
		Music = nullptr;
	}
	MusicState = EMusic::None;
}

void UTDSoundscapeSubsystem::TickMusic(bool bSoundOn, bool bWalking, float Dust)
{
	if (!bSoundOn)
	{
		StopCue(0.5f);
		LiftOffUntil = 0.0;
		return;
	}
	if (Clock < LiftOffUntil)
	{
		return;
	}
	// Which score fits now: the design screen loop, the storm build, or sparse exploring phrases.
	EMusic Want = !bWalking ? EMusic::Design : (Dust > 0.45f ? EMusic::Storm : EMusic::Explore);
	if (Want != MusicState)
	{
		StopCue(Want == EMusic::Storm ? 3.f : 4.f);
		MusicState = Want;
		if (Want == EMusic::Design)
		{
			StartCue(EMusic::Design, TEXT("MUS_TD_Design"), TDSound::MusicDesign, 3.f, true);
		}
		else if (Want == EMusic::Storm)
		{
			StartCue(EMusic::Storm, StormIndex++ % 2 == 0 ? TEXT("MUS_TD_Storm_A") : TEXT("MUS_TD_Storm_B"), TDSound::MusicStorm, 8.f, true);
		}
		else
		{
			// After a storm or the design screen: a short breath of silence, then the first phrase.
			NextPhraseAt = Clock + 6.0;
		}
	}
	if (MusicState == EMusic::Explore)
	{
		if (Music && !Music->IsPlaying())
		{
			Music = nullptr;
			NextPhraseAt = Clock + FMath::FRandRange(40.f, 80.f);   // long silences: only the suit and the wind
		}
		if (!Music && Clock >= NextPhraseAt)
		{
			StartCue(EMusic::Explore, PhraseIndex++ % 2 == 0 ? TEXT("MUS_TD_Explore_A") : TEXT("MUS_TD_Explore_B"), TDSound::MusicExplore, 4.f, false);
		}
	}
	if (Music)
	{
		Music->SetVolumeMultiplier(MusicBase * (bRadioOpen ? TDSound::RadioDuck : 1.f));
	}
}

void UTDSoundscapeSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	UTDMissionSubsystem* Mission = World ? World->GetSubsystem<UTDMissionSubsystem>() : nullptr;
	if (!Mission || !World->HasBegunPlay())
	{
		return;
	}
	DeltaTime = FMath::Min(DeltaTime, 0.1f);
	Clock += DeltaTime;
	if (!bBuilt)
	{
		bBuilt = true;
		SuitFan = Loop2D(TEXT("SFX_TD_Suit_Fan"));
		BreathCalm = Loop2D(TEXT("SFX_TD_Ambience_Breath"));
		BreathHeavy = Loop2D(TEXT("SFX_TD_Suit_BreathHeavy"));
		Wind = Loop2D(TEXT("SFX_TD_Ambience_MarsWind"));
		StormAir = Loop2D(TEXT("SFX_TD_Ambience_Storm"));
		StormBuffet = Loop2D(TEXT("SFX_TD_Storm_Buffet"));
		VisorPatter = Loop2D(TEXT("SFX_TD_Storm_VisorPatter"));
		for (UAudioComponent* Air : {Wind.Get(), StormAir.Get()})
		{
			if (Air)
			{
				Air->SetLowPassFilterEnabled(true);
			}
		}
		BuildModuleHums();
		int32 Layers = 0;
		for (const UAudioComponent* Layer : {SuitFan.Get(), BreathCalm.Get(), BreathHeavy.Get(), Wind.Get(), StormAir.Get(), StormBuffet.Get(), VisorPatter.Get()})
		{
			Layers += Layer ? 1 : 0;
		}
		UE_LOG(LogTemp, Display, TEXT("Soundscape: %d/7 layers, %d module hums, %d visor hits, %d suit moves"), Layers, ModuleHums.Num(), VisorHits.Num(), SuitMoves.Num());
	}

	const APlayerController* PC = World->GetFirstPlayerController();
	const ATDPlayerController* TDPC = Cast<ATDPlayerController>(PC);
	const ATDAstronautCharacter* Player = PC ? Cast<ATDAstronautCharacter>(PC->GetPawn()) : nullptr;
	const bool bCutscene = UTDCutsceneSubsystem::IsPlaying(this);   // the video carries its own sound
	const bool bSoundOn = Mission->IsSoundOn() && !bCutscene;
	const bool bWalking = bSoundOn && Mission->IsMissionRunning() && Player && !(TDPC && TDPC->IsDesignScreenOpen());
	const UTDSkySubsystem* Sky = World->GetSubsystem<UTDSkySubsystem>();
	const float Dust = Sky ? Sky->GetDustAmount() : 0.f;

	// Exertion: walking fast or working raises it quickly, rest brings it down slowly; a storm adds stress.
	float Speed = 0.f;
	bool bWorking = false;
	if (Player)
	{
		const UCharacterMovementComponent* Move = Player->GetCharacterMovement();
		Speed = Player->GetVelocity().Size2D();
		const float MaxWalk = Move ? FMath::Max(1.f, Move->MaxWalkSpeed) : 300.f;
		bWorking = Player->IsWorking();
		const float Target = FMath::Clamp(Speed / MaxWalk * 0.55f + (bWorking ? 0.7f : 0.f) + Dust * 0.25f, 0.f, 1.f);
		Exertion = FMath::FInterpConstantTo(Exertion, Target, DeltaTime, Target > Exertion ? 0.09f : 0.035f);
	}
	const float Heavy = TDSound::SmoothStep(0.2f, 0.8f, Exertion);

	// 1. Inside the helmet
	const float On = bWalking ? 1.f : 0.f;
	if (SuitFan) { SuitFan->SetVolumeMultiplier(On * TDSound::SuitFan); }
	if (BreathCalm) { BreathCalm->SetVolumeMultiplier(On * TDSound::BreathCalm * (1.f - 0.85f * Heavy)); }
	if (BreathHeavy) { BreathHeavy->SetVolumeMultiplier(On * TDSound::BreathHeavy * Heavy); }

	// Suit fabric: when setting off, when starting work, and now and then on the move.
	NextSuitMove -= DeltaTime;
	if (bWalking && SuitMoves.Num() > 0)
	{
		const bool bSetOff = LastSpeed < 20.f && Speed > 60.f;
		const bool bStartWork = bWorking && !bWasWorking;
		if (bSetOff || bStartWork || (Speed > 60.f && NextSuitMove <= 0.f))
		{
			PlayOneShot(SuitMoves[FMath::RandRange(0, SuitMoves.Num() - 1)], bSetOff || bStartWork ? 0.4f : 0.22f, FMath::FRandRange(0.92f, 1.08f));
			NextSuitMove = FMath::FRandRange(3.5f, 7.f);
		}
	}
	LastSpeed = Speed;
	bWasWorking = bWorking;

	// 2. The storm through the suit: gusts push the suit (sub-bass) and throw sand at the visor.
	const float Gust = FMath::Clamp(0.55f + 0.6f * FMath::PerlinNoise1D(static_cast<float>(Clock) * 0.23f), 0.f, 1.f);
	const float Storm = TDSound::SmoothStep(0.15f, 0.9f, Dust);
	if (StormBuffet)
	{
		StormBuffet->SetVolumeMultiplier(On * TDSound::StormBuffet * Storm * (0.5f + 0.5f * Gust));
		StormBuffet->SetPitchMultiplier(0.9f + 0.15f * Gust);
	}
	if (VisorPatter)
	{
		VisorPatter->SetVolumeMultiplier(On * TDSound::VisorPatter * TDSound::SmoothStep(0.3f, 0.85f, Dust) * (0.45f + 0.55f * Gust));
	}
	NextVisorHit -= DeltaTime;
	if (bWalking && Dust > 0.25f && VisorHits.Num() > 0 && NextVisorHit <= 0.f)
	{
		const float Strength = Storm * (0.4f + 0.6f * Gust);
		PlayOneShot(VisorHits[FMath::RandRange(0, VisorHits.Num() - 1)], 0.25f + 0.6f * Strength, FMath::FRandRange(0.85f, 1.2f));
		NextVisorHit = FMath::Lerp(7.f, 0.7f, Strength) * FMath::FRandRange(0.6f, 1.4f);
	}

	// 3. Outside air: faint and dull. The storm roar takes over from the light wind.
	if (Wind)
	{
		Wind->SetVolumeMultiplier(On * TDSound::Wind * (1.f - 0.7f * Dust) * (0.75f + 0.25f * Gust));
		Wind->SetLowPassFilterFrequency(TDSound::WindCutoff);
	}
	if (StormAir)
	{
		StormAir->SetVolumeMultiplier(On * TDSound::StormAir * Dust);
		StormAir->SetLowPassFilterFrequency(TDSound::StormCutoff + 500.f * Gust);
	}
	for (UAudioComponent* Hum : ModuleHums)
	{
		if (Hum)
		{
			const float Base = Hum->ComponentTags.Num() > 0 ? FCString::Atof(*Hum->ComponentTags[0].ToString()) : 0.6f;
			Hum->SetVolumeMultiplier(On * Base);
		}
	}

	// 4. Radio
	if (bRadioOpen && Clock >= RadioCloseAt)
	{
		bRadioOpen = false;
		if (bSoundOn)
		{
			PlayOneShot(LoadWave(TEXT("SFX_TD_Radio_Close")), 0.45f, FMath::FRandRange(0.97f, 1.03f));
		}
	}

	TickMusic(bSoundOn, bWalking, Dust);
}
