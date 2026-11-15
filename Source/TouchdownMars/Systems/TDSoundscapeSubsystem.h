// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDSoundscapeSubsystem.generated.h"

class UAudioComponent;
class USoundAttenuation;
class USoundWave;

/**
 * The player's sound world, built from what a suited astronaut on Mars would really hear.
 *
 * Mars air is thin CO2: sound is much quieter than on Earth and the high frequencies die within metres
 * (Perseverance SuperCam microphone, Maurice et al., Nature 605, 2022). So the mix has four layers:
 * 1. Inside the helmet, close and dry: suit fan and oxygen flow, breathing that follows exertion, suit fabric.
 * 2. Through the suit body (contact): boot thumps, the storm buffeting the suit, sand grains ticking on the visor.
 * 3. Outside air, faint and muffled: wind, the storm, machines and modules (3D, low-passed with distance).
 * 4. Radio: a click and static before and after each call; the music ducks under it.
 * Music is sparse and adaptive (phrases with long silences while exploring, a slow build in a storm, a
 * calm loop on the design screen, a swell at lift-off), the way film scores leave room for the breathing.
 * All levels and timings are look values ([DESIGNER VALUE]); no NASA number is shown.
 */
UCLASS()
class TOUCHDOWNMARS_API UTDSoundscapeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }

	/** A radio call: squelch open now, close after Seconds; the music ducks meanwhile. */
	static void RadioCall(const UObject* WorldContext, float Seconds);
	/** The lift-off swell (once). */
	static void LiftOffMusic(const UObject* WorldContext);
	/** Thin-air 3D falloff: quiet beyond a few metres and duller with distance. Shared by every world sound. */
	static USoundAttenuation* GetMarsAir();
	/** A looping 3D sound attached to an actor (machines). */
	static UAudioComponent* AttachLoop(AActor* Owner, const TCHAR* Name, float Volume);
	static USoundWave* LoadWave(const FString& Name);

private:
	// Layers (2D)
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> SuitFan;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> BreathCalm;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> BreathHeavy;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Wind;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> StormAir;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> StormBuffet;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> VisorPatter;
	// Modules heard from outside (3D)
	UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> ModuleHums;
	// One-shot sets
	UPROPERTY(Transient) TArray<TObjectPtr<USoundWave>> VisorHits;
	UPROPERTY(Transient) TArray<TObjectPtr<USoundWave>> SuitMoves;
	// Music
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Music;

	enum class EMusic : uint8 { None, Design, Explore, Storm, LiftOff };
	EMusic MusicState = EMusic::None;
	float MusicBase = 0.f;          // volume of the current cue before ducking
	double NextPhraseAt = 0.0;
	int32 PhraseIndex = 0;
	int32 StormIndex = 0;
	double LiftOffUntil = 0.0;
	double RadioCloseAt = 0.0;
	bool bRadioOpen = false;

	float Exertion = 0.f;
	float NextVisorHit = 0.f;
	float NextSuitMove = 0.f;
	float LastSpeed = 0.f;
	bool bWasWorking = false;
	bool bBuilt = false;
	double Clock = 0.0;

	UAudioComponent* Loop2D(const TCHAR* Name);
	void BuildModuleHums();
	void TickMusic(bool bSoundOn, bool bWalking, float Dust);
	void StartCue(EMusic State, const TCHAR* Name, float Volume, float FadeIn, bool bLoop);
	void StopCue(float FadeOut);
	void PlayOneShot(USoundWave* Wave, float Volume, float Pitch) const;
};
