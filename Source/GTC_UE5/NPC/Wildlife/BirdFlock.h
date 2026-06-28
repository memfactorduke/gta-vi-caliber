// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A flock of birds, as ONE agent — the trailer's opening flourish. A shotgun goes
 * off, a hundred pigeons explode off the wires, wheel overhead, then resettle. That
 * beat doesn't need a hundred AIs: it needs one collective state machine the adapter
 * can hang a cheap instanced/Niagara bird cloud on. FBirdFlock is that machine. It's
 * the flock-level companion to FWildlifeBehavior (which is one animal's reflex) and
 * is NOT per-bird steering (that's FCrowdSteering); it answers only "is the flock on
 * the wire, bursting up, wheeling, or coming down, and how high is it right now".
 *
 * The phase ring is Perched -> Startled -> Wheeling -> Settling -> Perched. A startle
 * (gunfire, an explosion, the player barging in) launches a perched flock. After the
 * Startled burst they Wheel, circling for at least MinWheelSeconds. Once they've
 * wheeled long enough AND nothing has startled them for CalmBeforeSettle, they Settle
 * back down over SettleSeconds and re-perch. The hysteresis that sells it: a startle
 * WHILE wheeling just resets the calm timer (they're already up, keep circling), but
 * a startle WHILE settling re-spooks them — they abandon the descent and burst again.
 *
 * Altitude (0 on the wire, 1 wheeling overhead) is derived from the phase so the
 * adapter just lerps the cloud's height and spread: 0 perched, ramping up over the
 * burst, 1 while wheeling, ramping back down while settling.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject.
 * The same startles and deltas always yield the same phase — unit-tested headless
 * (Tests/BirdFlockTest.cpp, prefix GTC.NPC.Wildlife.Flock).
 *
 * PURE-CORE boundary: spawning the bird cloud, driving its height/spread from
 * Altitude, the wing-flap audio, and sourcing startles from the weapon/explosion
 * events is the DEFERRED adapter and is NOT covered by the tests. Units: all
 * durations and Dt are in seconds; Altitude is 0..1.
 */
struct FBirdFlock
{
    /** What the flock is collectively doing. */
    enum class EPhase : uint8
    {
        Perched,  // on the wire / ground, at rest
        Startled, // exploding upward off the perch
        Wheeling, // circling aloft
        Settling, // descending back toward the perch
    };

    /** Timing tuning for the flock. Defaults give a snappy burst and an unhurried settle. */
    struct FParams
    {
        /** Seconds of explosive takeoff before the flock settles into wheeling. */
        double BurstSeconds = 0.8;
        /** Minimum seconds spent wheeling before the flock is allowed to come down. */
        double MinWheelSeconds = 4.0;
        /** Seconds without a startle (after the min wheel) before the flock begins to settle. */
        double CalmBeforeSettle = 2.5;
        /** Seconds of descent from wheeling back to perched. */
        double SettleSeconds = 3.0;
    };

    /**
     * Advance the flock one tick. `bStartle` is true on a frame something spooks the
     * flock (gunshot/explosion/player proximity); `Dt` (seconds, clamped >= 0) drives
     * the phase timers and the calm-down clock. Call once per frame and read Phase()/
     * Altitude() after.
     */
    void Update(bool bStartle, double Dt, const FParams& Params);

    /** The flock's phase this tick. */
    EPhase Phase() const { return CurrentPhase; }

    /** True whenever the flock is off the perch (Startled, Wheeling, or Settling). */
    bool IsAloft() const { return CurrentPhase != EPhase::Perched; }

    /**
     * Flock height 0..1 — 0 perched, ramping up over the burst, 1 while wheeling,
     * ramping back down while settling. The adapter lerps the cloud's height/spread
     * from this.
     */
    double Altitude(const FParams& Params) const;

private:
    EPhase CurrentPhase = EPhase::Perched;
    double PhaseTime = 0.0; // seconds in the current phase
    double CalmTime = 0.0;  // seconds since the last startle
};
