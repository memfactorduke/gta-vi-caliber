// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

/**
 * Pure freeroam ambient-encounter director — the "the city throws something at you" loop: a
 * mugging in progress, a street-race challenge, a getaway-driver job, a gang shootout. It
 * weight-picks an eligible encounter for the player's current context (wanted stars, district)
 * while respecting per-event cooldowns and a global anti-spam gap, so encounters feel
 * spontaneous but never spam.
 *
 * Plain C++ value type (no UObject): all randomness flows through a caller-supplied FRandomStream
 * (cf. LootTable), so selection stays deterministic and unit-tested headless. A world director
 * (deferred Wave-3 adapter) calls TriggerNext() on a timer and spawns the returned encounter id.
 *
 * Each event is {Id, Kind, Weight, MinStars, MaxStars, District, Cooldown, LastFired}; an empty
 * District means "any". Malformed entries (missing id, non-positive weight, duplicate id) are
 * dropped at construction.
 *
 * Parity notes (oracle: the reference test test_ambient_events.gd, 12 tests):
 * - the reference `float` is 64-bit; time and weight are stored as `double` to avoid 32-bit drift.
 * - an insertion-ordered map is insertion-ordered and Ids()/EligibleIds()/Reset() iterate it in that
 *   order, so an explicit ordered backing store (TArray<FEntry> + TMap<FString,int32> index)
 *   mirrors that observable order. TMap alone does NOT preserve insertion order.
 * - the reference `-INF` (never fired) → -std::numeric_limits<double>::infinity().
 * - the reference `int(context.get("stars", 0))` truncates a float toward zero; FAmbientContext::Stars is
 *   carried as a double and truncated with a (int32) cast (C++ truncates toward zero — same rule).
 * - the reference `maxf(0.0, cooldown)` floor on registration → FMath::Max(0.0, Cooldown).
 * - RNG: the reference _weighted_pick does `rng.randf() * total`; UE uses FRandomStream::GetFraction().
 *   The oracle only pins range/eligibility selection (a 0-star pick never returns a stars-gated or
 *   docks-only id) and determinism per seed — NOT a Godot-byte-identical sequence. So picks are
 *   deterministic-per-seed but NOT byte-identical to Godot. A null-rng early return is mirrored by
 *   the TriggerNextNoRng() no-op overload (the reference `rng == null` guard).
 *
 * DEFERRED-OWNERSHIP (lead-signed, option-1 own-state): the ambient-from-wanted-level coupling
 * (encounters modulated by wanted level) is owned here as OWN STATE — FAmbientEvents takes the
 * wanted-star count as an INPUT PARAMETER inside FAmbientContext and owns its own scheduling state
 * (per-event LastFired + the global clock). It does NOT include, depend on, or reference any Wanted
 * type. The director/spawner that reads the live wanted level and spawns the chosen encounter actor
 * (ambient_event_director.gd + ambient_encounter_spawner.gd, both `extends Node`) is DEFERRED to
 * Wave 3 and is NOT ported or tested here.
 */
class GTC_UE5_API FAmbientEvents
{
public:
	/** Minimum seconds between ANY two ambient events. */
	static constexpr double GlobalGap = 30.0;

	/** One event definition consumed by the constructor. Mirrors a lookup table. */
	struct FEventDef
	{
		FString Id;
		FString Kind = TEXT("misc");
		double Weight = 1.0;
		int32 MinStars = 0;
		int32 MaxStars = 5;
		FString District;
		double Cooldown = 60.0;
	};

	/** The query context: wanted stars (as a double, truncated toward zero) and current district. */
	struct FAmbientContext
	{
		double Stars = 0.0;
		FString District;
	};

	/** Construct from a list of event defs; an empty list uses DefaultEvents(). */
	explicit FAmbientEvents(const TArray<FEventDef>& Events = TArray<FEventDef>());

	/** The built-in encounter table spanning calm-to-hot situations. */
	static TArray<FEventDef> DefaultEvents();

	int32 EventCount() const;

	/** True if the event id exists in the roster. */
	bool HasEvent(const FString& Id) const;

	/** Every event id, in insertion order. */
	TArray<FString> Ids() const;

	/** Category tag of an event ("" if unknown). */
	FString KindOf(const FString& Id) const;

	/**
	 * Whether an event may fire right now given the context (wanted stars in range, district
	 * matches or unrestricted, and its own cooldown elapsed). The global gap is enforced
	 * separately by TriggerNext.
	 */
	bool CanFire(const FString& Id, double Now, const FAmbientContext& Context) const;

	/** Ids that could fire right now for the given context, in insertion order. */
	TArray<FString> EligibleIds(double Now, const FAmbientContext& Context) const;

	/**
	 * Pick and fire the next ambient encounter, or "" if the global gap hasn't passed or nothing
	 * is eligible. Marks the chosen event (and the global clock) as fired.
	 */
	FString TriggerNext(FRandomStream& Rng, double Now, const FAmbientContext& Context);

	/** Mirrors the reference `rng == null` early return: a null rng always yields "" and fires nothing. */
	FString TriggerNextNoRng();

	/** Force-mark an event as fired now (e.g. a scripted spawn), updating cooldowns. */
	void Trigger(const FString& Id, double Now);

	/** When an event last fired (-INF if never / unknown). */
	double LastFiredOf(const FString& Id) const;

	/** Clear all cooldowns (new game / chapter). */
	void Reset();

private:
	struct FEntry
	{
		FString Id;
		FString Kind;
		double Weight = 1.0;
		int32 MinStars = 0;
		int32 MaxStars = 5;
		FString District;
		double Cooldown = 60.0;
		double LastFired = 0.0;
	};

	/** Insertion-ordered storage (mirrors an insertion-ordered map). */
	TArray<FEntry> Entries;
	/** Id -> index into Entries. */
	TMap<FString, int32> Index;

	double LastAnyFired = 0.0;

	void Register(const FEventDef& Def);
	FString WeightedPick(FRandomStream& Rng, const TArray<FString>& Eligible) const;

	const FEntry* Find(const FString& Id) const;
	FEntry* Find(const FString& Id);
};
