// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The glue between FRoadNetwork's A* and a car's lane: turn a node path
 * (FRoadNetwork::FindPath output) into the ordered centerline polyline a vehicle
 * drives, so cars route the TRUE road graph instead of a baked walkability grid
 * (ROADMAP M4). The adapter then offsets it to a lane with FLanePath and drives
 * `s` forward with FTrafficModel.
 *
 * Decoupled by design: it operates on caller-supplied node POSITIONS + the index
 * path, not on the FRoadNetwork class, matching the repo's pure-core philosophy
 * (depend on data, not on a heavy collaborator). The adapter passes
 * `Net.GetNodes()` and `Net.FindPath(a, b)`.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject
 * — unit-tested headless (Tests/RoadRouteTest.cpp, prefix GTC.World.RoadRoute).
 * Frame: the repo's planar XZ convention (Y up).
 *
 * PURE-CORE boundary: producing the node path (A*) is FRoadNetwork's job; lane
 * offset + travel is FLanePath's; wiring spawn/destination snapping to the live
 * graph is the DEFERRED adapter. None of that is covered here.
 */
struct GTC_UE5_API FRoadRoute
{
    /** Planar tolerance for collapsing a near-duplicate start/end onto a node. */
    static constexpr double Eps = 1e-4;

    /**
     * Centerline polyline for a node path: NodePositions[NodePath[i]] in order.
     * Returns empty if the path is empty or ANY index is out of range (a malformed
     * path yields no road rather than a phantom line cutting across the map).
     */
    static TArray<FVector> Centerline(const TArray<FVector>& NodePositions, const TArray<int32>& NodePath);

    /**
     * Like Centerline, but threaded from an arbitrary Start to an arbitrary End —
     * the car's real spawn point and destination, which sit partway along the
     * first/last segment, not on a junction. Start is prepended and End appended,
     * each skipped when it already coincides (within Eps, planar) with the node it
     * would duplicate, so no zero-length lead-in segment is produced. Empty when
     * the underlying node path is invalid (same rule as Centerline).
     */
    static TArray<FVector> CenterlineThrough(
        const TArray<FVector>& NodePositions, const TArray<int32>& NodePath,
        const FVector& Start, const FVector& End);
};
