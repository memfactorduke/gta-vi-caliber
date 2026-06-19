// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Point-to-point pedestrian pathfinding over a walkability grid — the "route peds
 * THROUGH walkable space instead of straight-wandering" gap in ROADMAP M4. Now
 * that buildings are solid, a citizen must path around them; FNavGrid is the A*
 * that produces the waypoints, which FNpcSteering::AdvanceWaypoint then walks.
 *
 * Complements FFlowField (one shared goal, a field hundreds sample) with the
 * other half: an individual's path from A to B. Octile A* on an 8-neighbour grid
 * with corner-cut prevention (a diagonal is refused when it would clip a wall
 * corner — the same rule as FFlowField::DiagonalCutsCorner), so peds never shave
 * through a building edge.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject
 * — unit-tested headless (Tests/NavGridTest.cpp, prefix GTC.NPC.NavGrid).
 * Determinism: dense per-cell score arrays + an ordered open set with a strict-<
 * tie-break (first-inserted wins), mirroring FRoadNetwork's A*. Frame: the repo's
 * planar XZ convention (cx -> X, cz -> Z, Y up).
 *
 * PURE-CORE boundary: baking the walkability grid from the live navmesh and
 * feeding the waypoints to a CharacterMovementComponent is the DEFERRED adapter
 * and is NOT covered by the tests.
 */
struct GTC_UE5_API FNavGrid
{
    /** Row-major grid descriptor (cells indexed cz * Width + cx). */
    struct FGrid
    {
        int32 Width = 0;
        int32 Height = 0;
        int32 Index(int32 Cx, int32 Cz) const { return Cz * Width + Cx; }
        bool InBounds(int32 Cx, int32 Cz) const
        {
            return Cx >= 0 && Cx < Width && Cz >= 0 && Cz < Height;
        }
        int32 Count() const { return Width * Height; }
    };

    /**
     * Whether the diagonal step (Dx, Dz) out of (Cx, Cz) is allowed: both
     * orthogonally-adjacent cells it passes between must be walkable, so a ped
     * can't cut a wall corner. Dx/Dz are each -1 or +1. Trivially true for an
     * orthogonal step.
     */
    static bool DiagonalAllowed(
        const FGrid& G, const TArray<uint8>& Walkable, int32 Cx, int32 Cz, int32 Dx, int32 Dz);

    /**
     * Octile A* from StartCell to GoalCell over walkable cells (Walkable[i] != 0).
     * Returns the inclusive cell-index path (Start..Goal), or empty when the path
     * is invalid (out-of-range / blocked endpoint) or the goal is unreachable.
     * Start == Goal returns just {Start}.
     */
    static TArray<int32> FindPath(
        const FGrid& G, const TArray<uint8>& Walkable, int32 StartCell, int32 GoalCell);

    /** World cell-centre for (Cx, Cz): Origin + ((Cx+0.5)*CellSize, 0, (Cz+0.5)*CellSize). */
    static FVector CellToWorld(int32 Cx, int32 Cz, const FVector& Origin, double CellSize);

    /** The cell containing World (planar XZ); may be out of bounds (caller checks). */
    static int32 WorldToCell(const FGrid& G, const FVector& World, const FVector& Origin, double CellSize);

    /** Convert a cell-index path to world waypoints (cell centres) for the steerer. */
    static TArray<FVector> ToWorld(
        const FGrid& G, const TArray<int32>& CellPath, const FVector& Origin, double CellSize);
};
