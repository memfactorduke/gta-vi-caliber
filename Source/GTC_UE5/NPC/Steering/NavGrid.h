// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure, runtime-free pedestrian path-finding over a walkability grid — the
 * routing side of NPC life. A citizen that wants to walk somewhere asks for a
 * point-to-point path; FNavGrid runs 8-connected A* across the walkable cells
 * and hands back waypoints that route *through* open space and *around* solid
 * buildings, instead of the straight-line wander peds do today. The waypoints
 * are world-space FVectors meant to be walked with FNpcSteering::AdvanceWaypoint
 * / Arrive, so the existing steering layer follows the route unchanged.
 *
 * Scene-free pure core: a plain non-UObject struct holding the walkability
 * bitmap plus static-feeling query methods, the FRoadNetwork pattern (double
 * precision, deterministic). The crowd/Mass adapter owns baking the grid from
 * the navmesh/landscape and feeding the waypoints to a mover — NOT here.
 *
 * Conventions: the grid lives on the XZ ground plane (the repo's pedestrian
 * convention, see FNpcSteering); cell (X,Y) maps to world (Origin + (X,Y)*Cell)
 * at its centre, with world Y (height) left at 0 in returned waypoints. Cells
 * are row-major, index = Y*Width + X. Unit-tested headless in
 * Tests/NavGridTest.cpp and the oracle navgrid_oracle.cpp (prefix GTC.NPC.NavGrid).
 */

/** A grid cell coordinate. */
struct FNavGridCell
{
	int32 X = 0;
	int32 Y = 0;

	FNavGridCell() = default;
	FNavGridCell(int32 InX, int32 InY) : X(InX), Y(InY) {}

	bool operator==(const FNavGridCell& O) const { return X == O.X && Y == O.Y; }
};

struct GTC_UE5_API FNavGrid
{
	int32 Width = 0;
	int32 Height = 0;
	double CellSize = 1.0;
	/** World XZ position of the centre of cell (0,0). */
	FVector2D Origin = FVector2D(0.0, 0.0);
	/** Row-major Width*Height; 1 = walkable, 0 = solid. */
	TArray<uint8> Walkable;

	/** Allocate a W x H grid, every cell walkable, cells of side InCellSize. */
	void Init(int32 W, int32 H, double InCellSize, const FVector2D& InOrigin);

	bool InBounds(int32 X, int32 Y) const { return X >= 0 && Y >= 0 && X < Width && Y < Height; }
	int32 CellIndex(int32 X, int32 Y) const { return Y * Width + X; }

	/** Out-of-bounds cells read as solid, so callers needn't bounds-check first. */
	bool IsWalkable(int32 X, int32 Y) const;
	void SetWalkable(int32 X, int32 Y, bool bWalkable);

	/** Centre of cell (X,Y) in world XZ. */
	FVector2D CellToWorld(int32 X, int32 Y) const;
	/** Nearest cell to a world XZ point (clamped into the grid). */
	void WorldToCell(const FVector2D& P, int32& OutX, int32& OutY) const;

	/**
	 * 8-connected A* from (SX,SY) to (GX,GY). Returns the cell path inclusive of
	 * both ends; empty if either end is solid or unreachable. Diagonal steps cost
	 * sqrt(2) and are forbidden when they would cut the corner of a solid cell, so
	 * peds never clip a building edge. Deterministic: fixed neighbour order and a
	 * stable tie-break, so the same request always yields the same path.
	 */
	TArray<FNavGridCell> FindPath(int32 SX, int32 SY, int32 GX, int32 GY) const;

	/**
	 * Convenience: path between two world XZ points, returned as world-space
	 * waypoints (FVector on the XZ plane, Y = 0) at cell centres, ready to feed
	 * FNpcSteering::AdvanceWaypoint. Empty if unreachable.
	 */
	TArray<FVector> FindWorldPath(const FVector2D& Start, const FVector2D& Goal) const;

	/**
	 * Drop interior cells that lie on the same straight run, keeping only the
	 * corners — fewer waypoints for the same route, so the steerer turns only
	 * where the path actually bends. Endpoints are always kept.
	 */
	static TArray<FNavGridCell> Simplify(const TArray<FNavGridCell>& Path);
};
