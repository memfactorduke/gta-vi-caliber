// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * How a citizen reacts to the player: ignore, gawk, or flee. Pure decision math
 * — distance + how threatening the player currently is + this NPC's own nerve.
 *
 * Direct port of the Godot `NpcReaction` (RefCounted) at
 * `game/scripts/npc/npc_reaction.gd`. Unit-tested headless
 * (Tests/NpcReactionTest.cpp, prefix GTC.NPC.Decision.NpcReaction). Threat is a
 * 0..1 read of the player right now; bravery and curiosity are per-NPC traits.
 *
 * PURE-CORE boundary: the agent acting on the verb (locomotion / panic spread
 * wiring) is the DEFERRED Wave-3 adapter and is NOT covered by the parity tests.
 * Computed precision is `double` to match GDScript.
 */
struct GTC_UE5_API FNpcReaction
{
    /** Past this range the player is just scenery; the NPC carries on with its day. */
    static constexpr double NoticeRadius = 14.0;
    /** Inside this the NPC feels crowded even by a harmless passer-by. */
    static constexpr double PersonalRadius = 3.5;
    /** A panicking citizen this close is contagious — terror spreads through a crowd. */
    static constexpr double PanicRadius = 6.0;
    /** Bravery at/above this shrugs off a neighbour's panic (the steady ones hold). */
    static constexpr double PanicNerve = 0.85;

    /**
     * Build a 0..1 threat score from the player's state. Brandishing a weapon is
     * the big term; charging at speed adds to it. Saturates at 1.
     */
    static double ThreatFrom(double PlayerSpeed, bool bArmed);

    /**
     * Pick a reaction verb: "ignore", "gawk", or "flee".
     *   Distance  — metres from player to NPC
     *   Threat    — 0..1 from ThreatFrom()
     *   Bravery   — 0..1 trait; higher resists fear
     *   Curiosity — 0..1 trait; higher leans in to gawk
     */
    static FString Decide(double Distance, double Threat, double Bravery, double Curiosity);

    /**
     * Does this NPC catch a nearby citizen's panic? Close terror is contagious,
     * but the steady-nerved (bravery >= PanicNerve) hold their ground. This turns
     * one gunshot into a whole street emptying.
     */
    static bool CatchesPanic(double Distance, double Bravery);
};
