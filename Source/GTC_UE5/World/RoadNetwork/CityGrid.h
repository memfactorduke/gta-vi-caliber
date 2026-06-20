// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Generates a rectangular grid of street polylines — the runtime road source the
 * traffic layer drives until real OSM/citygen geometry is wired in. Feed the
 * polylines to FRoadNetwork::AddPolyline and you get a true, routable road graph:
 * shared intersections become junction nodes, each block edge a two-way segment,
 * so FTrafficAgent can route it (A*), turn at junctions (FTurnChoice) and queue
 * per lane (FTrafficLane) instead of sliding along the old invisible axis-aligned
 * grid that never turned a corner.
 *
 * The visual street layout is the same city-block pattern the previous traffic
 * subsystem faked; the difference is that this one is a real graph the cars
 * navigate. Swapping in OSM data later is just "supply different polylines to the
 * same FRoadNetwork" — the agent/subsystem don't change.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject
 * — unit-tested headless (Tests/CityGridTest.cpp, prefix GTC.World.CityGrid).
 * Frame: the repo's planar XZ convention (Y up), matching FRoadNetwork. Units are
 * the caller's (the traffic subsystem runs the graph in metres).
 */
struct GTC_UE5_API FCityGrid
{
    struct FSpec
    {
        /** Centre of the grid (XZ; Y is the road height). */
        FVector Origin = FVector::ZeroVector;
        /** Spacing between parallel streets (one block). */
        double BlockSize = 80.0;
        /** Blocks across X / along Z. The intersection lattice is (BlocksX+1) x
         *  (BlocksZ+1); clamped to at least 1 block each way. */
        int32 BlocksX = 6;
        int32 BlocksZ = 6;
    };

    /** Intersection columns (streets running along Z), == BlocksX + 1 (min 2). */
    static int32 CountX(const FSpec& Spec) { return FMath::Max(1, Spec.BlocksX) + 1; }
    /** Intersection rows (streets running along X), == BlocksZ + 1 (min 2). */
    static int32 CountZ(const FSpec& Spec) { return FMath::Max(1, Spec.BlocksZ) + 1; }

    /** World position of intersection (i, j), i in [0, CountX), j in [0, CountZ). */
    static FVector Intersection(const FSpec& Spec, int32 i, int32 j);

    /**
     * The street polylines: one per row (running along X) and one per column
     * (running along Z), each threading every intersection on that line so
     * FRoadNetwork creates a segment between consecutive junctions. Feed each to
     * FRoadNetwork::AddPolyline.
     */
    static TArray<TArray<FVector>> Polylines(const FSpec& Spec);

    /**
     * Snap a world position to the nearest block-grid origin around it (multiples
     * of BlockSize from Spec.Origin). The subsystem recenters the grid on this so
     * the road graph only rebuilds when the player crosses a block boundary, not
     * every frame.
     */
    static FVector SnapOriginTo(const FSpec& Spec, const FVector& WorldXZ);
};
