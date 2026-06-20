// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A driveable graph built from a district's real OSM roads: shared endpoints
 * snap together into junction nodes, and each polyline becomes short directed
 * segments (both directions, since lanes are two-way here). Traffic agents walk
 * segment->segment, picking a continuation at each junction.
 *
 * Direct port of the Godot `RoadNetwork` (RefCounted) at
 * `game/scripts/world/road_network.gd`. Plain value type, no UObject —
 * unit-tested headless via the parity oracle (Tests/RoadNetworkTest.cpp, prefix
 * GTC.World.RoadNetwork).
 *
 * Geometry comes in ALREADY projected to local metres via FGeoProjection
 * (World/GeoProjection.h) — this model does not re-project. The AddDistrict
 * helper that consumes raw {kind, path:[[lat,lon]]} district dictionaries is
 * intentionally NOT ported: it depends on the OSM/district data shape, which is
 * a separate Wave-2/3 data-loading concern. The pure graph core takes
 * already-local FVector polylines via AddPolyline.
 *
 * Double precision throughout, to match the GDScript math (the Godot store is
 * PackedFloat32Array, but parity asserts are well within float<->double; we keep
 * doubles for the A* cost/heuristic accumulation determinism).
 *
 * Axis convention (Godot, Y-up): work happens on the XZ plane, Y is height. No
 * Godot->UE Z-up axis remap is baked in here — the model stays in the Godot XZ
 * frame so the ported unit tests match the oracle bit-for-bit. Porting the axis
 * convention to UE's Z-up space is a DEFERRED Wave-3 concern.
 *
 * Determinism: nodes/segments are stored in INSERTION ORDER (ordered TArrays),
 * which the oracle observes (node indices, segment counts, A* path order). A*
 * uses an ordered open-set with a strict-less tie-break (first-inserted wins),
 * mirroring the Godot Dictionary iteration order exactly.
 *
 * PURE-CORE boundary: this is the pure graph (nodes/edges/adjacency) plus the
 * A* node search and the segment/nearest-point geometry, computed from
 * caller-supplied local polylines. Recast navmesh generation and MassTraffic
 * lane/spline wiring are the DEFERRED Wave-3 adapters and are NOT implemented
 * here and NOT covered by the parity tests.
 */
class GTC_UE5_API FRoadNetwork
{
public:
    /** Mirrors the Godot point_on_segment / nearest_point dictionary results. */
    struct FSegmentPoint
    {
        FVector Pos = FVector::ZeroVector;
        FVector Heading = FVector::ZeroVector;
    };

    /** Nearest-point-on-graph result; bValid == false is the Godot empty `{}`. */
    struct FNearestPoint
    {
        bool bValid = false;
        int32 Seg = -1;
        double Offset = 0.0;
        FVector Pos = FVector::ZeroVector;
        FVector Heading = FVector::ZeroVector;
        double Dist = 0.0;
    };

    /** Snap defaults to 2.0 m, floored at 0.001 like the Godot _init. */
    explicit FRoadNetwork(double InSnap = 2.0);

    /** Add a polyline of local points, creating junction nodes and two-way segments. */
    void AddPolyline(const TArray<FVector>& Pts);

    int32 NodeCount() const { return Nodes.Num(); }
    int32 SegmentCount() const { return SegA.Num(); }

    /** Read access to node positions (the oracle scans these by index). */
    const TArray<FVector>& GetNodes() const { return Nodes; }

    /** Segment indices that start at `Node` (empty if none). */
    const TArray<int32>& SegmentsFrom(int32 Node) const;

    /** Sample a segment at a distance `Offset` from its start: {Pos, Heading}. */
    FSegmentPoint PointOnSegment(int32 Seg, double Offset) const;

    /**
     * Read-only segment topology, so an agent can walk the graph segment->segment:
     * a car riding `Seg` reaches `SegmentEndNode(Seg)`, then continues onto one of
     * `SegmentsFrom(that node)`. Out-of-range indices return -1 / 0 / ZeroVector
     * rather than asserting, so a caller holding a stale index degrades gracefully.
     */
    int32 SegmentStartNode(int32 Seg) const { return SegA.IsValidIndex(Seg) ? SegA[Seg] : -1; }
    int32 SegmentEndNode(int32 Seg) const { return SegB.IsValidIndex(Seg) ? SegB[Seg] : -1; }
    double SegmentLength(int32 Seg) const { return SegLen.IsValidIndex(Seg) ? SegLen[Seg] : 0.0; }

    /** Position of a node (ZeroVector for an out-of-range index). */
    FVector NodePosition(int32 Node) const { return Nodes.IsValidIndex(Node) ? Nodes[Node] : FVector::ZeroVector; }

    /**
     * Bucket every segment into the XZ grid so NearestPoint() scans only nearby
     * ones. Idempotent; NearestPoint builds it lazily on first use.
     */
    void BuildSpatialIndex();

    /**
     * Nearest point ON the road graph to a world position (planar XZ). bValid ==
     * false (the Godot empty `{}`) when the graph has no segments. Offset is
     * metres along the segment from its start node. Builds the index on first use.
     */
    FNearestPoint NearestPoint(const FVector& Pos);

    /**
     * A* shortest node path from Start to Goal over the directed graph (edge cost
     * = segment length, heuristic = straight-line distance), inclusive of both
     * ends. Empty if unreachable or the search budget (1500) is exceeded.
     */
    TArray<int32> FindPath(int32 Start, int32 Goal) const;

private:
    TArray<FVector> Nodes;
    TArray<int32> SegA;
    TArray<int32> SegB;
    TArray<double> SegLen;

    double Snap = 2.0;
    /** Snap-key -> node index. Ordered so iteration is deterministic (not observed, but tidy). */
    TMap<FString, int32> Index;
    /** Node index -> outgoing segment indices, in insertion order. */
    TMap<int32, TArray<int32>> Adj;

    double SegCell = 24.0;
    TMap<FString, TArray<int32>> SegIndex;
    bool bSegIndexBuilt = false;

    int32 NodeFor(const FVector& P);
    void AddSegment(int32 A, int32 B);
    FNearestPoint ProjectToSegment(int32 Seg, const FVector& Pos) const;

    /** Round-half-away-from-zero, matching Godot roundi for the snap grid key. */
    static int32 RoundHalfAway(double V);
};
