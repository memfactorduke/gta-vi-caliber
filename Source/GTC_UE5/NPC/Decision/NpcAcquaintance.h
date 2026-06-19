// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A citizen's little black book — the handful of other people they've come to
 * recognise. Until now every encounter was between strangers: two NPCs who'd
 * chatted yesterday met today with the same wary opener. This gives each person
 * a bounded, decaying memory of who they've met, so a regular at the same corner
 * gets a warm "Oh hey, you again!" while a true stranger gets the cautious line —
 * the difference between a crowd of interchangeable pawns and a neighbourhood
 * where people have histories.
 *
 * Familiarity rises a notch each meeting (capped) and fades slowly when they
 * stop running into each other (Forget), so ties are earned and lost like real
 * acquaintance. The table is capacity-bounded with least-familiar eviction: you
 * can only hold so many faces, and a passing stranger never bumps a close friend.
 *
 * PURE-CORE: a plain value type (no UObject, no engine coupling), deterministic,
 * unit-tested headless (Tests/NpcAcquaintanceTest.cpp, prefix
 * GTC.NPC.Decision.NpcAcquaintance). AGTCCitizen owns one and calls Meet() when a
 * chat begins, then greets by TierWith(). Identities are stable citizen ids
 * (GetStableId, or seed for non-roster citizens); INDEX_NONE means "anonymous"
 * and is never recorded.
 */
enum class ENpcFamiliarity : uint8
{
	Stranger,     // never met, or barely
	FamiliarFace, // seen around — worth a nod
	Friend,       // a real regular
};

struct FNpcAcquaintanceEntry
{
	int32 Id = INDEX_NONE;
	float Familiarity = 0.0f;
};

struct GTC_UE5_API FNpcAcquaintance
{
	/** How many distinct faces a citizen can hold before the least-familiar is
	 *  forgotten to make room. Small + linear — this is a person's memory, not a db. */
	static constexpr int32 Capacity = 16;

	/** Familiarity gained per meeting, and the ceiling it saturates at. */
	static constexpr float MeetGain = 1.0f;
	static constexpr float MaxFamiliarity = 5.0f;

	/** Thresholds for the tiers (see TierFor). */
	static constexpr float FriendThreshold = 3.0f;
	static constexpr float FamiliarThreshold = 1.0f;

	TArray<FNpcAcquaintanceEntry> Entries;

	/**
	 * Record running into `OtherId`, bumping familiarity by Amount (default
	 * MeetGain), capped at MaxFamiliarity. Adds a new entry if unknown; if the
	 * book is full, evicts the least-familiar face first. INDEX_NONE is ignored.
	 */
	void Meet(int32 OtherId, float Amount = MeetGain);

	/** Current familiarity with `OtherId` (0 if unknown / anonymous). */
	float FamiliarityWith(int32 OtherId) const;

	/** The relationship tier with `OtherId`. */
	ENpcFamiliarity TierWith(int32 OtherId) const;

	/** Fade every tie by Amount (forgetting over time); entries at/below 0 drop. */
	void Forget(float Amount);

	/** Number of faces currently remembered. */
	int32 Num() const { return Entries.Num(); }

	/** Map a raw familiarity value to its tier. */
	static ENpcFamiliarity TierFor(float Familiarity);
};
