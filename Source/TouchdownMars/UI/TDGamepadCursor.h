// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/AnalogCursor.h"

/**
 * Gamepad cursor for the mouse-driven screens (design room, base computer, mission console, debrief): the left
 * stick moves the cursor, A clicks (engine FAnalogCursor), the right stick scrolls, B goes back. Active only while
 * IsActive() says a screen needs the cursor; otherwise every pad input goes to the game (walking, looking).
 */
class TOUCHDOWNMARS_API FTDGamepadCursor : public FAnalogCursor
{
public:
	FTDGamepadCursor(TFunction<bool()> InIsActive, TFunction<void()> InOnBack);

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override;
	virtual const TCHAR* GetDebugName() const override { return TEXT("TDGamepadCursor"); }

	/** True once any pad input arrived (the HUD can show pad hints). */
	static bool WasPadUsed();

private:
	TFunction<bool()> IsActive;
	TFunction<void()> OnBack;
	float ScrollAccumulator = 0.f;
	bool bWasActive = false;
};
