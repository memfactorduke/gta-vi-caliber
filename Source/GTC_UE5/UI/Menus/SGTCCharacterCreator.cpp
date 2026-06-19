// Copyright Epic Games, Inc. All Rights Reserved.

#include "SGTCCharacterCreator.h"

#include "../../Player/Pawn/GTCPlayerLook.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "GTCCharacterCreator"

void SGTCCharacterCreator::Construct(const FArguments& InArgs)
{
    Services = InArgs._Services;

    TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
    for (int32 Slot = 0; Slot < GTCLookSlot::Count; ++Slot)
    {
        Rows->AddSlot().AutoHeight().Padding(0.f, 5.f)
        [
            MakeSlotRow(Slot)
        ];
    }

    ChildSlot
    [
        // Dim the world behind the panel (same treatment as the pause menu).
        SNew(SBorder)
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.6f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            [
                SNew(SBorder)
                .Padding(FMargin(44.f, 34.f))
                .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FLinearColor(0.05f, 0.06f, 0.08f, 0.95f))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 22.f).HAlign(HAlign_Center)
                    [
                        SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 28))
                        .Text(LOCTEXT("Title", "Create Your Character"))
                    ]
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        Rows
                    ]
                    // Randomize + Done.
                    + SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().FillWidth(1.f).Padding(4.f, 0.f)
                        [
                            SNew(SButton)
                            .HAlign(HAlign_Center).VAlign(VAlign_Center)
                            .ContentPadding(FMargin(20.f, 12.f))
                            .OnClicked_Lambda([this]()
                            {
                                if (Services.Randomize) { Services.Randomize(); }
                                return FReply::Handled();
                            })
                            [
                                SNew(STextBlock)
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
                                .Text(LOCTEXT("Randomize", "Randomize"))
                            ]
                        ]
                        + SHorizontalBox::Slot().FillWidth(1.f).Padding(4.f, 0.f)
                        [
                            SNew(SButton)
                            .HAlign(HAlign_Center).VAlign(VAlign_Center)
                            .ContentPadding(FMargin(20.f, 12.f))
                            .OnClicked_Lambda([this]()
                            {
                                if (Services.Confirm) { Services.Confirm(); }
                                return FReply::Handled();
                            })
                            [
                                SNew(STextBlock)
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
                                .Text(LOCTEXT("Done", "Done"))
                            ]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f).HAlign(HAlign_Center)
                    [
                        SNew(STextBlock)
                        .ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.f))
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
                        .Text(LOCTEXT("Hint", "Enter to confirm  ·  Esc to close"))
                    ]
                ]
            ]
        ]
    ];
}

TSharedRef<SWidget> SGTCCharacterCreator::MakeSlotRow(int32 Slot)
{
    return SNew(SHorizontalBox)
        // ‹ previous
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(SButton)
            .HAlign(HAlign_Center).VAlign(VAlign_Center)
            .ContentPadding(FMargin(16.f, 8.f))
            .OnClicked_Lambda([this, Slot]()
            {
                if (Services.Cycle) { Services.Cycle(Slot, -1); }
                return FReply::Handled();
            })
            [
                SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 18)).Text(LOCTEXT("Prev", "‹"))
            ]
        ]
        // live label (Name + value / count), reads back from the pawn each paint
        + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).HAlign(HAlign_Center).Padding(16.f, 0.f)
        [
            SNew(STextBlock)
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
            .MinDesiredWidth(260.f)
            .Justification(ETextJustify::Center)
            .Text_Lambda([this, Slot]() -> FText
            {
                return Services.Label ? Services.Label(Slot) : FText::GetEmpty();
            })
        ]
        // › next
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(SButton)
            .HAlign(HAlign_Center).VAlign(VAlign_Center)
            .ContentPadding(FMargin(16.f, 8.f))
            .OnClicked_Lambda([this, Slot]()
            {
                if (Services.Cycle) { Services.Cycle(Slot, +1); }
                return FReply::Handled();
            })
            [
                SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 18)).Text(LOCTEXT("Next", "›"))
            ]
        ];
}

FReply SGTCCharacterCreator::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();
    if (Key == EKeys::Enter || Key == EKeys::Escape || Key == EKeys::Virtual_Accept)
    {
        if (Services.Confirm) { Services.Confirm(); }
        return FReply::Handled();
    }
    return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
