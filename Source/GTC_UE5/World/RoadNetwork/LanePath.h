// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A single drivable lane: the geometry a traffic agent actually rides. A road's
 * centerline (e.g. one continuation through an FRoadNetwork node path) is a
 * two-way line down the middle of the street; a car drives offset to its SIDE of
 * travel. FLanePath bakes that offset polyline once, arc-length-parameterises it,
 * and answers the two questions a moving car asks every tick: "where (pos +
 * heading) am I at distance s along the lane?" and "how do I advance s by the
 * metres I travelled this frame, without running off the end?".
 *
 * This is the missing glue that lets cars route a TRUE road instead of a baked
 * walkability grid (ROADMAP M4): FRoadNetwork::FindPath gives the node path,
 * FRoadRoute (next) turns it into a centerline, FLanePath offsets+rides it, and
 * FTrafficModel supplies the longitudinal acceleration that drives `s` forward.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject
 * — unit-tested headless (Tests/LanePathTest.cpp, prefix GTC.World.LanePath).
 *
 * Frame: the repo's planar XZ convention (Y is up), matching FRoadNetwork /
 * FGpsNavigation. Headings are unit (X, 0, Z); the zero-length fallback is the
 * Godot forward (0, 0, -1), as in FRoadNetwork::PointOnSegment. Porting to UE's
 * Z-up space is a DEFERRED adapter concern.
 *
 * Lane side: LateralOffset is signed and applied along RightOf(travel direction)
 * — a POSITIVE offset places the lane to the driver's right of the centerline
 * (right-hand traffic); negative for left-hand. The corner offset is the simple
 * averaged-normal kind (no miter compensation), so width pinches slightly on
 * sharp bends; sharp turns are junctions, handled by the intersection core, not
 * mid-lane. PURE-CORE boundary: applying a pose to a Chaos vehicle / actor each
 * tick is the DEFERRED Wave-3 adapter and is NOT covered by the tests.
 */
struct GTC_UE5_API FLanePath
{
    /** Tolerance for zero-length segments / "reached the end" (planar metres). */
    static constexpr double Eps = 1e-4;

    /** A sampled point on the lane: where the car is and which way it faces. */
    struct FPose
    {
        FVector Pos = FVector::ZeroVector;
        FVector Heading = FVector(0.0, 0.0, -1.0); // Godot forward fallback
    };

    /**
     * Unit perpendicular to the right of a travel heading, in the XZ plane:
     * RightOf((1,0,0)) == (0,0,-1). Input need not be normalised; output is unit
     * (zero for a zero heading).
     */
    static FVector RightOf(const FVector& Heading);

    /**
     * Bake the lane by offsetting Centerline to the side of travel by
     * LateralOffset (see "Lane side" above). Each vertex shifts along the average
     * of its adjacent segments' right-perpendiculars. A 0/1-point or all-degenerate
     * centerline yields a zero-length lane (the raw points, Length() == 0).
     */
    void BuildFromCenterline(const TArray<FVector>& Centerline, double LateralOffset);

    /** Baked lane vertex count (== centerline vertex count). */
    int32 NumPoints() const { return Points.Num(); }

    /** A baked lane vertex (offset already applied). */
    FVector Point(int32 Index) const { return Points[Index]; }

    /** Total drivable length along the offset polyline (planar metres). */
    double Length() const { return Cumulative.IsEmpty() ? 0.0 : Cumulative.Last(); }

    /**
     * Pose at arc-length S (clamped to [0, Length()]). Empty lane → zero pos with
     * the forward fallback; single point → that point. Heading is the local
     * segment direction (the forward fallback on a degenerate segment).
     */
    FPose PoseAtDistance(double S) const;

    /** Remaining length from S to the lane end (0 once at/over the end). */
    double DistanceToEnd(double S) const;

    /**
     * Advance arc-length S by DeltaS, clamped to [0, Length()]. bOutReachedEnd is
     * true when a forward step (DeltaS > 0) lands at/over the end — the signal a
     * car uses to pick its next lane at the junction.
     */
    double Advance(double S, double DeltaS, bool& bOutReachedEnd) const;

private:
    /** Offset lane vertices. */
    TArray<FVector> Points;
    /** Cumulative arc length to each vertex; Cumulative[0] == 0, Last() == Length. */
    TArray<double> Cumulative;
};
