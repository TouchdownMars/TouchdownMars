// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/CompositeFont.h"
#include "Fonts/SlateFontInfo.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"

/**
 * Shared look of the in-game screens (docs/design.md "Mission screens"): NASA-punk palette, Barlow Semi Condensed
 * and IBM Plex Mono. Header-only so the HUD, Mission Control and later screens draw with the same tokens.
 */
namespace TDStyle
{
	inline const FSlateBrush* White() { return FCoreStyle::Get().GetBrush("GenericWhiteBox"); }

	inline FLinearColor Hex(const TCHAR* V, float A = 1.f) { FLinearColor C = FLinearColor(FColor::FromHex(V)); C.A = A; return C; }
	inline FLinearColor Dim() { return Hex(TEXT("080A0D"), 0.55f); }
	inline FLinearColor Bg() { return Hex(TEXT("E2DFD8")); }
	inline FLinearColor Panel() { return Hex(TEXT("EFEDE8"), 0.97f); }
	inline FLinearColor Raised() { return Hex(TEXT("FAF9F6")); }
	inline FLinearColor Line() { return Hex(TEXT("C8C4BA")); }
	inline FLinearColor Track() { return Hex(TEXT("D9D5CC")); }
	inline FLinearColor Text() { return Hex(TEXT("15181C")); }
	inline FLinearColor Text2() { return Hex(TEXT("5C636E")); }
	inline FLinearColor Charcoal() { return Hex(TEXT("22272E")); }
	inline FLinearColor Red() { return Hex(TEXT("E03C31")); }
	inline FLinearColor Go() { return Hex(TEXT("1E8E4E")); }
	inline FLinearColor Caution() { return Hex(TEXT("B7791F")); }
	inline FLinearColor NoGo() { return Hex(TEXT("D7261E")); }
	inline FLinearColor Orange() { return Hex(TEXT("F07F2E")); }
	inline FLinearColor Blue() { return Hex(TEXT("3A8DDE")); }
	inline FLinearColor Violet() { return Hex(TEXT("B58CF0")); }
	inline FLinearColor Green() { return Hex(TEXT("3FB27F")); }

	inline TSharedPtr<const FCompositeFont> TeamFont()
	{
		static TSharedPtr<FCompositeFont> Cached;
		static bool bTried = false;
		if (!bTried)
		{
			bTried = true;
			const FString Dir = FPaths::ProjectContentDir() / TEXT("TouchdownMars/UI/Fonts");
			const FString Regular = Dir / TEXT("TDFont_Regular.ttf");
			const FString Bold = Dir / TEXT("TDFont_Bold.ttf");
			if (FPaths::FileExists(Regular) && FPaths::FileExists(Bold))
			{
				Cached = MakeShared<FCompositeFont>();
				Cached->DefaultTypeface.AppendFont(TEXT("Regular"), Regular, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
				Cached->DefaultTypeface.AppendFont(TEXT("Bold"), Bold, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
				const FString Mono = Dir / TEXT("TDFont_Mono.ttf");
				if (FPaths::FileExists(Mono))
				{
					Cached->DefaultTypeface.AppendFont(TEXT("Mono"), Mono, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
				}
			}
		}
		return Cached;
	}

	inline FSlateFontInfo Font(float Size, bool bBold = false)
	{
		if (TSharedPtr<const FCompositeFont> Team = TeamFont())
		{
			return FSlateFontInfo(Team, Size, bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular")));
		}
		return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
	}

	inline FSlateFontInfo HeadFont(float Size)
	{
		FSlateFontInfo Info = Font(Size, true);
		Info.LetterSpacing = 90;
		return Info;
	}

	inline FSlateFontInfo NumFont(float Size)
	{
		if (TSharedPtr<const FCompositeFont> Team = TeamFont())
		{
			return FSlateFontInfo(Team, Size, FName(TEXT("Mono")));
		}
		return FCoreStyle::GetDefaultFontStyle("Mono", Size);
	}
}
