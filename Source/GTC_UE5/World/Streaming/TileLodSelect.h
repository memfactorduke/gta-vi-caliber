// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure per-tile detail-band selection for the M3 streaming world.
 *
 * A streamed tile is shown at one of four detail levels by camera distance: the
 * full-detail mesh up close, a merged HLOD proxy further out, a cheap impostor
 * billboard further still, and nothing at all beyond the streaming range. This
 * picks that band from distance — and, given the tile's current band, applies a
 * symmetric dead-zone (margin) around each threshold so a tile sitting on a
 * boundary doesn't flip detail every frame (the LOD analogue of the load/unload
 * hysteresis in FTileHysteresis).
 *
 * Selection is the rule; the actual mesh/HLOD/impostor swap and the impostor
 * BAKING are live-editor runtime work that consume this decision (see PROGRESS).
 *
 * Plain non-UObject static helpers (the FStreamingGrid / FTileHysteresis
 * pattern), double precision, distance-metric agnostic. Thresholds are
 * normalised to be non-decreasing (Full <= HLOD <= Impostor) so a misordered
 * config degrades predictably instead of inverting.
 *
 * Coverage: Tests/TileLodSelectTest.cpp, prefix GTC.World.Streaming.Lod.
 */

/** Detail band for a tile, ordered finest (0) to coarsest/none (3). */
enum class ETileLod : uint8
{
	Full = 0,     // up close: full-detail mesh
	HLOD = 1,     // mid: merged HLOD proxy
	Impostor = 2, // far: cheap billboard impostor
	Unloaded = 3, // beyond streaming range: not shown
};

/** Outer distance of each detail band (inclusive). Non-decreasing after normalise. */
struct FTileLodBands
{
	double FullDist = 0.0;     // Distance <= FullDist -> Full
	double HlodDist = 0.0;     // <= HlodDist -> HLOD
	double ImpostorDist = 0.0; // <= ImpostorDist -> Impostor; beyond -> Unloaded
};

struct GTC_UE5_API FTileLodSelect
{
	/**
	 * Band purely from distance (no hysteresis): the first band whose outer
	 * distance Distance falls within, else Unloaded. Thresholds are normalised
	 * non-decreasing first.
	 */
	static ETileLod Select(double Distance, const FTileLodBands& Bands);

	/**
	 * Band from distance starting at the tile's Current band, with a symmetric
	 * Margin dead-zone on each threshold: move coarser only once Distance exceeds
	 * a boundary by more than Margin, finer only once it drops below by more than
	 * Margin, otherwise hold Current. Resolves multi-band jumps (a teleport)
	 * without oscillating within a single call. Margin is clamped to >= 0.
	 */
	static ETileLod SelectWithHysteresis(double Distance, ETileLod Current, const FTileLodBands& Bands, double Margin);
};
