// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The reel-vs-line fight — the heart of a fishing minigame, for a game set on the
 * water. Once a fish is hooked, landing it is a tug of war you can lose two ways:
 * let the fish run until it spits the hook, or crank the reel so hard the line
 * snaps. FFishingFight is that fight: you feed a reel input each tick, the fish
 * fights back, and the line tension between you decides whether you land it.
 *
 * The fish's resistance is DERIVED from its stamina, not rolled — it fights hard
 * while fresh and tires over time — so the whole contest is a deterministic function
 * of your reel inputs alone. The skill lives in one balance: tension is your reel
 * effort PLUS the fish's pull PLUS a combo term for fighting it head-on, so cranking
 * hard while the fish pulls hard snaps the line, but reeling hard once it's worn out
 * is safe. Reel faster than the fish pulls and it comes in; reel too gently and it
 * takes line. Land it (distance to zero) before the line snaps and it's yours.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject.
 * The same reel inputs over the same deltas always yield the same catch — unit-tested
 * headless (Tests/FishingFightTest.cpp, prefix GTC.Systems.Fishing).
 *
 * PURE-CORE boundary: the cast/bite detection, the reel input from the stick, the
 * rod-bend / line VFX, and awarding the catch is the DEFERRED adapter and is NOT
 * covered by the tests. Units: Distance and Tension are normalized; ReelInput is
 * 0..1; Dt is seconds.
 */
struct FFishingFight
{
    /** How the fight ends. */
    enum class EOutcome : uint8
    {
        Fighting, // still on the line
        Landed,   // reeled all the way in
        Lost,     // the line snapped
    };

    /** Per-fish tuning. Defaults give a ~10s fight a careful angler wins. */
    struct FParams
    {
        /** Tension at which the line snaps. */
        double LineStrength = 1.0;
        /** Distance reeled in per second at full reel (before the fish resists). */
        double ReelRate = 0.25;
        /** Distance the fish takes per second at full pull. */
        double PullBack = 0.15;
        /** Tension contributed by full reel effort. */
        double TensionPerReel = 0.5;
        /** Tension contributed by a full-strength fish pull. */
        double TensionPerPull = 0.5;
        /** Extra tension when reeling AND the fish pulls (fighting it head-on). */
        double TensionCombo = 0.6;
        /** Fraction of the fish's stamina spent per second of fighting (it tires). */
        double StaminaDrainPerSec = 0.2;
    };

    /** Begin a fresh fight: fish at full distance, full stamina, slack line. */
    void Configure(const FParams& InParams);

    /**
     * Advance the fight one tick with `ReelInput` (clamped [0,1]) over `Dt` seconds
     * (clamped >= 0). Computes the line tension; snaps (Lost) if it exceeds LineStrength,
     * otherwise moves the fish by reel-vs-pull, tires it, and lands it (Landed) when it
     * reaches you. A no-op once the fight is over. Read Outcome()/Distance()/Tension().
     */
    void Update(double ReelInput, double Dt);

    EOutcome Outcome() const { return Result; }
    bool IsFighting() const { return Result == EOutcome::Fighting; }
    bool IsLanded() const { return Result == EOutcome::Landed; }
    bool IsLost() const { return Result == EOutcome::Lost; }

    /** Remaining distance to land the fish, 0 = at the boat. */
    double Distance() const { return DistanceValue; }

    /** The fish's current pull strength, 0..1 (drops as it tires) — the adapter picks reel from this. */
    double PullStrength() const { return FMath::Clamp(Stamina, 0.0, 1.0); }

    /** The line tension from the last Update, 0.. (>= LineStrength means it snapped). */
    double Tension() const { return TensionValue; }

private:
    FParams Params;
    EOutcome Result = EOutcome::Fighting;
    double DistanceValue = 1.0;
    double Stamina = 1.0;
    double TensionValue = 0.0;
};
