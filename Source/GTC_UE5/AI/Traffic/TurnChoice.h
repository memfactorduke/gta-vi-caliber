// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * What a car does at a junction: pick the next lane and know whether it's going
 * straight, turning, or making a U-turn. FRoadRoute gives the planned node path
 * and FLanePath rides one segment; FTurnChoice is the decision at the node where
 * one lane ends — follow the route if there is one, otherwise continue the most
 * natural way (straightest, no needless U-turn) for a free-roaming car.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject
 * — unit-tested headless (Tests/TurnChoiceTest.cpp, prefix GTC.AI.Traffic.Turn).
 * Frame: the repo's planar XZ convention; the signed-turn sign matches
 * FGpsNavigation (positive = Left, Y-up).
 *
 * PURE-CORE boundary: gathering the live outgoing lanes at a node and driving the
 * turn signal / steering is the DEFERRED adapter and is NOT covered by the tests.
 */
struct GTC_UE5_API FTurnChoice
{
    /** The maneuver class at a junction, for signalling / speed / animation. */
    enum class ETurn : uint8
    {
        Straight,
        Left,
        Right,
        UTurn,
    };

    /** |angle| at/under this (rad) reads as going straight. Default ~20°. */
    static constexpr double StraightThresholdRad = 0.3490658503988659;
    /** |angle| at/over this (rad) reads as a U-turn. Default ~150°. */
    static constexpr double UTurnThresholdRad = 2.6179938779914944;

    /**
     * Signed turn from Incoming to Outgoing in the XZ plane, radians in [-pi, pi].
     * Positive is Left, negative Right (matching FGpsNavigation). 0 for a
     * degenerate heading.
     */
    static double SignedTurn(const FVector& Incoming, const FVector& Outgoing);

    /** Classify the maneuver from the incoming + outgoing headings. */
    static ETurn Classify(
        const FVector& Incoming, const FVector& Outgoing,
        double StraightThreshold = StraightThresholdRad, double UTurnThreshold = UTurnThresholdRad);

    /**
     * Index into OutgoingNodeIds of the lane that continues the planned route (the
     * one leading to DesiredNextNode), or -1 if the route doesn't continue from
     * here. First match wins.
     */
    static int32 ChooseByRoute(const TArray<int32>& OutgoingNodeIds, int32 DesiredNextNode);

    /**
     * Free-roam fallback: the outgoing whose heading best continues the incoming
     * one (largest forward dot). When bAllowUTurn is false, near-reversals (turn
     * angle >= UTurnThreshold) are excluded so a car doesn't spin around on open
     * road. -1 if there is no acceptable continuation. Ties keep the lower index.
     */
    static int32 ChooseStraightest(
        const FVector& Incoming, const TArray<FVector>& OutgoingHeadings,
        bool bAllowUTurn = false, double UTurnThreshold = UTurnThresholdRad);
};
