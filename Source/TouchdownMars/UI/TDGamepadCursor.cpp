// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "UI/TDGamepadCursor.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "InputCoreTypes.h"

namespace
{
	bool GPadUsed = false;
}

FTDGamepadCursor::FTDGamepadCursor(TFunction<bool()> InIsActive, TFunction<void()> InOnBack)
	: IsActive(MoveTemp(InIsActive))
	, OnBack(MoveTemp(InOnBack))
{
	// Readable on a TV and precise on small buttons: a bit faster than the engine default, slower near widgets.
	SetMaxSpeed(1800.f);
	SetAcceleration(2400.f);
	SetStickySlowdown(0.5f);
	SetDeadZone(0.2f);
}

bool FTDGamepadCursor::WasPadUsed()
{
	return GPadUsed;
}

void FTDGamepadCursor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
	const bool bActive = IsActive && IsActive();
	if (!bActive)
	{
		if (bWasActive)
		{
			ClearAnalogValues();
		}
		bWasActive = false;
		return;
	}
	if (!bWasActive)
	{
		// Coming from walking: start the cursor in the middle of the screen.
		ClearAnalogValues();
		if (TSharedPtr<FSlateUser> User = SlateApp.GetCursorUser())
		{
			if (TSharedPtr<SWindow> Window = SlateApp.GetActiveTopLevelWindow())
			{
				const FVector2D Centre = Window->GetPositionInScreen() + Window->GetSizeInScreen() * 0.5f;
				User->SetCursorPosition(Centre);
			}
		}
	}
	bWasActive = true;
	FAnalogCursor::Tick(DeltaTime, SlateApp, Cursor);

	// Right stick scrolls lists (mouse-wheel steps at a steady rate).
	const float Stick = GetAnalogValues(EAnalogStick::Right).Y;
	if (FMath::Abs(Stick) > 0.3f)
	{
		ScrollAccumulator += Stick * DeltaTime * 10.f;
		if (FMath::Abs(ScrollAccumulator) >= 1.f)
		{
			const float Steps = FMath::TruncToFloat(ScrollAccumulator);
			ScrollAccumulator -= Steps;
			if (TSharedPtr<FSlateUser> User = SlateApp.GetCursorUser())
			{
				const FPointerEvent Wheel(User->GetUserIndex(), FSlateApplication::CursorPointerIndex, User->GetCursorPosition(), User->GetCursorPosition(),
					TSet<FKey>(), EKeys::MouseWheelAxis, Steps, FModifierKeysState());
				SlateApp.ProcessMouseWheelOrGestureEvent(Wheel, nullptr);
			}
		}
	}
	else
	{
		ScrollAccumulator = 0.f;
	}
}

bool FTDGamepadCursor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey().IsGamepadKey())
	{
		GPadUsed = true;
	}
	if (!IsActive || !IsActive())
	{
		return false;
	}
	if (InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right)
	{
		if (!InKeyEvent.IsRepeat() && OnBack)
		{
			OnBack();
		}
		return true;
	}
	return FAnalogCursor::HandleKeyDownEvent(SlateApp, InKeyEvent);
}

bool FTDGamepadCursor::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (!IsActive || !IsActive())
	{
		return false;
	}
	if (InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right)
	{
		return true;
	}
	return FAnalogCursor::HandleKeyUpEvent(SlateApp, InKeyEvent);
}

bool FTDGamepadCursor::HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent)
{
	const FKey Key = InAnalogInputEvent.GetKey();
	if (Key.IsGamepadKey() && FMath::Abs(InAnalogInputEvent.GetAnalogValue()) > 0.3f)
	{
		GPadUsed = true;
	}
	if (!IsActive || !IsActive())
	{
		return false; // walking: the sticks belong to the astronaut
	}
	// Only the sticks drive the cursor; triggers fall through.
	if (Key == EKeys::Gamepad_LeftX || Key == EKeys::Gamepad_LeftY || Key == EKeys::Gamepad_RightX || Key == EKeys::Gamepad_RightY)
	{
		return FAnalogCursor::HandleAnalogInputEvent(SlateApp, InAnalogInputEvent);
	}
	return false;
}
