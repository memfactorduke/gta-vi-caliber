// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure pool of NPC one-liners ("barks") by situation.
 *
 * Direct PURE-CORE port of the the reference `BarkPool` (RefCounted) at
 * `game/scripts/npc/bark_pool.gd`. Scene-free + deterministic so line selection
 * and cooldown gating are unit-tested headless (Tests/BarkPoolTest.cpp, prefix
 * GTC.NPC.Bark). A BarkDirector picks a situation and an index (counter/RNG) and
 * this returns the line; no scene/RNG access here.
 *
 * DEFERRED — Wave 3 adapter (NOT here): bark_director.gd (Node) and any
 * Pedestrian runtime wiring.
 */

/** Situation buckets, matching the the reference own enum `Situation { IDLE, ALARMED, FLEE }`. */
enum class EBarkSituation : uint8
{
	IDLE,
	ALARMED,
	FLEE,
};

struct GTC_UE5_API FBarkPool
{
	/**
	 * The line for a situation at the given index, wrapping (the reference posmod) so any
	 * counter/RNG value is valid. Returns "" for an unknown/empty situation.
	 */
	static FString Line(EBarkSituation Situation, int32 Index);

	/** Number of distinct lines for a situation (for callers that want to vary). */
	static int32 Count(EBarkSituation Situation);

	/** Cooldown gate so an NPC doesn't chatter every frame. */
	static bool ShouldBark(double TimeSinceLast, double Cooldown);
};
