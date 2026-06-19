// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure tile-grid addressing for the M3 streaming world.
 *
 * The streamed district is a uniform 2D grid of square tiles laid out on the
 * world XY plane (UE is Z-up; the ground plane is XY). This struct is the
 * coordinate substrate the rest of the streaming core sits on: it converts
 * between world positions and integer tile coordinates, reports a tile's
 * center/bounds, measures tile-space distance, and enumerates the square window
 * of candidate tiles around the camera (the coarse residency set the priority
 * scorer and budget scheduler then rank and cull).
 *
 * Plain non-UObject static helpers, the same scene-free testable-core pattern as
 * FPlayerMotion. Double precision throughout (FVector2D components are doubles in
 * UE5) so the math holds at city scale and matches the floating-origin world.
 * `TileSize` is in world units (cm); callers pass whatever cadence they stream at
 * (e.g. 12800 for 128 m tiles). Degenerate inputs (TileSize <= 0, negative
 * radius) return safe empty/zero results rather than asserting.
 *
 * Coverage: Tests/StreamingGridTest.cpp, prefix GTC.World.Streaming.Grid.
 */
struct GTC_UE5_API FStreamingGrid
{
	/** Integer tile coordinate containing a world XY point: floor(World / TileSize) per axis. */
	static FIntPoint WorldToTile(const FVector2D& WorldXY, double TileSize);

	/** Min (lower-left) corner of a tile in world space: Tile * TileSize. */
	static FVector2D TileMin(const FIntPoint& Tile, double TileSize);

	/** Center of a tile in world space: (Tile + 0.5) * TileSize. */
	static FVector2D TileCenter(const FIntPoint& Tile, double TileSize);

	/**
	 * Chebyshev (chessboard) distance in tiles: max(|dx|, |dy|). This is the
	 * "ring" a tile sits on around a center tile — the natural radius for a
	 * square residency window.
	 */
	static int32 TileRing(const FIntPoint& A, const FIntPoint& B);

	/** Euclidean distance from a world XY point to a tile's center, in world units. */
	static double CenterDistance(const FIntPoint& Tile, const FVector2D& WorldXY, double TileSize);

	/**
	 * Every tile within Chebyshev radius RingRadius of CenterTile — a
	 * (2*RingRadius + 1)^2 square window including the center. This is the coarse
	 * candidate set; finer culling to a circular load radius and velocity-biased
	 * ordering happens in the priority layer. Returned in deterministic row-major
	 * order (y ascending outer, x ascending inner). RingRadius < 0 yields empty.
	 */
	static TArray<FIntPoint> TilesInWindow(const FIntPoint& CenterTile, int32 RingRadius);
};
