// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
// Pause menu (Esc / Menu) in the design-room style: resume, back to the design room, language, sound, motion,
// screen, look sensitivity, invert look, hold or tap to work, quit. Scaled from a 1920x1080 canvas.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UTDMissionSubsystem;
class SBorder;

DECLARE_DELEGATE(FTDPauseAction);

class TOUCHDOWNMARS_API STDPauseMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STDPauseMenu) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UTDMissionSubsystem>, Mission)
		SLATE_EVENT(FTDPauseAction, OnResume)
		SLATE_EVENT(FTDPauseAction, OnRedesign)
		SLATE_EVENT(FTDPauseAction, OnQuit)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void Rebuild();
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	FText Txt(const TCHAR* Key) const;
	TWeakObjectPtr<UTDMissionSubsystem> Mission;
	FTDPauseAction OnResume;
	FTDPauseAction OnRedesign;
	FTDPauseAction OnQuit;
	TSharedPtr<SBorder> Root;
};
