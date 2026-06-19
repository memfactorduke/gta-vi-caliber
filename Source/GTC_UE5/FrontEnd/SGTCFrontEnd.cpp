#include "SGTCFrontEnd.h"

#include "GTCFrontEndPalette.h"
#include "MediaTexture.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Notifications/SProgressBar.h"

// Local aliases onto the shared shell palette so the menu reads as the same
// production as the intro video, the loading cover, and the pause menu.
namespace GTCFrontEndStyle
{
	static const FLinearColor Accent = GTCFrontEndPalette::Cyan;        // loading-bar fill (data accent)
	static const FLinearColor TextDim = GTCFrontEndPalette::TextDim;
	static const FLinearColor TextBright = GTCFrontEndPalette::TextBright;
	static const FLinearColor FooterColor = GTCFrontEndPalette::Footer;
	static const FLinearColor KickerColor = GTCFrontEndPalette::Kicker;
}

void SGTCFrontEnd::Construct(const FArguments& InArgs)
{
	OnSkip = InArgs._OnSkip;
	OnStart = InArgs._OnStart;
	OnSettings = InArgs._OnSettings;
	OnExit = InArgs._OnExit;

	VideoBrush.SetResourceObject(InArgs._VideoTexture);
	VideoBrush.ImageSize = FVector2D(1920.0f, 1080.0f);
	VideoBrush.DrawAs = ESlateBrushDrawType::Image;
	VideoBrush.TintColor = FLinearColor::White;

	const FSlateFontInfo KickerFont = FCoreStyle::GetDefaultFontStyle("Bold", 13);
	const FSlateFontInfo FooterFont = FCoreStyle::GetDefaultFontStyle("Regular", 14);
	const FSlateFontInfo LoadingFont = FCoreStyle::GetDefaultFontStyle("Bold", 22);

	MenuActions = { OnStart, OnSettings, OnExit };

	TSharedRef<SVerticalBox> ButtonStack = SNew(SVerticalBox);
	ButtonStack->AddSlot().AutoHeight().Padding(0.0f, 6.0f)
		[ MakeMenuButton(NSLOCTEXT("GTC", "Start", "START GAME"), OnStart, 0) ];
	ButtonStack->AddSlot().AutoHeight().Padding(0.0f, 6.0f)
		[ MakeMenuButton(NSLOCTEXT("GTC", "Settings", "SETTINGS"), OnSettings, 1) ];
	ButtonStack->AddSlot().AutoHeight().Padding(0.0f, 6.0f)
		[ MakeMenuButton(NSLOCTEXT("GTC", "Exit", "EXIT"), OnExit, 2) ];

	ChildSlot
	[
		SNew(SOverlay)

		// 1. Full-screen background video.
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
		[
			SNew(SImage).Image(&VideoBrush)
		]

		// 2. Left scrim so menu text stays legible over bright video.
		+ SOverlay::Slot()
		.HAlign(HAlign_Left).VAlign(VAlign_Fill)
		[
			SNew(SBox).WidthOverride(720.0f)
			[
				SNew(SColorBlock).Color(this, &SGTCFrontEnd::GetScrimColor)
			]
		]

		// 3. Main menu (kicker + accent rail + buttons), left-anchored, centered.
		+ SOverlay::Slot()
		.HAlign(HAlign_Left).VAlign(VAlign_Center)
		.Padding(72.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.Visibility(this, &SGTCFrontEnd::GetMenuVisibility)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Font(KickerFont)
					.ColorAndOpacity(GTCFrontEndStyle::KickerColor)
					.Text(NSLOCTEXT("GTC", "MainMenu", "MAIN MENU"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 18.0f)
				[
					SNew(SBox).WidthOverride(172.0f).HeightOverride(3.0f)
					[
						SNew(SColorBlock).Color(this, &SGTCFrontEnd::GetAccentColor)
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					ButtonStack
				]
			]
		]

		// 4. Footer hint.
		+ SOverlay::Slot()
		.HAlign(HAlign_Left).VAlign(VAlign_Bottom)
		.Padding(72.0f, 0.0f, 0.0f, 40.0f)
		[
			SNew(STextBlock)
			.Visibility(this, &SGTCFrontEnd::GetMenuVisibility)
			.Font(FooterFont)
			.ColorAndOpacity(GTCFrontEndStyle::FooterColor)
			.Text(NSLOCTEXT("GTC", "Footer", "ENTER  SELECT      ESC  BACK"))
		]

		// 5. Loading panel.
		+ SOverlay::Slot()
		.HAlign(HAlign_Left).VAlign(VAlign_Bottom)
		.Padding(108.0f, 0.0f, 0.0f, 70.0f)
		[
			SNew(SBox)
			.Visibility(this, &SGTCFrontEnd::GetLoadingVisibility)
			.WidthOverride(372.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Font(KickerFont)
					.ColorAndOpacity(FLinearColor(0.9f, 0.96f, 1.0f, 0.72f))
					.Text(NSLOCTEXT("GTC", "EnteringWorld", "ENTERING CALIBER ISLES"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 12.0f)
				[
					SNew(STextBlock)
					.Font(LoadingFont)
					.ColorAndOpacity(GTCFrontEndStyle::TextBright)
					.Text(this, &SGTCFrontEnd::GetLoadingText)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox).HeightOverride(4.0f)
					[
						SNew(SProgressBar)
						.Percent(this, &SGTCFrontEnd::GetLoadingPercent)
						.FillColorAndOpacity(GTCFrontEndStyle::Accent)
					]
				]
			]
		]

		// 5b. Intro loading strip — world preloading under the boot video.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center).VAlign(VAlign_Bottom)
		.Padding(0.0f, 0.0f, 0.0f, 48.0f)
		[
			SNew(SBox)
			.Visibility(this, &SGTCFrontEnd::GetIntroVisibility)
			.WidthOverride(360.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
					.ColorAndOpacity(FLinearColor(0.92f, 0.96f, 1.0f, 0.30f))
					.Text(this, &SGTCFrontEnd::GetIntroPromptText)
				]
			]
		]

		// 6. Black fade overlay (never eats input).
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
		[
			SNew(SColorBlock)
			.Color(this, &SGTCFrontEnd::GetFadeColor)
			.Visibility(EVisibility::HitTestInvisible)
		]
	];
}

TSharedRef<SButton> SGTCFrontEnd::MakeMenuButton(const FText& Label, FSimpleDelegate Clicked, int32 Index)
{
	const FSlateFontInfo ButtonFont = FCoreStyle::GetDefaultFontStyle("Bold", 34);

	TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
		.ContentPadding(FMargin(0.0f, 4.0f))
		.HAlign(HAlign_Left)
		.OnHovered_Lambda([this, Index]() { FocusButton(Index); })
		.OnClicked_Lambda([Clicked]() -> FReply
		{
			Clicked.ExecuteIfBound();
			return FReply::Handled();
		})
		[
			SNew(STextBlock)
			.Font(ButtonFont)
			.Text(Label)
			.ColorAndOpacity_Lambda([this, Index]() -> FSlateColor
			{
				return FSlateColor(Index == FocusedIndex
					? GTCFrontEndStyle::TextBright
					: GTCFrontEndStyle::TextDim);
			})
		];

	if (MenuButtons.Num() <= Index)
	{
		MenuButtons.SetNum(Index + 1);
	}
	MenuButtons[Index] = Button;
	return Button;
}

void SGTCFrontEnd::SetVisual(EGTCFrontEndVisual InVisual)
{
	Visual = InVisual;
	if (Visual == EGTCFrontEndVisual::Menu)
	{
		FocusButton(0);
	}
}

void SGTCFrontEnd::SetLoadingProgress(float InProgress01)
{
	LoadingProgress = FMath::Clamp(InProgress01, 0.0f, 1.0f);
}

void SGTCFrontEnd::FocusButton(int32 Index)
{
	if (!MenuButtons.IsValidIndex(Index) || !MenuButtons[Index].IsValid())
	{
		return;
	}
	FocusedIndex = Index;
	FSlateApplication::Get().SetKeyboardFocus(MenuButtons[Index], EFocusCause::SetDirectly);
}

void SGTCFrontEnd::ActivateFocusedButton()
{
	if (MenuActions.IsValidIndex(FocusedIndex))
	{
		MenuActions[FocusedIndex].ExecuteIfBound();
	}
}

FReply SGTCFrontEnd::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Visual == EGTCFrontEndVisual::Menu)
	{
		if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::W)
		{
			FocusButton((FocusedIndex + MenuButtons.Num() - 1) % MenuButtons.Num());
			return FReply::Handled();
		}
		if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::S)
		{
			FocusButton((FocusedIndex + 1) % MenuButtons.Num());
			return FReply::Handled();
		}
		if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
		{
			ActivateFocusedButton();
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	if (Visual == EGTCFrontEndVisual::Video)
	{
		// Any key skips the intro.
		OnSkip.ExecuteIfBound();
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SGTCFrontEnd::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (Visual == EGTCFrontEndVisual::Video)
	{
		OnSkip.ExecuteIfBound();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

EVisibility SGTCFrontEnd::GetMenuVisibility() const
{
	return Visual == EGTCFrontEndVisual::Menu ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SGTCFrontEnd::GetLoadingVisibility() const
{
	return Visual == EGTCFrontEndVisual::Loading ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SGTCFrontEnd::GetIntroVisibility() const
{
	return Visual == EGTCFrontEndVisual::Video ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}

TOptional<float> SGTCFrontEnd::GetIntroPercent() const
{
	return IntroProgress;
}

FText SGTCFrontEnd::GetIntroPromptText() const
{
	if (bSkipReady)
	{
		return NSLOCTEXT("GTC", "IntroReady", "PRESS ANY KEY TO CONTINUE");
	}
	return FText::FromString(FString::Printf(TEXT("LOADING  %d%%"), FMath::RoundToInt(IntroProgress * 100.0f)));
}

FLinearColor SGTCFrontEnd::GetFadeColor() const
{
	return FLinearColor(0.0f, 0.0f, 0.0f, FadeAlpha);
}

FLinearColor SGTCFrontEnd::GetScrimColor() const
{
	// Slightly stronger behind the menu, near-invisible during video.
	const float Alpha = (Visual == EGTCFrontEndVisual::Video) ? 0.0f : 0.55f;
	FLinearColor C = GTCFrontEndPalette::Scrim;
	C.A = Alpha;
	return C;
}

FLinearColor SGTCFrontEnd::GetAccentColor() const
{
	// Magenta neon rail, breathing in sync with the rest of the shell.
	return GTCFrontEndPalette::PulsingAccent();
}

TOptional<float> SGTCFrontEnd::GetLoadingPercent() const
{
	return LoadingProgress;
}

FText SGTCFrontEnd::GetLoadingText() const
{
	return FText::FromString(FString::Printf(TEXT("LOADING %d%%"), FMath::RoundToInt(LoadingProgress * 100.0f)));
}
