// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The little "ooh, what's that?" — the curious glances that keep a citizen from
 * beelining to its destination like an automaton. Even with somewhere to be,
 * real people clock a shop window, a passing dog, a commotion across the street.
 * This decides, per opportunity, whether a citizen takes such a glance, scaled by
 * how nosy they are, plus a spacing so it stays an occasional thing, not a tic.
 *
 * Deliberately light: it drives a head-turn + murmur (a glance, the anim layer's
 * job), not a full route change — so it adds life without fighting the schedule.
 *
 * PURE-CORE: scene-free, deterministic, no engine coupling. The per-opportunity
 * ROLL itself is the adapter's (it owns the RNG stream); this supplies the
 * probability and the cooldown. Unit-tested headless
 * (Tests/NpcDetourTest.cpp, prefix GTC.NPC.Decision.NpcDetour). GTC-original.
 */
struct GTC_UE5_API FNpcDetour
{
	/**
	 * Probability [0..~0.2] that a citizen takes a curious glance on a given
	 * opportunity, rising with `Curiosity` [0,1]. Bounded so even the nosiest
	 * don't gawk at everything.
	 */
	static float GlanceChance(double Curiosity);

	/**
	 * Seconds to wait before the next possible glance after taking one — varied by
	 * `Seed` and a touch shorter for the curious (they look more often). Keeps
	 * glances spaced out rather than back-to-back.
	 */
	static double GlanceCooldown(double Curiosity, int32 Seed);
};
