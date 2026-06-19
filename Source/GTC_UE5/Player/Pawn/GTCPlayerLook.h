// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GTCPlayerLook.generated.h"

/**
 * The player's chosen appearance, as a tiny set of pool indices into the
 * UGTCAppearanceSet wardrobe (the same pools the NPC crowd draws from). Kept as
 * indices (not hard mesh refs) so it serialises trivially and survives level
 * travel when stashed on the GameInstance — the character the player builds in
 * the creator follows them onto every map.
 *
 * Slot order matches GTCLookSlot below; keep the two in lockstep.
 */
USTRUCT()
struct FGTCPlayerLook
{
    GENERATED_BODY()

    /** Body skeletal mesh index (UGTCAppearanceSet::BodyMeshes, wraps). */
    UPROPERTY()
    int32 Body = 0;

    /** Face/head mesh index (HeroHeadMeshes preferred, else HeadMeshes, wraps). */
    UPROPERTY()
    int32 Face = 0;

    /** Hair mesh index (HairMeshes, wraps). */
    UPROPERTY()
    int32 Hair = 0;

    /** Outfit mesh index (OutfitMeshes, wraps). */
    UPROPERTY()
    int32 Outfit = 0;

    /** Skin tone index (SkinTones, or the built-in fallback palette, wraps). */
    UPROPERTY()
    int32 Skin = 0;

    /** True once the player has confirmed a look — gates restore-on-spawn. */
    UPROPERTY()
    bool bValid = false;
};

/**
 * Appearance slot ids shared by the pawn (CycleAppearanceSlot / GetAppearanceSlotText)
 * and the creator widget. The widget iterates [0, Count) so it stays agnostic to
 * the individual names; the labels come from the pawn.
 */
namespace GTCLookSlot
{
    enum Type
    {
        Body = 0,
        Face = 1,
        Hair = 2,
        Outfit = 3,
        Skin = 4,
        Count = 5
    };
}
