// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A lock-tumbler minigame — the "find the sweet spot and turn it" tension a locked
 * door, a safe, or a hotwired car wants. The world has locked things but no skill
 * check to open them quietly; FLockpick is that check. You hunt a hidden sweet-spot
 * angle with the pick and apply tension to turn the lock — line it up and it opens,
 * force it off-spot and the pick snaps.
 *
 * It's one trade-off, every tick: tension converts into TURN PROGRESS scaled by how
 * well-aligned the pick is, and into PICK STRESS scaled by how badly you're forcing
 * it. So tension at the sweet spot turns the lock with zero stress, while cranking
 * off-spot snaps the pick. Letting tension off lets the stress recover — the "back
 * off and reposition" beat. The sweet spot stays hidden in the lock's data;
 * AlignmentAt only tells the adapter how warm the CURRENT angle is, for a haptic/
 * audio "getting closer" cue, so the secret's in the data and the skill's with the
 * player.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject.
 * The same angle/tension over the same deltas always yields the same result —
 * unit-tested headless (Tests/LockpickTest.cpp, prefix GTC.Systems.Lockpick).
 *
 * PURE-CORE boundary: mapping the stick to pick angle + tension, the rumble/SFX from
 * AlignmentAt, swapping in a broken pick, and opening the door is the DEFERRED
 * adapter and is NOT covered by the tests. Units: PickAngle/SweetSpot/Tolerance are
 * normalized 0..1; Tension and progress/stress are 0..1; rates are per-second; Dt is
 * seconds.
 */
struct FLockpick
{
    /** How the attempt ends. */
    enum class EOutcome : uint8
    {
        Picking,  // still working it
        Unlocked, // turned all the way
        Broken,   // snapped the pick
    };

    /** Per-lock tuning. Defaults give a ~1.3s pick once you're on the spot. */
    struct FParams
    {
        /** The hidden angle that turns the lock, 0..1. */
        double SweetSpot = 0.5;
        /** Half-width of the sweet spot: at the centre alignment is 1, beyond this it's 0. */
        double Tolerance = 0.08;
        /** Turn progress per second at perfect alignment and full tension. */
        double TurnRatePerSec = 0.8;
        /** Pick stress per second at zero alignment and full tension. */
        double StressRatePerSec = 1.5;
        /** Stress (0..1) at which the pick snaps. */
        double PickStrength = 1.0;
        /** Stress recovered per second while tension is released. */
        double RelaxRatePerSec = 2.0;
    };

    /** Begin a fresh attempt on a lock with the given tuning. */
    void Configure(const FParams& InParams);

    /**
     * Advance one tick with `PickAngle` (clamped [0,1]) and `Tension` (clamped [0,1])
     * over `Dt` seconds (clamped >= 0). Applying tension turns the lock by alignment and
     * stresses the pick by misalignment; releasing tension recovers stress. Unlocks at
     * full progress, breaks if stress exceeds PickStrength. A no-op once finished or for
     * a non-positive Dt.
     */
    void Update(double PickAngle, double Tension, double Dt);

    EOutcome Outcome() const { return Result; }
    bool IsPicking() const { return Result == EOutcome::Picking; }
    bool IsUnlocked() const { return Result == EOutcome::Unlocked; }
    bool IsBroken() const { return Result == EOutcome::Broken; }

    /** Turn progress so far, 0..1. */
    double Progress() const { return TurnProgress; }

    /** Pick stress so far, 0..1 (>= PickStrength means it snapped). */
    double Stress() const { return PickStress; }

    /** How aligned `Angle` is with the (hidden) sweet spot, 0..1 — for the adapter's "warmer" cue. */
    double AlignmentAt(double Angle) const;

private:
    FParams Params;
    EOutcome Result = EOutcome::Picking;
    double TurnProgress = 0.0;
    double PickStress = 0.0;
};
