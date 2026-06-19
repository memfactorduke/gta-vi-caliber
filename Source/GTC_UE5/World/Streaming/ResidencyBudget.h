// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure residency / VRAM accounting for the M3 streaming world.
 *
 * A 4 km² district holds far more tile data than fits in memory at once, so the
 * resident set is capped by a byte budget. This tracks the bytes a set of
 * resident tiles occupies and, when admitting a new tile would blow the budget,
 * picks which resident tiles to evict — the least wanted first.
 *
 * "Least wanted" is the entry's EvictionKey: higher = evict sooner. Feed it
 * FTileStreamPriority::EffectiveDistance directly (the farthest / least urgent
 * tile is shed first). The caller passes only EVICTABLE candidates — pinned
 * tiles (the one under the camera, the near collision/nav ring) are simply left
 * out of the list, never offered for eviction.
 *
 * Plain non-UObject static helpers (the FStreamBudget pattern). Bytes are int64
 * (a city's resident set is multi-gigabyte). Eviction is deterministic: highest
 * EvictionKey first, ties broken by larger byte size (free more per eviction)
 * then lower index.
 *
 * Coverage: Tests/ResidencyBudgetTest.cpp, prefix GTC.World.Streaming.Residency.
 */

/** One resident tile's footprint and how readily it may be evicted. */
struct FResidentTile
{
	int64 Bytes = 0;          // memory this tile occupies
	double EvictionKey = 0.0; // higher = evict sooner (e.g. EffectiveDistance)
};

struct GTC_UE5_API FResidencyBudget
{
	/** Total bytes occupied by the resident set. */
	static int64 TotalBytes(const TArray<FResidentTile>& Resident);

	/** True if the resident set exceeds BudgetBytes. */
	static bool IsOverBudget(const TArray<FResidentTile>& Resident, int64 BudgetBytes);

	/**
	 * Indices (into Resident) to evict to free at least BytesToFree, highest
	 * EvictionKey first, returned in eviction order. Frees as much as possible if
	 * the whole set isn't enough; returns empty for BytesToFree <= 0.
	 */
	static TArray<int32> SelectEvictions(const TArray<FResidentTile>& Resident, int64 BytesToFree);

	/**
	 * Indices to evict so an incoming tile of IncomingBytes fits within
	 * BudgetBytes: frees max(0, Total + Incoming - Budget). With IncomingBytes 0
	 * this just trims an over-budget set back under the cap. Empty if it already
	 * fits.
	 */
	static TArray<int32> EvictionsToAdmit(const TArray<FResidentTile>& Resident, int64 IncomingBytes, int64 BudgetBytes);
};
