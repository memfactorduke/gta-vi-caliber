// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

/**
 * Live bridge from the creator widget to the possessed player pawn. The controller
 * fills these in with lambdas that re-resolve the pawn each call (so the creator
 * survives a respawn), mirroring the FGTCPhoneServices pattern.
 */
struct FGTCCreatorServices
{
    /** Step a slot's pool index by Delta (-1 / +1); the pawn re-applies its look. */
    TFunction<void(int32 /*Slot*/, int32 /*Delta*/)> Cycle;

    /** Live display text for a slot, e.g. "Face   2 / 6" (drives the row label). */
    TFunction<FText(int32 /*Slot*/)> Label;

    /** Roll every slot to a random valid value. */
    TFunction<void()> Randomize;

    /** Commit the look (persist it) and close the creator. */
    TFunction<void()> Confirm;
};

/**
 * In-game character creator. Minimal Slate (matches SGTCPauseMenu / the C++-driven
 * front-end): one ‹ value › row per appearance slot (body / face / hair / outfit /
 * skin), plus Randomize and Done. The pawn owns all state and asset resolution;
 * this widget only fires the service callbacks and reads back the live labels.
 */
class GTC_UE5_API SGTCCharacterCreator : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SGTCCharacterCreator) {}
        SLATE_ARGUMENT(FGTCCreatorServices, Services)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // Focusable so Enter (confirm) / Esc (close) land while the game is paused.
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
    FGTCCreatorServices Services;

    /** Build one "Label   ‹  value  ›" row for the given appearance slot. */
    TSharedRef<class SWidget> MakeSlotRow(int32 Slot);
};
