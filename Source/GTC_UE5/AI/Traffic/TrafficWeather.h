// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Weather caution for ambient traffic — the other half of the "feed wetness into
 * vehicle grip / traffic speed" TODO (FRoadGrip did the player's grip; this does
 * the AI traffic's speed). FTrafficAgent drives an IDM whose two free parameters
 * are DesiredSpeed (v0, the free-road cruise) and TimeHeadway (T, the gap it keeps
 * to the car ahead). On a clear day those are fixed, so the city's cars barrel
 * through a downpour exactly as they do in sun. FTrafficWeather is the producer
 * that makes the traffic drive scared in bad weather: it turns how wet the road is
 * and how far you can see into a SPEED factor (cruise slower) and a HEADWAY factor
 * (hang back further), which the adapter folds into each agent's IDM params.
 *
 * Both rain and fog slow traffic and stretch the gaps, and they compound — a foggy
 * downpour is worse than either alone. Speed bleeds off by WetSpeedLoss per unit of
 * wetness and FogSpeedLoss per unit of lost visibility, floored at MinSpeedFactor so
 * the city never grinds to a crawl. The headway gain is derived from that SAME
 * severity, so the storm that most slows the cars also most widens their gaps — one
 * coherent dial, not two that can disagree. AdjustedDesiredSpeed / AdjustedTimeHeadway
 * apply the factors straight onto an agent's base v0 / T.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject,
 * all static (like FRoadGrip) — unit-tested headless (Tests/TrafficWeatherTest.cpp,
 * prefix GTC.AI.Traffic.Weather).
 *
 * PURE-CORE boundary: sourcing wetness/visibility from the weather system and
 * folding these factors into each FTrafficAgent's IDM params each tick is the
 * DEFERRED adapter and is NOT covered by the tests. Units: Wetness and Visibility
 * are 0..1 (Visibility 1 = clear, 0 = whiteout); the returned factors are
 * dimensionless multipliers.
 */
struct FTrafficWeather
{
    /** Tuning for how cautious traffic gets. Defaults: ~25% slower in rain, ~35% in fog. */
    struct FParams
    {
        /** Fraction of cruise speed shed at full wetness. */
        double WetSpeedLoss = 0.25;
        /** Fraction of cruise speed shed at zero visibility. */
        double FogSpeedLoss = 0.35;
        /** Floor on the speed factor — traffic never crawls slower than this fraction. */
        double MinSpeedFactor = 0.55;
        /** Time-headway multiplier at the worst conditions (1 = no change). */
        double MaxHeadwayGain = 1.8;
    };

    /**
     * Cruise-speed multiplier (0..1) for the given Wetness and Visibility (both
     * clamped to [0,1]; Visibility 1 = clear). 1.0 in clear, dry weather; falls with
     * rain and fog (which compound) down to MinSpeedFactor.
     */
    static double SpeedFactor(double Wetness, double Visibility, const FParams& Params);

    /**
     * Time-headway multiplier (>= 1) for the given conditions — 1 in the clear,
     * rising to MaxHeadwayGain at the worst, tracking the same severity that slows
     * the traffic.
     */
    static double HeadwayFactor(double Wetness, double Visibility, const FParams& Params);

    /** An agent's base free-road cruise (v0) scaled by SpeedFactor. */
    static double AdjustedDesiredSpeed(double BaseDesiredSpeed, double Wetness, double Visibility, const FParams& Params);

    /** An agent's base time gap (T) scaled by HeadwayFactor. */
    static double AdjustedTimeHeadway(double BaseTimeHeadway, double Wetness, double Visibility, const FParams& Params);
};
