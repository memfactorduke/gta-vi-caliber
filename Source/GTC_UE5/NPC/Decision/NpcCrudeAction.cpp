// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcCrudeAction.h"

namespace
{
    // Deterministic roll in [0,1000) from a seed — an FNV-1a avalanche, no RNG object,
    // so the same context always yields the same outcome (headless-testable).
    int32 GtcCrudeRoll(int32 Seed)
    {
        uint32 H = 2166136261u ^ static_cast<uint32>(Seed);
        H *= 16777619u; H ^= H >> 15; H *= 2246822519u; H ^= H >> 13;
        return static_cast<int32>(H % 1000u);
    }
}

bool FNpcCrudeAction::IsNight(double Hour)
{
    // Late evening through the small hours: the cover of dark.
    return Hour >= 21.0 || Hour < 5.0;
}

FName FNpcCrudeAction::Pick(double Hour, FName PlaceKind, uint8 Busyness, ENpcTemper Temper, int32 Seed)
{
    // NOTE: "relieve_self" (pee) and "vomit" are disabled for now (removed on request).
    // Their context gate (IsNight / SecludedAtOrBelow, the Hour/PlaceKind/Busyness inputs)
    // is kept so they can be restored by re-adding the two branches below.
    const int32 Roll = GtcCrudeRoll(Seed);
    const bool bRude = FNpcAttitude::IsRude(Temper);

    // --- spit: a low-key hawk, any time of day; the rude do it more. -----------
    const int32 Chance = bRude ? 60 : 25;
    if (Roll < Chance)
    {
        return TEXT("spit");
    }

    return NAME_None;
}
