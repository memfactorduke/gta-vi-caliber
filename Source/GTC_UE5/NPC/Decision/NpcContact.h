// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * How a citizen reacts to being physically touched by the player: a brush, a
 * bump, a shove, or a real blow. This is the close-range counterpart to
 * FNpcReaction (which judges the player from a distance) — it turns a moment of
 * contact into a graded, human response. The whole point is that touching a
 * citizen is never nothing: someone you brush past says "excuse me", someone you
 * barge into turns and snaps at you, someone you shove stumbles, and someone you
 * strike either squares up or bolts — and the city remembers it.
 *
 * PURE-CORE: scene-free, deterministic, double precision, no engine/UObject
 * coupling — the same (impact, nerve) always yields the same verdict, so the
 * thresholds are unit-tested headless (Tests/NpcContactTest.cpp, prefix
 * GTC.NPC.Decision.NpcContact). The acting-on-the-verdict half (knockback,
 * montages, flee/memory wiring) lives in the AGTCCitizen adapter and is NOT
 * covered by these tests, exactly like its FNpcReaction / FNpcMemory siblings.
 */

/**
 * The dominant response to a contact, ordered by escalating intensity. `Ignore`
 * is the resting value the adapter holds between contacts; FNpcContact::Decide
 * never returns it (any registered contact gets at least an "excuse me").
 */
enum class ENpcContactReaction : uint8
{
    Ignore,    // no contact reaction active (adapter resting state)
    Excuse,    // a gentle brush — a polite "excuse me" and a small step aside
    Annoyed,   // a firmer bump — irritated, "watch it", reclaim the space
    Flinch,    // a startling contact — recoil a step, on guard, but holds
    Confront,  // provoked and brave — square up, square off, maybe shove back
    Flee,      // provoked and timid — turn and bolt
    Knockdown, // a flooring blow — knocked off their feet
};

struct GTC_UE5_API FNpcContact
{
    // --- Severity bands (0..1), classified from the physical contact alone ------

    /** At/below this it was a brush — barely contact at all. */
    static constexpr double GrazeMax = 0.12;
    /** Up to here it's a bump: you walked into them. */
    static constexpr double BumpMax = 0.32;
    /** Up to here it's a hard shove; above it a timid victim flees rather than flinches. */
    static constexpr double HardMax = 0.62;
    /** At/above this the contact floors them outright, brave or not. */
    static constexpr double FloorMin = 0.85;

    /** Bravery at/above this squares up to a serious contact instead of fleeing. */
    static constexpr double ConfrontNerve = 0.55;
    /** Alarm (memory/fear, 0..1) at/above this makes even light contact a jumpy flinch. */
    static constexpr double RattledThreshold = 0.5;

    // --- Severity tuning --------------------------------------------------------

    /** Body approach speed (m/s) that reads as a full-force barge before capping. */
    static constexpr double SlamSpeed = 14.0;
    /** A bare-handed body contact can stagger but never floor — capped just below FloorMin. */
    static constexpr double BodyMax = 0.80;
    /** A deliberate strike is serious even at a standstill; this is its floor. */
    static constexpr double StrikeBase = 0.6;
    /** Swing/closing speed (m/s) at which a strike reaches a flooring blow. */
    static constexpr double StrikeSwingSpeed = 10.0;

    // The fight/flight band must sit between a bump and the floor, and a bare body
    // contact (capped at BodyMax) must reach that band but never the knockdown
    // floor — so you provoke people by barging in, but can only floor them with a
    // real strike. Lock that ordering at compile time.
    static_assert(GrazeMax < BumpMax && BumpMax < HardMax && HardMax < BodyMax && BodyMax < FloorMin,
        "FNpcContact severity bands out of order");

    /**
     * Severity 0..1 of a contact. `ImpactSpeed` is the closing/approach speed in
     * m/s (for a body bump, the player's velocity component toward the citizen;
     * for a strike, the fist/weapon speed). `bStrike` marks a deliberate blow,
     * which is always serious and can reach a knockdown — a body bump cannot.
     */
    static double Severity(double ImpactSpeed, bool bStrike);

    /**
     * The dominant reaction, given the contact severity, the citizen's bravery
     * (0..1; fight-or-flight axis on serious contact), and how rattled they
     * already are (0..1 from lingering fear/memory; a jumpy citizen flinches at a
     * touch). Never returns Ignore.
     */
    static ENpcContactReaction Decide(double Severity, double Bravery, double Alarm);

    // --- Verdict classification (for the adapter) -------------------------------

    /** A hostile, stand-and-square-up reaction. */
    static bool IsHostile(ENpcContactReaction R) { return R == ENpcContactReaction::Confront; }
    /** A panic reaction that should route through the flee/threat machinery. */
    static bool IsPanic(ENpcContactReaction R)
    {
        return R == ENpcContactReaction::Flee || R == ENpcContactReaction::Knockdown;
    }
    /** Knocked to the ground — a stunned beat before scrambling up. */
    static bool IsKnockdown(ENpcContactReaction R) { return R == ENpcContactReaction::Knockdown; }
    /** True when a reaction is actually playing (anything but the resting Ignore). */
    static bool Reacts(ENpcContactReaction R) { return R != ENpcContactReaction::Ignore; }

    /** How long (s) the citizen stays in this reaction before resuming its day. */
    static double Duration(ENpcContactReaction R);

    /** Knockback impulse magnitude (cm/s) imparted to the body; 0 for no-shove reactions. */
    static double Knockback(ENpcContactReaction R, double Severity);

    /**
     * How much this contact burns into NpcMemory intensity (0..1). Being struck or
     * floored is vividly remembered (the citizen will recognise and call out the
     * culprit); a polite brush leaves no mark.
     */
    static double MemoryBump(ENpcContactReaction R, double Severity);
};
