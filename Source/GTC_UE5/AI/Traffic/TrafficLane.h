// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Car-follow spacing on a single lane — the "queue, don't overlap" half of the
 * M4 traffic sim. FTrafficModel supplies the IDM longitudinal acceleration GIVEN
 * a gap to the car ahead and that leader's speed; the missing piece was deciding
 * WHICH car is the leader and how big the bumper-to-bumper gap is. FTrafficLane
 * answers exactly that from the agents' arc-length positions along the lane
 * (FLanePath's `s`), so a line of cars settles into a spaced queue at a junction
 * instead of stacking on one point.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject
 * — unit-tested headless (Tests/TrafficLaneTest.cpp, prefix GTC.AI.Traffic.Lane).
 *
 * One lane, one direction of travel: agents move in the direction of INCREASING
 * `s`, so a car's leader is the nearest agent ahead of it (greater `s`). `s` is
 * the agent's CENTRE position; bodies are `Length` long, so the bumper gap nets
 * out half of each car's length.
 *
 * PURE-CORE boundary: feeding the gap + leader speed into FTrafficModel and
 * integrating speed/position onto a Chaos vehicle each tick is the DEFERRED
 * adapter — the core only does the spatial selection and is NOT coupled to
 * FTrafficModel here (the test demonstrates the pairing).
 */
struct GTC_UE5_API FTrafficLane
{
    /** A typical car length (m); the default so callers can omit it. */
    static constexpr double DefaultVehicleLength = 4.5;

    /** One vehicle on the lane: centre arc-length `S`, current `Speed`, body `Length`. */
    struct FAgent
    {
        double S = 0.0;
        double Speed = 0.0;
        double Length = DefaultVehicleLength;
    };

    /**
     * Index of `Self`'s leader: the nearest agent strictly ahead (greater `S`).
     * -1 when `Self` is at the front of the queue (open road) or the index is out
     * of range. Deterministic: on an exact distance tie the lower index wins.
     */
    static int32 LeaderIndex(const TArray<FAgent>& Agents, int32 Self);

    /**
     * Bumper-to-bumper gap from `Follower` to `Leader` (assumed ahead): the centre
     * spacing minus half of each body. NEGATIVE when the bodies overlap — the
     * signal that two cars have been placed/integrated into each other and one
     * must brake hard (FTrafficModel treats a non-positive gap as touching).
     */
    static double BumperGap(const FAgent& Follower, const FAgent& Leader);

    /** Do the two bodies overlap on the lane (centre spacing < half their lengths)? */
    static bool Overlaps(const FAgent& A, const FAgent& B);
};
