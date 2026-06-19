// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Right-of-way arbitration at a junction — the rule that decides which waiting
 * car proceeds and which holds at the stop line, so traffic takes turns through
 * an intersection instead of piling in (ROADMAP M4). FTrafficModel already knows
 * how to brake for a leader; FIntersection supplies the two missing pieces: WHO
 * has right-of-way, and the phantom "stopped leader" a yielding car brakes for.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject
 * — unit-tested headless (Tests/IntersectionTest.cpp, prefix
 * GTC.AI.Traffic.Intersection).
 *
 * Discipline: a strict priority order, so the rule is total and deterministic.
 * A claim outranks another by, in order: higher Priority (a major road beats a
 * minor approach / a stop sign), then earlier ArrivalTime (first to the line goes
 * first — all-way-stop FCFS), then lower ApproachId (the deterministic stand-in
 * for "yield to the vehicle on your right").
 *
 * PURE-CORE boundary: gathering the live claims at a junction and feeding the
 * stop-line gap into FTrafficModel each tick is the DEFERRED adapter and is NOT
 * covered by the tests.
 */
struct GTC_UE5_API FIntersection
{
    /** One vehicle's claim to cross: which approach, when it reached the line, its road class. */
    struct FApproach
    {
        int32 ApproachId = 0;
        double ArrivalTime = 0.0;
        int32 Priority = 0; // higher = major road / right-of-way class
    };

    /**
     * Index of the claim with right-of-way (proceeds first), or -1 if none.
     * Total order: higher Priority, then earlier ArrivalTime, then lower ApproachId.
     */
    static int32 RightOfWay(const TArray<FApproach>& Claims);

    /** Does A have to yield to B (B outranks A by the discipline above)? */
    static bool Yields(const FApproach& A, const FApproach& B);

    /** Convenience: may the claim at SelfIndex proceed now (does it hold right-of-way)? */
    static bool MayProceed(const TArray<FApproach>& Claims, int32 SelfIndex);

    /**
     * The gap a yielding car should feed FTrafficModel: the stop line acts as a
     * stopped leader, so the effective bumper gap is the distance to the line minus
     * the car's front half-length. Never negative (clamped at 0 = at the line).
     */
    static double StopLineGap(double DistanceToStopLine, double VehicleHalfLength);
};
