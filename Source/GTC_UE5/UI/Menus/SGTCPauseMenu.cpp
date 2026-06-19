#include "SGTCPauseMenu.h"

#include "../../FrontEnd/GTCFrontEndPalette.h"

#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Colors/SColorBlock.h"

#define LOCTEXT_NAMESPACE "GTCPauseMenu"

void SGTCPauseMenu::Construct(const FArguments& InArgs)
{
	OnResume = InArgs._OnResume;
	OnFirstPerson = InArgs._OnFirstPerson;
	OnThirdPerson = InArgs._OnThirdPerson;
	OnMissionEditor = InArgs._OnMissionEditor;
	OnEditCharacter = InArgs._OnEditCharacter;

	const FSlateFontInfo KickerFont = FCoreStyle::GetDefaultFontStyle("Bold", 13);
	const FSlateFontInfo FooterFont = FCoreStyle::GetDefaultFontStyle("Regular", 14);

	TSharedRef<SVerticalBox> Column = SNew(SVerticalBox);

	// Resume is always available; the others appear only when the owner binds them
	// (so the column never shows a dead row).
	AddAction(&Column.Get(), LOCTEXT("Resume", "RESUME"), OnResume);
	AddAction(&Column.Get(), LOCTEXT("EditCharacter", "EDIT CHARACTER"), OnEditCharacter);
	AddAction(&Column.Get(), LOCTEXT("ThirdPerson", "CAMERA VIEW"), OnThirdPerson);
	AddAction(&Column.Get(), LOCTEXT("FirstPerson", "CHARACTER VIEW"), OnFirstPerson);
	AddAction(&Column.Get(), LOCTEXT("MissionEditor", "MISSION EDITOR"), OnMissionEditor);

	ChildSlot
	[
		SNew(SOverlay)

		// 1. Dim + cool the world behind the menu.
		+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Fill)
		[
			SNew(SColorBlock).Color(FLinearColor(0.0f, 0.006f, 0.022f, 0.74f))
		]

		// 2. Left-anchored neon column (kicker + accent rail + actions).
		+ SOverlay::Slot()
		.HAlign(HAlign_Left).VAlign(VAlign_Center)
		.Padding(96.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Font(KickerFont)
				.ColorAndOpacity(GTCFrontEndPalette::Kicker)
				.Text(LOCTEXT("Paused", "PAUSED"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 22.0f)
			[
				SNew(SBox).WidthOverride(172.0f).HeightOverride(3.0f)
				[
					SNew(SColorBlock).Color(this, &SGTCPauseMenu::GetAccentColor)
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Column
			]
		]

		// 3. Footer hint.
		+ SOverlay::Slot()
		.HAlign(HAlign_Left).VAlign(VAlign_Bottom)
		.Padding(96.0f, 0.0f, 0.0f, 44.0f)
		[
			SNew(STextBlock)
			.Font(FooterFont)
			.ColorAndOpacity(GTCFrontEndPalette::Footer)
			.Text(LOCTEXT("Footer", "ENTER  SELECT      ESC  RESUME"))
		]
	];

	FocusButton(0);
}

void SGTCPauseMenu::AddAction(SVerticalBox* Column, const FText& Label, const FSimpleDelegate& Action)
{
	if (!Action.IsBound() || Column == nullptr)
	{
		return;
	}

	const int32 Index = MenuButtons.Num();
	const FSlateFontInfo ButtonFont = FCoreStyle::GetDefaultFontStyle("Bold", 32);

	TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
		.ContentPadding(FMargin(0.0f, 4.0f))
		.HAlign(HAlign_Left)
		.OnHovered_Lambda([this, Index]() { FocusButton(Index); })
		.OnClicked_Lambda([Action]() -> FReply
		{
			Action.ExecuteIfBound();
			return FReply::Handled();
		})
		[
			SNew(SHorizontalBox)
			// Pulsing magenta caret marks the focused row.
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 16.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(6.0f).HeightOverride(26.0f)
				[
					SNew(SColorBlock).Color_Lambda([this, Index]() -> FLinearColor
					{
						return Index == FocusedIndex
							? GTCFrontEndPalette::PulsingAccent()
							: FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
					})
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(ButtonFont)
				.Text(Label)
				.ColorAndOpacity_Lambda([this, Index]() -> FSlateColor
				{
					return FSlateColor(Index == FocusedIndex
						? GTCFrontEndPalette::TextBright
						: GTCFrontEndPalette::TextDim);
				})
			]
		];

	Column->AddSlot().AutoHeight().Padding(0.0f, 6.0f)[ Button ];

	MenuButtons.Add(Button);
	MenuActions.Add(Action);
}

void SGTCPauseMenu::FocusButton(int32 Index)
{
	if (!MenuButtons.IsValidIndex(Index) || !MenuButtons[Index].IsValid())
	{
		return;
	}
	FocusedIndex = Index;
	FSlateApplication::Get().SetKeyboardFocus(MenuButtons[Index], EFocusCause::SetDirectly);
}

void SGTCPauseMenu::ActivateFocused()
{
	if (MenuActions.IsValidIndex(FocusedIndex))
	{
		MenuActions[FocusedIndex].ExecuteIfBound();
	}
}

FLinearColor SGTCPauseMenu::GetAccentColor() const
{
	return GTCFrontEndPalette::PulsingAccent();
}

FReply SGTCPauseMenu::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	const int32 Count = MenuButtons.Num();

	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		OnResume.ExecuteIfBound();
		return FReply::Handled();
	}
	if (Count > 0 && (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::W))
	{
		FocusButton((FocusedIndex + Count - 1) % Count);
		return FReply::Handled();
	}
	if (Count > 0 && (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::S))
	{
		FocusButton((FocusedIndex + 1) % Count);
		return FReply::Handled();
	}
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		ActivateFocused();
		return FReply::Handled();
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
