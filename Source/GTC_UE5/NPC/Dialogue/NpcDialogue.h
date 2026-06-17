// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * The city's comic voice: procedural NPC barks. Given a citizen's voice (from
 * NpcArchetypes), what it's doing, and a seed, it returns a one-liner.
 *
 * Direct PURE-CORE port of the the reference `NpcDialogue` (RefCounted) at
 * `game/scripts/npc/npc_dialogue.gd`. Pure + deterministic: the same
 * (voice, context, seed) always yields the same line, unit-tested headless
 * (Tests/NpcDialogueTest.cpp, prefix GTC.NPC.Dialogue).
 *
 * Banks are keyed by voice then context; a missing voice/context falls back to a
 * generic bank, then to a last-resort murmur "...". Lines may contain {slots}
 * filled deterministically from absurd word banks. Selection is seed-driven via
 * the reference posmod (positive modulo) over ordered string arrays, so index order is
 * observable and preserved exactly (TArray, author order).
 *
 * DEFERRED — Wave 3 adapter (NOT here): bark_director.gd (Node, Label3D /
 * BaseMaterial3D speech-bubble rendering) and any Pedestrian / WeaponController
 * runtime wiring. This generator takes (voice, context, seed) as input only.
 */
struct GTC_UE5_API FNpcDialogue
{
	/** A recognition one-liner a rattled witness blurts. Deterministic, slot-filled, never empty. */
	static FString WitnessBark(int32 SeedValue);

	/** A weather one-liner in `Voice` for `Condition`. The "weather" voice gets a forecast. Never empty. */
	static FString WeatherBark(const FString& Voice, const FString& Condition, int32 SeedValue);

	/** Pick a line for (Voice, Context), filling slots. Falls back to generic, then "...". Never "". */
	static FString Bark(const FString& Voice, const FString& Context, int32 SeedValue);

	/** Convenience: bark for a brain decision, mapping its activity to a context. */
	static FString BarkForActivity(const FString& Voice, const FString& Activity, int32 SeedValue);

	/**
	 * The candidate lines for (Voice, Context) with generic fallback. Public so
	 * the spawner can pre-warm or count a voice's repertoire.
	 */
	static TArray<FString> LinesFor(const FString& Voice, const FString& Context);

	/** Word banks (exposed for parity tests that assert a slot resolves to a real entry). */
	static const TArray<FString>& Nouns();
	static const TArray<FString>& Places();
	static const TArray<FString>& Animals();
	static const TArray<FString>& Witness();

private:
	/** Resolve {slots} in a line deterministically from SeedValue. */
	static FString Fill(const FString& Line, int32 SeedValue);
};
