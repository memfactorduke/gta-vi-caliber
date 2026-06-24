// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ResidencyPlanner.h"
#include "TileLodSelect.h"

/**
 * Stateful streaming brain for the M3 world — the scene-free core the runtime
 * UWorldSubsystem adapter drives each tick.
 *
 * FResidencyPlanner is pure and stateless: given the camera state and the CURRENT
 * resident set it returns this tick's load/unload lists. Something has to own that
 * resident set across frames, apply each plan to it, and track which detail band
 * (Full/HLOD/Impostor) every resident tile is currently shown at so the LOD swap
 * has hysteresis. That state is exactly what this struct holds — and nothing more,
 * so it stays a plain non-UObject testable core (the FStreamingGrid / FPlayerMotion
 * pattern). The adapter samples the camera, calls Step(), and forwards the returned
 * plan + band changes to the actual World Partition cell load/unload (the live-editor
 * seam, kept out of here).
 *
 * Load model: an admitted load becomes resident immediately (synchronous
 * approximation). Real async load latency is modelled by the per-tick load cap
 * (FResidencyPlanInput::MaxLoadsPerTick) plus the anticipatory look-ahead — if the
 * camera outruns the budget, the resident ring falls behind, which is precisely the
 * failure the acceptance probe asserts against.
 *
 * Coverage: Tests/StreamingTraverseTest.cpp, prefix GTC.World.Streaming.Traverse.
 */
struct GTC_UE5_API FStreamingResidency
{
	/** One tile whose detail band changed this step (for the LOD-swap seam). */
	struct FBandChange
	{
		FIntPoint Tile = FIntPoint::ZeroValue;
		ETileLod Lod = ETileLod::Unloaded;
	};

	/** Everything the adapter needs to act on after one Step(). */
	struct FStepResult
	{
		FStreamPlan Plan;                  // tiles begun-loading / released this step
		TArray<FBandChange> BandChanges;   // resident tiles whose LOD band moved
	};

	/**
	 * Advance one tick: run the planner against the held resident set, apply the
	 * load/unload lists to it, then re-evaluate the detail band of every resident
	 * tile (with hysteresis off its current band). Returns the plan and the set of
	 * band changes for the adapter to push at World Partition.
	 *
	 * `Bands` outer distances and `LodMargin` are in the same world units as
	 * `In.TileSize`; pass FTileLodBands{} to skip band tracking (all Full).
	 */
	FStepResult Step(const FResidencyPlanInput& In, const FTileLodBands& Bands, double LodMargin);

	/** Tiles currently resident, in insertion order. */
	const TArray<FIntPoint>& Resident() const { return ResidentTiles; }
	int32 Num() const { return ResidentTiles.Num(); }
	bool IsResident(const FIntPoint& Tile) const { return ResidentTiles.Contains(Tile); }

	/** Current detail band of a resident tile (Unloaded if not resident). */
	ETileLod BandOf(const FIntPoint& Tile) const;

	/** Drop all residency — the adapter calls this on level teardown / Deinitialize. */
	void Reset();

private:
	TArray<FIntPoint> ResidentTiles;
	TMap<FIntPoint, ETileLod> TileBands;
};
