// Copyright Epic Games, Inc. All Rights Reserved.

#include "PoliceEscalation.h"

// Out-of-line storage for the response-composition table, ported VERBATIM from the
// Godot RESPONSE_BY_STAR constant (game/scripts/ai/police_escalation.gd). Index =
// clamped stars (0..MaxStars). Element counts set only the *mix*; absolute head-
// count lives in FPoliceResponse, not here. Ordered (order is observable: tests
// index units[0] and the returned array is a copy a caller may mutate).
namespace
{
    using EUT = EPoliceUnitType;

    const TArray<EPoliceUnitType> GtcPoliceResponseByStar[FPoliceEscalation::MaxStars + 1] = {
        {},                                                                          // 0 stars -- clean, no response.
        {EUT::BeatCop},                                                              // 1 star  -- a lone beat cop on foot.
        {EUT::BeatCop, EUT::Cruiser, EUT::Cruiser},                                  // 2 stars -- cruisers roll in.
        {EUT::Cruiser, EUT::Cruiser, EUT::Cruiser, EUT::Swat},                       // 3 stars -- more cruisers + first SWAT.
        {EUT::Cruiser, EUT::Cruiser, EUT::Swat, EUT::Swat, EUT::Helicopter},         // 4 stars -- SWAT + chopper overhead.
        {EUT::Cruiser, EUT::Swat, EUT::Swat, EUT::Swat, EUT::Helicopter},            // 5 stars -- heavier SWAT saturation.
        {EUT::Swat, EUT::Swat, EUT::Helicopter, EUT::Helicopter, EUT::Military, EUT::Military}, // 6 stars -- the army.
    };
}

TArray<EPoliceUnitType> FPoliceEscalation::ResponseUnits(int32 Stars)
{
    // `duplicate()` parity: return a fresh copy the caller may freely mutate.
    return GtcPoliceResponseByStar[ClampStars(Stars)];
}
