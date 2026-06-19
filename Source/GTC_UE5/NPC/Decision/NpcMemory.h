// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A citizen's short memory of mayhem it witnessed the player cause. Seeing a
 * neighbour gunned down burns in a memory that fades over a minute or so; while
 * it lingers and the culprit is in sight, the citizen recognises them and reacts
 * — the city remembers what you did, for a while.
 *
 * Direct port of the Godot `NpcMemory` (RefCounted) at
 * `game/scripts/npc/npc_memory.gd`. Pure and deterministic (intensity + time ->
 * state), unit-tested headless (Tests/NpcMemoryTest.cpp, prefix
 * GTC.NPC.Decision.NpcMemory).
 *
 * PURE-CORE boundary: the Citizen owning the mutable intensity and the
 * witness-bark humour (NpcDialogue) are the DEFERRED Wave-3 adapter and are NOT
 * covered by the parity tests. Computed precision is `double` to match GDScript.
 */
struct GTC_UE5_API FNpcMemory
{
    /** Above this, a citizen is openly rattled and will call the player out. */
    static constexpr double Alarmed = 0.6;
    /** Above this, it still remembers enough to recognise the player. */
    static constexpr double Uneasy = 0.25;

    /**
     * Fade a memory intensity toward 0 over `Dt` seconds. ~0.08/s ~= a 12 s fade
     * from a full scare to "uneasy", forgotten entirely in ~13 s.
     */
    static double Decay(double Memory, double Dt, double Rate = 0.08);

    /** A label for how vividly this is remembered: "alarmed" / "uneasy" / "none". */
    static FString Category(double Memory);

    /**
     * Does the citizen recognise the culprit right now? It must still remember
     * (at least "uneasy") and have the player within recognition range.
     */
    static bool Recognizes(double Memory, double PlayerDistance, double RangeM = 12.0);
};
