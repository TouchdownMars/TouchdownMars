// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "UI/TDPauseMenu.h"

#include "Systems/TDMissionSubsystem.h"
#include "UI/TDStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

using namespace TDStyle;

namespace
{
	const FButtonStyle* PauseButtonStyle()
	{
		static FSlateColorBrush Normal(FLinearColor::White);
		static FSlateColorBrush Hovered(FLinearColor(0.82f, 0.82f, 0.82f, 1.f));
		static FSlateColorBrush Pressed(FLinearColor(0.66f, 0.66f, 0.66f, 1.f));
		static const FButtonStyle Style = FButtonStyle().SetNormal(Normal).SetHovered(Hovered).SetPressed(Pressed).SetDisabled(Normal)
			.SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f));
		return &Style;
	}

	TSharedRef<SWidget> Row(const FText& Label, const FText& Value, FOnClicked OnClicked, const FLinearColor& Fill, bool bStrong = false)
	{
		const bool bDark = Fill.GetLuminance() < 0.45f;
		return SNew(SButton).ButtonStyle(PauseButtonStyle()).ButtonColorAndOpacity(Fill).ContentPadding(FMargin(18.f, 12.f)).OnClicked(OnClicked)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Label).Font(Font(bStrong ? 22 : 18, true)).ColorAndOpacity(bDark ? Hex(TEXT("FFFFFF")) : Text())
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Value).Font(NumFont(16)).ColorAndOpacity(bDark ? Hex(TEXT("FFFFFF")) : Red())
			]
		];
	}
}

void STDPauseMenu::Construct(const FArguments& InArgs)
{
	Mission = InArgs._Mission;
	OnResume = InArgs._OnResume;
	OnRedesign = InArgs._OnRedesign;
	OnQuit = InArgs._OnQuit;
	ChildSlot
	[
		SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Dim()).Padding(0.f)
		[
			SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
			[
				SNew(SBox).WidthOverride(1920.f).HeightOverride(1080.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(760.f)
					[
						SAssignNew(Root, SBorder).BorderImage(White()).BorderBackgroundColor(Panel()).Padding(FMargin(28.f, 24.f))
					]
				]
			]
		]
	];
	Rebuild();
}

FText STDPauseMenu::Txt(const TCHAR* Key) const
{
	return Mission.IsValid() ? Mission->GetText(FName(Key)) : FText::FromString(Key);
}

FReply STDPauseMenu::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (InKeyEvent.IsRepeat())
	{
		return FReply::Handled();
	}
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_Special_Right || Key == EKeys::Gamepad_FaceButton_Right)
	{
		OnResume.ExecuteIfBound();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void STDPauseMenu::Rebuild()
{
	UTDMissionSubsystem* M = Mission.Get();
	if (!Root.IsValid() || !M)
	{
		return;
	}
	auto OnOff = [this](bool bOn) { return Txt(bOn ? TEXT("UI_On") : TEXT("UI_Off")); };
	auto Act = [this](TFunction<void(UTDMissionSubsystem*)> Do)
	{
		return FOnClicked::CreateLambda([this, Do]()
		{
			if (Mission.IsValid())
			{
				Do(Mission.Get());
				Mission->PlaySfx(TEXT("UI_Confirm"));
			}
			Rebuild();
			return FReply::Handled();
		});
	};
	FNumberFormattingOptions One;
	One.MinimumFractionalDigits = 1;
	One.MaximumFractionalDigits = 2;

	TSharedRef<SVerticalBox> List = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)[SNew(SBox).WidthOverride(8.f).HeightOverride(30.f)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Red())]]
			+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(STextBlock).Text(Txt(TEXT("Pause_Title"))).Font(HeadFont(28)).ColorAndOpacity(Text())]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 16.f)[SNew(STextBlock).Text(Txt(TEXT("Pause_Hint"))).Font(Font(14)).ColorAndOpacity(Text2())]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f)
		[
			Row(Txt(TEXT("UI_Resume")), FText::GetEmpty(), FOnClicked::CreateLambda([this]() { OnResume.ExecuteIfBound(); return FReply::Handled(); }), Red(), true)
		];
	List->AddSlot().AutoHeight().Padding(0.f, 14.f, 0.f, 4.f)[SNew(STextBlock).Text(Txt(TEXT("Pause_Controls"))).Font(HeadFont(13)).ColorAndOpacity(Text2())];
	List->AddSlot().AutoHeight().Padding(0.f, 3.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[
			Row(Txt(TEXT("Pause_LookSpeed")), FText::Format(FText::FromString(TEXT("{0}x")), FText::AsNumber(M->GetLookSpeed(), &One)),
				Act([](UTDMissionSubsystem* S) { S->SetLookSpeed(S->GetLookSpeed() >= 1.95f ? 0.5f : S->GetLookSpeed() + 0.25f); }), Raised())
		]
	];
	List->AddSlot().AutoHeight().Padding(0.f, 3.f)
	[
		Row(Txt(TEXT("Pause_InvertY")), OnOff(M->IsLookInverted()), Act([](UTDMissionSubsystem* S) { S->SetLookInverted(!S->IsLookInverted()); }), Raised())
	];
	List->AddSlot().AutoHeight().Padding(0.f, 3.f)
	[
		Row(Txt(TEXT("Pause_TapWork")), Txt(M->IsTapToWork() ? TEXT("Pause_TapWorkTap") : TEXT("Pause_TapWorkHold")),
			Act([](UTDMissionSubsystem* S) { S->SetTapToWork(!S->IsTapToWork()); }), Raised())
	];
	List->AddSlot().AutoHeight().Padding(0.f, 14.f, 0.f, 4.f)[SNew(STextBlock).Text(Txt(TEXT("Pause_Game"))).Font(HeadFont(13)).ColorAndOpacity(Text2())];
	if (UTDMissionSubsystem::bTurkishEnabled)
	List->AddSlot().AutoHeight().Padding(0.f, 3.f)
	[
		Row(Txt(TEXT("Pause_Language")), FText::FromString(M->GetLanguage() == ETDLanguage::EN ? TEXT("English") : TEXT("Türkçe")),
			Act([](UTDMissionSubsystem* S) { S->SetLanguage(S->GetLanguage() == ETDLanguage::EN ? ETDLanguage::TR : ETDLanguage::EN); }), Raised())
	];
	List->AddSlot().AutoHeight().Padding(0.f, 3.f)
	[
		Row(Txt(TEXT("UI_Sound")), OnOff(M->IsSoundOn()), Act([](UTDMissionSubsystem* S) { S->SetSoundOn(!S->IsSoundOn()); }), Raised())
	];
	List->AddSlot().AutoHeight().Padding(0.f, 3.f)
	[
		Row(Txt(TEXT("UI_ReduceMotion")), OnOff(M->IsReduceMotion()), Act([](UTDMissionSubsystem* S) { S->SetReduceMotion(!S->IsReduceMotion()); }), Raised())
	];
	List->AddSlot().AutoHeight().Padding(0.f, 3.f)
	[
		Row(Txt(TEXT("Pause_Screen")), Txt(M->IsFullscreen() ? TEXT("Pause_ScreenFull") : TEXT("Pause_ScreenWindow")), Act([](UTDMissionSubsystem* S) { S->SetFullscreen(!S->IsFullscreen()); }), Raised())
	];
	List->AddSlot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
	[
		Row(Txt(TEXT("UI_Quit")), FText::GetEmpty(), FOnClicked::CreateLambda([this]() { OnQuit.ExecuteIfBound(); return FReply::Handled(); }), Line())
	];
	Root->SetContent(List);
}
