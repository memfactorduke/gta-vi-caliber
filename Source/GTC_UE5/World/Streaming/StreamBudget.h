// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure per-frame work budgeting for the M3 streaming world.
 *
 * When the camera turns a corner a whole ring of tiles can become needed at
 * once. Streaming them all in one frame hitches; the fix is to admit only a
 * bounded slice of work per tick and let the rest wait for the next frames
 * (the "one bounded step per frame" rule). This caps admissions on two axes at
 * once: a count limit (at most MaxOps items) and a cost limit (at most
 * MaxCostPerTick units of work, e.g. estimated milliseconds or bytes).
 *
 * Forward-progress guarantee: the first item each tick is always admitted even
 * if its cost alone exceeds the budget, so a single oversized tile can never
 * deadlock the queue — it just gets a tick to itself.
 *
 * Plain non-UObject static helpers (the FStreamingGrid pattern), double
 * precision. Cost-unit agnostic. Pair with FTileStreamPriority ordering: feed
 * the costs of pending tiles in priority order and admit from the front.
 *
 * Coverage: Tests/StreamBudgetTest.cpp, prefix GTC.World.Streaming.Budget.
 */
struct GTC_UE5_API FStreamBudget
{
	/**
	 * Whether one more item costing NextCost can be admitted this tick, given how
	 * many (AdmittedSoFar) and how much cost (SpentCost) are already admitted.
	 * The incremental predicate a runtime loop calls as it measures real cost:
	 * false once the op-limit is hit; the first item is always allowed (forward
	 * progress); otherwise true only if SpentCost + NextCost stays within
	 * MaxCostPerTick. MaxOps <= 0 admits nothing.
	 */
	static bool CanAdmit(int32 AdmittedSoFar, double SpentCost, double NextCost, int32 MaxOps, double MaxCostPerTick);

	/**
	 * How many items from the front of a priority-ordered cost list to admit this
	 * tick, applying CanAdmit greedily. Returns a count in [0, Costs.Num()].
	 */
	static int32 AdmitCount(const TArray<double>& CostsInPriorityOrder, int32 MaxOps, double MaxCostPerTick);
};
