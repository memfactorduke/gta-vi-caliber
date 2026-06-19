// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * How a citizen reads the CROWD at a place — the difference between people who
 * pile obliviously into the same spot and people who notice "this diner's
 * packed" and act on it. The place registry already spreads crowds across POIs
 * of a kind; this adds the felt response: a private sort veers off when a place
 * is heaving, a sociable one is happy in the throng, and when the spot they need
 * is full they queue rather than clip through each other.
 *
 * Busyness is occupancy vs capacity (the registry's advisory numbers). The
 * personality knob is a `Tolerance` [0,1] (crowd-averse .. crowd-loving) the
 * adapter maps from the citizen's curiosity/sociability.
 *
 * PURE-CORE: scene-free, deterministic, no engine coupling. Unit-tested headless
 * (Tests/NpcCrowdingTest.cpp, prefix GTC.NPC.Decision.NpcCrowding). GTC-original.
 */
enum class ENpcBusyness : uint8
{
	Empty,  // nobody here
	Quiet,  // a few, plenty of room
	Busy,   // filling up
	Packed, // at or over capacity
};

struct GTC_UE5_API FNpcCrowding
{
	/**
	 * Classify a place from its live `Occupancy` and `Capacity`. Capacity 0 means
	 * uncapped (open street / synthesized point) — never "Packed", just Empty or
	 * Quiet. Otherwise: empty -> Empty, up to half -> Quiet, up to full -> Busy,
	 * at/over capacity -> Packed.
	 */
	static ENpcBusyness Classify(int32 Occupancy, int32 Capacity);

	/**
	 * Would this citizen veer away from a place this busy? `Tolerance` [0,1] is
	 * crowd-averse..crowd-loving. The crowd-averse avoid a Packed (and, if very
	 * averse, a Busy) place; the crowd-loving never avoid. Empty/Quiet never repel.
	 */
	static bool ShouldAvoid(ENpcBusyness Busyness, double Tolerance);

	/**
	 * If the citizen is heading somewhere Packed but ISN'T avoiding it (it needs
	 * to be there, e.g. its own workplace), it should queue/wait rather than
	 * overlap bodies. True only for Packed + not avoiding.
	 */
	static bool ShouldQueue(ENpcBusyness Busyness, bool bAvoiding);

	/**
	 * How far (cm) to stop SHORT of a place's centre, so a crowd rings/queues
	 * around a busy spot instead of stacking on the exact point. 0 when it's
	 * comfortable (walk right up); grows for Busy and more for Packed, and the
	 * crowd-averse (low `Tolerance`) hang back further than the crowd-loving.
	 * The adapter pushes the goal outward by this much along the approach.
	 */
	static float StandoffDistance(ENpcBusyness Busyness, double Tolerance);

	/** A short token for the busyness level (logging/debug/barks). */
	static const TCHAR* Name(ENpcBusyness Busyness);
};
