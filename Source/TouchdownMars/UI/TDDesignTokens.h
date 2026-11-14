// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#pragma once

#include "CoreMinimal.h"
#include "Model/TDTypes.h"

/** Colors and sizes from docs/design.md. FLinearColor(FColor) converts the sRGB hex to linear. */
namespace TDTokens
{
	inline FLinearColor Hex(const TCHAR* Value) { return FLinearColor(FColor::FromHex(Value)); }

	inline FLinearColor CorePrimary() { return Hex(TEXT("16243A")); }
	inline FLinearColor CoreSecondary() { return Hex(TEXT("22344F")); }
	inline FLinearColor CoreBackground() { return Hex(TEXT("0E1726")); }
	inline FLinearColor CoreText() { return Hex(TEXT("F4F1EA")); }
	inline FLinearColor CoreAccent() { return Hex(TEXT("F7D046")); }

	inline FLinearColor SemDanger() { return Hex(TEXT("E5484D")); }
	inline FLinearColor SemSuccess() { return Hex(TEXT("3FB27F")); }
	inline FLinearColor SemLow() { return Hex(TEXT("F2C14E")); }
	inline FLinearColor SemDisabled() { FLinearColor Color = Hex(TEXT("5B6577")); Color.A = 0.6f; return Color; }

	inline FLinearColor System(ETDSystem System)
	{
		switch (System)
		{
		case ETDSystem::Air: return Hex(TEXT("3A8DDE"));
		case ETDSystem::Greenhouse: return Hex(TEXT("3FB27F"));
		default: return Hex(TEXT("F07F2E"));
		}
	}

	/** Level → state color: 0-1 Danger, 2 Low, 3-4 Good (system color). */
	inline FLinearColor LevelColor(ETDSystem SystemValue, int32 Level)
	{
		return Level <= 1 ? SemDanger() : (Level == 2 ? SemLow() : System(SystemValue));
	}

	inline FName SystemTextKey(ETDSystem System)
	{
		switch (System)
		{
		case ETDSystem::Air: return TEXT("System_Air");
		case ETDSystem::Greenhouse: return TEXT("System_Greenhouse");
		default: return TEXT("System_Shelter");
		}
	}

	constexpr int32 SizeT1 = 48;
	constexpr int32 SizeT2 = 36;
	constexpr int32 SizeBody = 28;
	constexpr int32 SizeCaption = 24;
}
