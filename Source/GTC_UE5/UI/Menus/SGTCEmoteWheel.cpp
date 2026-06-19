// Copyright Epic Games, Inc. All Rights Reserved.

#include "SGTCEmoteWheel.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "GTCEmoteWheel"

void SGTCEmoteWheel::Construct(const FArguments& InArgs)
{
    Services = InArgs._Services;

    TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
    List->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 18.f).HAlign(HAlign_Center)
    [
        SNew(STextBlock)
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 26))
        .Text(LOCTEXT("Title", "Emotes"))
    ];

    for (int32 i = 0; i < Services.Names.Num(); ++i)
    {
        List->AddSlot().AutoHeight().Padding(0.f, 5.f)
        [
            MakeEmoteButton(i)
        ];
    }

    List->AddSlot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f).HAlign(HAlign_Center)
    [
        SNew(STextBlock)
        .Font(FCoreStyle::GetDefaultFontStyle("Italic", 12))
        .ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
        .Text(LOCTEXT("Hint", "Press a number to play  ·  B / Esc to close"))
    ];

    ChildSlot
    [
        // Dim the world behind the panel (same look as the pause menu).
        SNew(SBorder)
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.5f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
            [
                SNew(SBorder)
                .Padding(FMargin(40.f, 32.f))
                .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FLinearColor(0.05f, 0.06f, 0.08f, 0.95f))
                [
                    List
                ]
            ]
        ]
    ];
}

TSharedRef<SWidget> SGTCEmoteWheel::MakeEmoteButton(int32 Index)
{
    const FText Label = FText::Format(
        LOCTEXT("EmoteRow", "{0}   {1}"), FText::AsNumber(Index + 1), Services.Names[Index]);

    return SNew(SButton)
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        .ContentPadding(FMargin(28.f, 12.f))
        .OnClicked(FOnClicked::CreateLambda([this, Index]()
        {
            Pick(Index);
            return FReply::Handled();
        }))
        [
            SNew(STextBlock)
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
            .Text(Label)
        ];
}

void SGTCEmoteWheel::Pick(int32 Index)
{
    if (Services.Names.IsValidIndex(Index) && Services.Play)
    {
        Services.Play(Index);
    }
    if (Services.Close)
    {
        Services.Close();
    }
}

FReply SGTCEmoteWheel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();

    if (Key == EKeys::Escape || Key == EKeys::B)
    {
        if (Services.Close)
        {
            Services.Close();
        }
        return FReply::Handled();
    }

    // Number-row 1..9 picks the matching emote (1 = first).
    static const FKey NumKeys[9] = {
        EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
        EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine
    };
    for (int32 i = 0; i < 9; ++i)
    {
        if (Key == NumKeys[i] && Services.Names.IsValidIndex(i))
        {
            Pick(i);
            return FReply::Handled();
        }
    }

    return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
