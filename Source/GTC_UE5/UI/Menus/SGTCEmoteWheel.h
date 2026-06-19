// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

/**
 * Live bridge from the emote panel to the possessed player pawn. The controller
 * fills these in with lambdas that re-resolve the pawn each call (so the panel
 * survives a respawn), mirroring FGTCCreatorServices / FGTCPhoneServices.
 */
struct FGTCEmoteServices
{
    /** Emote display names, in order (index = the id passed back to Play). */
    TArray<FText> Names;

    /** Play the emote at the given index on the pawn. */
    TFunction<void(int32 /*Index*/)> Play;

    /** Close the panel (also called after a pick). */
    TFunction<void()> Close;
};

/**
 * In-game emote panel: ONE key (B) opens it, then a click — or the matching number
 * key — plays an emote and closes. Replaces the per-emote hard keys (H/G/J) with a
 * single, extensible picker. Minimal Slate, matching SGTCPauseMenu /
 * SGTCCharacterCreator. The pawn owns the clips; this widget only fires callbacks.
 */
class GTC_UE5_API SGTCEmoteWheel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SGTCEmoteWheel) {}
        SLATE_ARGUMENT(FGTCEmoteServices, Services)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // Focusable so the number keys / Esc land while the panel is up.
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
    FGTCEmoteServices Services;

    /** Play emote Index and close the panel. */
    void Pick(int32 Index);

    /** Build one "1   Wave" pick button for the emote at Index. */
    TSharedRef<class SWidget> MakeEmoteButton(int32 Index);
};
