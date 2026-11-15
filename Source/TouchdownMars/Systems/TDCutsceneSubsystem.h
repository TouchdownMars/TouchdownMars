// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDCutsceneSubsystem.generated.h"

class UMediaPlayer;
class UMediaTexture;
class UFileMediaSource;
class UMediaSoundComponent;
class SWidget;

/**
 * Full-screen video cutscenes between the game's phases: the launch from Earth, the landing at Jezero, the crew's
 * lift-off from Mars and the return to Earth. The clips are MP4 files in Content/Movies (staged as loose files),
 * AI-generated with Google Veo (Gemini) from the team's prompts and, for the Mars shots, the game's own frames as
 * reference (sources.csv: GeminiVeoCutscenes). Each clip shows a short caption (EN/TR) and can be skipped with any
 * key or a click. While a clip plays the soundscape and the music stay quiet. A missing clip is skipped silently.
 * Dev runs (-TDSurface, -TDCam, -TDShotTour, -TDWorkTest, -TDNoCutscenes) play no cutscenes.
 */
UCLASS()
class TOUCHDOWNMARS_API UTDCutsceneSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual void Deinitialize() override;

	/** Plays the clips in order (names without extension, e.g. "CS_Launch"), then calls OnDone. */
	static void Play(const UObject* WorldContext, const TArray<FName>& Clips, TFunction<void()> OnDone);
	static bool IsPlaying(const UObject* WorldContext);
	static bool Enabled();

	void Skip();

private:
	UPROPERTY(Transient) TObjectPtr<UMediaPlayer> Player;
	UPROPERTY(Transient) TObjectPtr<UMediaTexture> Texture;
	UPROPERTY(Transient) TObjectPtr<UFileMediaSource> Source;
	UPROPERTY(Transient) TObjectPtr<UMediaSoundComponent> Sound;
	UPROPERTY(Transient) TObjectPtr<AActor> SoundOwner;

	TArray<FName> Queue;
	TFunction<void()> Done;
	TSharedPtr<SWidget> Overlay;
	TSharedPtr<class STextBlock> Caption;
	TSharedPtr<FSlateBrush> Brush;
	double ClipStart = 0.0;
	bool bStarted = false;
	bool bActive = false;

	void StartNext();
	void Finish();
	void BuildOverlay();
	void RemoveOverlay();
};
