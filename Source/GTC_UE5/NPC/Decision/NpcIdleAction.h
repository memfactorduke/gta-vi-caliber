// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * What a citizen actually DOES while it lingers somewhere — the small human
 * business that fills the gap between "walking to a place" and "standing frozen
 * at it". A loiterer checks their phone, people-watches, lights a cigarette,
 * stretches; someone eating sips and savours; a commuter checks the time and
 * sighs. Picking a contextual fidget per arrival is what turns a knot of idle
 * pawns into a street of people each doing their own thing.
 *
 * PURE-CORE: scene-free, deterministic FName selection by activity + seed (Godot
 * posmod wrap, mirroring FBarkPool), no engine coupling. Unit-tested headless
 * (Tests/NpcIdleActionTest.cpp, prefix GTC.NPC.Decision.NpcIdleAction). The action
 * is an FName token the AGTCCitizen adapter hands to an Anim Blueprint (montage /
 * additive) via OnIdleAction — so the gameplay choice is testable here and the
 * animation is data, exactly like the contact-reaction montage seam.
 */
struct GTC_UE5_API FNpcIdleAction
{
    /**
     * A contextual idle action for the given Activity (the FNpcIntent activity:
     * "loiter"/"socialize"/"eat"/"commute"/"goof_off"/"freshen_up"/"work"/...),
     * chosen deterministically from Seed (posmod-wrapped). Unknown/empty activities
     * fall back to a generic bank, then to NAME_None only if even that is empty.
     */
    static FName Pick(const FString& Activity, int32 Seed);

    /** Number of distinct idle actions for an activity (generic-fallback count for
     *  an unknown activity). */
    static int32 Count(const FString& Activity);
};
