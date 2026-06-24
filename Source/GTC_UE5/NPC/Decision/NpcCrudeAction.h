// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NpcAttitude.h"

/**
 * The crude, very-human business a citizen occasionally does — currently just a
 * surly type hawking a spit on the pavement. This is the R-rated end of
 * FNpcIdleAction (a contextual action token handed to the anim layer via
 * OnIdleAction), gated HARD on context so it reads as situational colour.
 *
 * NOTE: the heavier acts — relieving themselves down a quiet alley at night, a
 * drunk retching outside a bar at closing time — are DISABLED for now (removed on
 * request). Their context gate (IsNight / SecludedAtOrBelow + the Hour/PlaceKind/
 * Busyness inputs) is kept so they can be restored by re-adding two branches to Pick.
 *
 * The gate is the whole point: nobody pees in a packed daytime plaza, and only the
 * drunk-at-a-bar-at-2am retch. The adapter consults this only after its own rare
 * roll (CrudeActionChance) and a master toggle (bAllowCrudeActions), so the final
 * frequency is low by construction. "Drunk" is derived from being at a bar late at
 * night (the project has no intoxication state), which is the testable equivalent.
 *
 * PURE-CORE: scene-free, deterministic selection from context + seed (no engine
 * coupling, no RNG object), unit-tested headless (Tests/NpcCrudeActionTest.cpp,
 * prefix GTC.NPC.Decision.NpcCrudeAction). GTC-original. Returns NAME_None — the
 * common case — when the context doesn't warrant anything crude.
 */
struct GTC_UE5_API FNpcCrudeAction
{
    /** Busyness tier below which a spot counts as "secluded enough" (matches
     *  ENpcCrowding: 0 Empty, 1 Quiet, 2 Busy, 3 Packed). Quiet or emptier. */
    static constexpr uint8 SecludedAtOrBelow = 1; // ENpcBusyness::Quiet

    /** True for the small-hours / late-evening window crude acts hide in. */
    static bool IsNight(double Hour);

    /**
     * A crude action token for the given context, or NAME_None (the usual answer).
     * `Hour` is 0..24 game time; `PlaceKind` is the claimed POI kind ("bar"/"park"/
     * "home"/"street"/...); `Busyness` is the ENpcBusyness tier (0..3) of that spot;
     * `Temper` nudges likelihood (the rude are less shy); `Seed` makes it deterministic.
     *
     * Rules:
     *   - "spit"    : a low-key, anytime hawk; the rude do it more.
     *   - NAME_None : otherwise.
     * (The disabled "vomit" / "relieve_self" branches would key off Hour/PlaceKind/
     *  Busyness — which is why those inputs are still threaded through.)
     */
    static FName Pick(double Hour, FName PlaceKind, uint8 Busyness, ENpcTemper Temper, int32 Seed);
};
