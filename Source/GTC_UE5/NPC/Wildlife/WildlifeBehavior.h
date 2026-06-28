// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Florida fauna brain — the reflex that makes the wildlife in the city react to
 * the player instead of standing around. The crowd/NPC layer animates humans;
 * nothing yet drives the animals the setting is known for (gators on the canal
 * bank, pelicans on a piling, a stray dog on the boardwalk). FWildlifeBehavior is
 * that reflex: one deterministic state machine, branched by the creature's
 * temperament, that decides whether an animal idles, perks up, bolts, or charges.
 *
 * It runs off two things the adapter measures each tick — how far the nearest
 * threat (usually the player) is, and whether the animal was just provoked (shot
 * at / hit / cornered) — plus a per-species tuning block, so the SAME tested core
 * drives a skittish flamingo, an ambush gator, and an indifferent pelican without
 * a class per animal:
 *   - Placid     notices a threat (Idle -> Alert) but never escalates on its own;
 *                only a provoke makes it bolt.
 *   - Skittish   flees once the threat enters its personal space (ReactRange), and
 *                on any provoke.
 *   - Aggressive holds, then charges when the threat enters ReactRange, or on a
 *                provoke — and keeps charging until the threat has been gone a while.
 *
 * "Gone a while" is the calm-down: a threat counts as present while it's within
 * NoticeRange; once it's beyond that, a timer runs, and after CalmSeconds the
 * animal settles back to Idle from any state. That hysteresis (react close, calm
 * far, settle slow) keeps an animal from flickering when the player loiters at the
 * edge of its notice ring. CalmProgress feeds a "settling down" tell for the anim
 * adapter.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject.
 * The same inputs always yield the same state — unit-tested headless
 * (Tests/WildlifeBehaviorTest.cpp, prefix GTC.NPC.Wildlife).
 *
 * PURE-CORE boundary: measuring the distance to the player, deciding the flee/charge
 * MOVE each frame, and playing the hiss/bolt/lunge animation is the DEFERRED adapter
 * and is NOT covered by the tests. Units: distances are in centimetres (UE world
 * units); CalmSeconds and Dt are in seconds.
 */
struct GTC_UE5_API FWildlifeBehavior
{
    /** What kind of animal this is — picks which way Alert escalates. */
    enum class ETemperament : uint8
    {
        Placid,     // notices but won't escalate unless provoked (pelican, flamingo)
        Skittish,   // flees when crowded or provoked (most birds, a stray dog)
        Aggressive, // charges when crowded or provoked (gator, guard dog)
    };

    /** What the animal is doing right now — what the adapter animates / moves on. */
    enum class EState : uint8
    {
        Idle,    // grazing / loafing, unaware
        Alert,   // aware of the threat, weighing it
        Flee,    // running away (skittish / provoked placid)
        Charge,  // attacking the threat (aggressive)
    };

    /** Per-species tuning the adapter supplies. */
    struct FParams
    {
        ETemperament Temperament = ETemperament::Skittish;
        /** Distance at/under which the animal notices a threat (Idle -> Alert) and counts it "present". */
        double NoticeRange = 1500.0;
        /** Personal space: at/under this an Alert animal flees or charges. Clamped to <= NoticeRange. */
        double ReactRange = 600.0;
        /** Seconds the threat must stay beyond NoticeRange before the animal settles back to Idle. */
        double CalmSeconds = 4.0;
    };

    /**
     * Advance the brain one tick. `DistanceToThreat` is the centimetres to the
     * nearest threat (negative is treated as 0 — right on top of it). `bProvoked`
     * is true on a frame the animal was attacked/cornered and forces an escalation
     * this tick regardless of distance. `Dt` (seconds, clamped >= 0) only advances
     * the calm-down timer; the model is otherwise rate-free. Call once per frame and
     * read State() after.
     */
    void Update(double DistanceToThreat, bool bProvoked, double Dt, const FParams& Params);

    /** The behaviour state this tick. */
    EState State() const { return CurrentState; }

    /** Convenience: is the animal actively attacking (Charge)? */
    bool IsHostile() const { return CurrentState == EState::Charge; }

    /** Convenience: is the animal running away (Flee)? */
    bool IsFleeing() const { return CurrentState == EState::Flee; }

    /**
     * How far through the calm-down the animal is, 0..1 — 0 while the threat is
     * present, ramping to 1 over CalmSeconds once it's gone (at which point the next
     * Update settles to Idle). 1 when CalmSeconds is non-positive. Idle reports 0.
     */
    double CalmProgress(const FParams& Params) const;

private:
    EState CurrentState = EState::Idle;
    double CalmTimer = 0.0; // seconds the threat has been beyond NoticeRange
};
