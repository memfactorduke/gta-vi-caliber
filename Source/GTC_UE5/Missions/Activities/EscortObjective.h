// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The escort / protect-the-convoy mission beat. The tail (FTailObjective) is about
 * NOT being seen and the delivery (FDeliveryRun) is about the clock; the escort is
 * about keeping someone else alive to the destination. FEscortObjective is that
 * primitive: an escortee with INTEGRITY that bleeds under fire, plus ROUTE PROGRESS
 * toward the drop-off.
 *
 * What makes it an escort rather than a health bar is the push-pull. The escortee's
 * integrity drains in proportion to the current THREAT LEVEL (how much heat is on it
 * right now), but once you've SUPPRESSED the attackers — threat near zero — it slowly
 * recovers. So the player's job is to actively clear the heat, not just out-run it,
 * and a convoy that looked doomed can be saved by killing the right car. Reach the
 * destination with integrity left and it's delivered; let integrity hit zero and the
 * escortee is lost.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject. The
 * adapter feeds route progress and a threat level (cooked from nearby hostiles) each
 * tick; the same inputs always yield the same outcome — unit-tested headless
 * (Tests/EscortObjectiveTest.cpp, prefix GTC.Missions.Activities.Escort).
 *
 * PURE-CORE boundary: measuring the escortee's route progress, computing the threat
 * level from attackers, applying damage to the escortee actor, and the mission reward
 * is the DEFERRED adapter and is NOT covered by the tests. Units: Progress/Integrity/
 * ThreatLevel are 0..1; rates are per-second; Dt is seconds.
 */
struct FEscortObjective
{
    /** How the escort ends. */
    enum class EState : uint8
    {
        Escorting,
        Delivered, // reached the destination with integrity left
        Lost,      // the escortee was destroyed
    };

    /** Tuning for the escort. */
    struct FParams
    {
        /** Integrity lost per second at full threat. */
        double DrainPerSec = 0.25;
        /** Integrity recovered per second while the threat is suppressed. */
        double RegenPerSec = 0.1;
        /** Threat at/below which the escortee is "safe" and starts recovering. */
        double RegenThreatThreshold = 0.05;
    };

    /** Install the tuning and start the escort fresh (full integrity, at the start). */
    void Configure(const FParams& InParams);

    /**
     * Advance one tick. `ProgressDelta` (>= 0) is the route fraction the escortee
     * covered this tick; `ThreatLevel` (clamped 0..1) is the heat on it right now —
     * above the regen threshold it drains integrity in proportion, at/below it the
     * escortee recovers. Integrity reaching 0 loses the escort; reaching the
     * destination (progress >= 1) with integrity left delivers it. A non-positive Dt
     * is a no-op, and a no-op once the escort ends.
     */
    void Update(double ProgressDelta, double ThreatLevel, double Dt);

    EState State() const { return CurrentState; }
    bool IsEscorting() const { return CurrentState == EState::Escorting; }
    bool IsDelivered() const { return CurrentState == EState::Delivered; }
    bool IsLost() const { return CurrentState == EState::Lost; }

    /** Escortee integrity, 0..1 (0 = destroyed). */
    double Integrity() const { return IntegrityValue; }

    /** Route covered so far, 0..1. */
    double Progress() const { return ProgressValue; }

private:
    FParams Params;
    EState CurrentState = EState::Escorting;
    double IntegrityValue = 1.0;
    double ProgressValue = 0.0;
};
