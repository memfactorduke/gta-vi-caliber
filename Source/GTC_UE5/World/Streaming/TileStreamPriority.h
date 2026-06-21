// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure load-priority scoring for the M3 streaming world.
 *
 * Given the camera's planar position and velocity, ranks candidate tiles so the
 * ones the camera is heading *toward* load first and the ones behind it are the
 * least urgent. The metric is an **anticipatory (look-ahead) distance**: measure
 * each tile's distance not from where the camera is, but from where it will be
 * `LookAheadSeconds` from now (CameraPos + Velocity * LookAhead). Driving fast
 * pushes that point further ahead, so tiles down the road become "closer" and
 * get streamed in before the camera reaches them; a stationary camera collapses
 * to plain radial distance.
 *
 * Lower `EffectiveDistance` == higher streaming priority (it behaves as a cost,
 * the natural input to the priority-queue ordering and frame-budget scheduler
 * that consume it). Plain non-UObject static helpers (the FPlayerMotion /
 * FStreamingGrid pattern), double precision, planar XY (streaming is driven by
 * horizontal movement). Takes a tile *center* (FStreamingGrid::TileCenter) so it
 * stays decoupled from the grid addressing.
 *
 * Coverage: Tests/TileStreamPriorityTest.cpp, prefix GTC.World.Streaming.Priority.
 */
struct GTC_UE5_API FTileStreamPriority
{
	/**
	 * Where the camera is predicted to be in LookAheadSeconds:
	 * CameraPos + Velocity * LookAheadSeconds. LookAhead <= 0 (or zero velocity)
	 * returns CameraPos unchanged.
	 */
	static FVector2D LookAheadPoint(const FVector2D& CameraPos, const FVector2D& Velocity, double LookAheadSeconds);

	/**
	 * Anticipatory distance from a tile center to the camera's look-ahead point —
	 * the streaming priority cost (lower = load sooner). With zero velocity or
	 * LookAhead <= 0 this equals the plain camera-to-tile distance.
	 */
	static double EffectiveDistance(
		const FVector2D& CameraPos,
		const FVector2D& Velocity,
		const FVector2D& TileCenter,
		double LookAheadSeconds);

	/**
	 * Signed distance of a tile along the camera's travel direction: positive if
	 * the tile is ahead of the camera, negative if behind, zero for a lateral
	 * tile or a stationary camera. (The projection of (TileCenter - CameraPos)
	 * onto the unit velocity.) Useful for ahead/behind classification independent
	 * of the look-ahead metric.
	 */
	static double AlongTrack(const FVector2D& CameraPos, const FVector2D& Velocity, const FVector2D& TileCenter);

	/**
	 * True if tile A is strictly more urgent to stream than tile B (A's effective
	 * distance < B's). A strict-weak-ordering comparator the priority queue can
	 * sort with so the most urgent tiles come first — the strict `<` keeps it
	 * irreflexive (a tile is never more urgent than itself), as a sort requires.
	 */
	static bool IsMoreUrgent(
		const FVector2D& CameraPos,
		const FVector2D& Velocity,
		const FVector2D& TileCenterA,
		const FVector2D& TileCenterB,
		double LookAheadSeconds);
};
