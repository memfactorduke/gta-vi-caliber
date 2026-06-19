// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure per-frame residency planner for the M3 streaming world — the capstone
 * that composes the other streaming cores into one decision.
 *
 * Each tick it answers: given where the camera is and where it's heading, and
 * which tiles are resident now, which tiles should we start LOADING this frame
 * (most urgent first, capped to a per-frame budget) and which should we UNLOAD?
 * It folds together:
 *   - FStreamingGrid     — the candidate window of tiles around the camera
 *   - FTileHysteresis    — the load/unload band so boundary tiles don't thrash
 *   - FTileStreamPriority— anticipatory ordering so tiles ahead load first
 *   - a per-frame load cap (the FStreamBudget count axis)
 *
 * Deterministic: the load list is sorted by anticipatory distance with a stable
 * tile-coordinate tie-break, and the unload list is sorted by tile coordinate.
 * The runtime adapter (a UWorldSubsystem) drives this and hands the lists to the
 * actual World Partition cell load/unload — that wiring is live-editor work, not
 * here (see PROGRESS).
 *
 * Plain non-UObject static helpers, double precision, planar XY.
 *
 * Coverage: Tests/ResidencyPlannerTest.cpp, prefix GTC.World.Streaming.Planner.
 */

/** Inputs describing this tick's camera state and the streaming policy. */
struct FResidencyPlanInput
{
	FVector2D CameraPos = FVector2D::ZeroVector;
	FVector2D Velocity = FVector2D::ZeroVector; // planar camera velocity (for anticipation)
	double TileSize = 0.0;                       // world units per tile edge
	int32 WindowRadius = 0;                      // Chebyshev radius of the candidate window
	double LoadRadius = 0.0;                     // load a tile within this (hysteresis inner)
	double UnloadRadius = 0.0;                   // unload a tile beyond this (hysteresis outer)
	double LookAheadSeconds = 0.0;               // anticipation horizon for priority
	int32 MaxLoadsPerTick = 0;                   // per-frame load cap; <= 0 means unbounded
};

/** This tick's plan: tiles to begin loading (priority order) and to release. */
struct FStreamPlan
{
	TArray<FIntPoint> ToLoad;   // most urgent first, capped to MaxLoadsPerTick
	TArray<FIntPoint> ToUnload; // every tile to release this tick, coord-sorted
};

struct GTC_UE5_API FResidencyPlanner
{
	/**
	 * Compute this tick's load/unload plan from the camera state and the current
	 * resident set. A window tile that is resident and still wanted stays put
	 * (neither list); one beyond the unload radius, or resident but outside the
	 * window entirely, is unloaded; a not-yet-resident tile within the load
	 * radius becomes a load candidate, ranked by anticipatory distance and capped
	 * to the per-frame budget.
	 */
	static FStreamPlan Plan(const FResidencyPlanInput& In, const TArray<FIntPoint>& Resident);
};
