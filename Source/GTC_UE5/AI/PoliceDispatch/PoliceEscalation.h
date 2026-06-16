// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure response-composition model for the wanted heat ramp: which KINDS of units
 * a star level summons (beat cops -> cruisers -> SWAT -> helicopter -> military),
 * how hard they press, how often waves reinforce, and what they shoot with.
 *
 * Direct 1:1 port of the Godot RefCounted `PoliceEscalation`
 * (game/scripts/ai/police_escalation.gd). All-static plain functions, no
 * UObject, no scene/RNG state -> the curve is deterministic and unit-tested
 * (Tests/PoliceEscalationTest.cpp, prefix GTC.AI.PoliceDispatch.PoliceEscalation).
 *
 * This is the unit-TYPE layer, distinct from its neighbours: FPoliceResponse maps
 * stars->head-count + spawn radius, and FPoliceDispatch maps that count->ring
 * spawn positions/recalls. FPoliceEscalation answers "what responds" and never
 * how many or where. Stars are clamped to [0, MaxStars] and every curve is
 * monotonic in stars (higher heat is never a weaker response).
 *
 * Double precision throughout (LWC) to match the Godot oracle's float arithmetic.
 *
 * PURE-CORE boundary: this is the entire tested unit. A spawner/AI layer reading
 * ResponseUnits() to instance prefabs is the DEFERRED Wave-3 adapter -- not
 * implemented here and not covered by the parity tests.
 */

/**
 * Unit-type identifiers, ordered weakest -> heaviest. Returned by ResponseUnits()
 * as the composition of an active response; a spawner maps each to a prefab.
 * Godot ordinal order preserved (BeatCop=0 .. Military=4).
 */
enum class EPoliceUnitType : int32
{
    BeatCop = 0,
    Cruiser = 1,
    Swat = 2,
    Helicopter = 3,
    Military = 4,
};

class GTC_UE5_API FPoliceEscalation
{
public:
    /** Highest heat tier: 6 stars summons the military, the top of the ramp. */
    static constexpr int32 MaxStars = 6;

    /** Star at which each heavy asset first joins the response (and stays). */
    static constexpr int32 SwatStars = 3;
    static constexpr int32 HelicopterStars = 4;
    static constexpr int32 MilitaryStars = 6;

    /** Stars snapped into the valid [0, MaxStars] band. */
    static int32 ClampStars(int32 Stars)
    {
        return FMath::Clamp(Stars, 0, MaxStars);
    }

    /**
     * The mix of unit types active at this heat (a fresh copy, safe to mutate).
     * Empty at 0 stars; monotonic -- a higher star is never a weaker response.
     */
    static TArray<EPoliceUnitType> ResponseUnits(int32 Stars);

    /** Whether a SWAT unit is part of the response at this heat. */
    static bool HasSwat(int32 Stars)
    {
        return ClampStars(Stars) >= SwatStars;
    }

    /** Whether a pursuit helicopter is part of the response at this heat. */
    static bool HasHelicopter(int32 Stars)
    {
        return ClampStars(Stars) >= HelicopterStars;
    }

    /** Whether the military is part of the response at this heat (6 stars only). */
    static bool HasMilitary(int32 Stars)
    {
        return ClampStars(Stars) >= MilitaryStars;
    }

    /** 0.0 (arrest / shoot only if shot at) .. 1.0 (shoot-to-kill). Rises with stars. */
    static double Aggression(int32 Stars)
    {
        return FMath::Clamp(double(ClampStars(Stars)) / double(MaxStars), 0.0, 1.0);
    }

    /**
     * Seconds between reinforcement waves -- SHORTER at higher heat (more
     * relentless). 0 stars returns the slow baseline (nothing spawns at 0 anyway).
     */
    static double ReinforcementInterval(int32 Stars)
    {
        return FMath::Lerp(12.0, 3.0, Aggression(Stars));
    }

    /** 0.0 .. 1.0 chance a roadblock is set up -- rises with stars, none at 0. */
    static double RoadblockChance(int32 Stars)
    {
        return FMath::Lerp(0.0, 0.9, Aggression(Stars));
    }

    /**
     * Weapon tier the response carries: 0 none/melee, 1 pistols, 2 SMGs, 3 rifles,
     * 4 military-grade. Non-decreasing with stars.
     */
    static int32 WeaponTier(int32 Stars)
    {
        const int32 S = ClampStars(Stars);
        if (S <= 0)
        {
            return 0;
        }
        if (S <= 2)
        {
            return 1;
        }
        if (S <= 3)
        {
            return 2;
        }
        if (S <= 5)
        {
            return 3;
        }
        return 4;
    }
};
