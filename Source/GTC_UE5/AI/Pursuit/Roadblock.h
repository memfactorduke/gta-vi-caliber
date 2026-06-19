// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Police roadblocks — the wall of cruisers strung across the road ahead of a fleeing
 * player at higher heat. FPoliceEscalation::RoadblockChance(stars) already decides
 * WHETHER one gets set up; this is the missing other half — WHERE, and HOW the units line
 * up across the carriageway — that the dispatch adapter uses once that roll succeeds.
 *
 * The chase logic that matters: the block has to be placed far enough AHEAD that the
 * cruisers have time to arrive and deploy before the player reaches it (so it scales with
 * the player's speed), spread across the road's width, and — below a saturating unit count
 * — it leaves a squeeze-through gap, so a roadblock is an obstacle to thread or ram, not an
 * instant wall. FPursuitTactics::BlockOffset positions ONE interceptor; this lays out the
 * whole coordinated line.
 *
 * PURE-CORE: scene-free, deterministic, double precision, FVector-in / FVector-out, no
 * engine coupling, no RNG. The dispatch adapter supplies the player position + flee
 * direction (unit, ground-plane) and the road's right-vector + width from the road
 * network, spawns a cruiser at each returned point, and checks HasPassed to retire the
 * block once the player is through. Units are the adapter's own (UE cm); +Z up. Unit-
 * tested headless (AI/Pursuit/Tests/RoadblockTest.cpp, prefix GTC.AI.Pursuit.Roadblock).
 */
struct GTC_UE5_API FRoadblock
{
    /** Seconds of lead the cruisers need to arrive and deploy before the player gets there. */
    static constexpr double DefaultSetupLeadSeconds = 2.5;
    /** Never set the block up closer than this (cm), however slow the player is crawling. */
    static constexpr double MinLeadDistance = 1500.0;
    /** Road span a single cruiser + officer plugs (cm). */
    static constexpr double UnitWidth = 280.0;

    /**
     * How far ahead (cm) to place the block: PlayerSpeed * SetupLead, floored at
     * MinLeadDistance so a slow player can't be walled in on the spot.
     */
    static double LeadDistance(double PlayerSpeed, double SetupLeadSeconds = DefaultSetupLeadSeconds);

    /** The block's centre point: ahead of the player along the (normalised) flee direction. */
    static FVector BlockadeCenter(const FVector& PlayerPos, const FVector& FleeDir, double Lead);

    /** Units needed to plug a road of this width (ceil(width / UnitWidth), at least 1). */
    static int32 UnitsToSpan(double RoadWidth);

    /** True once UnitCount covers the whole width — no squeeze-through gap remains. */
    static bool FullyBlocks(double RoadWidth, int32 UnitCount);

    /**
     * Positions for `UnitCount` cruisers spread evenly across the road about Center along
     * the (normalised) RoadRight: a lone unit sits at the centre; multiple units span edge
     * to edge. Empty for UnitCount <= 0 or a degenerate RoadRight.
     */
    static TArray<FVector> UnitPositions(const FVector& Center, const FVector& RoadRight,
        double RoadWidth, int32 UnitCount);

    /** True once the player has driven past the blockade line (beyond Center along FleeDir). */
    static bool HasPassed(const FVector& PlayerPos, const FVector& Center, const FVector& FleeDir);
};
