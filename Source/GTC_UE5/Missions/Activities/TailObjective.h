// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The "tail the target" mission beat — follow without being made. The mission stack
 * has races and activities but no surveillance objective; FTailObjective is that one,
 * and it's failure-bounded on BOTH sides, which is what gives it tension. Drift too
 * far and you lose them; get too close (or wander into their view) and you're spotted.
 * The sweet band between is where the objective makes progress.
 *
 * The two failure modes are modelled differently because they FEEL different. "Lost"
 * is a hard timer: once you're past MaxDistance a grace clock runs, and if you don't
 * close the gap before it expires the target's gone. "Spotted" is a soft meter:
 * being too close or in the target's view raises suspicion, riding in the band lets
 * it cool, and only a FULL meter blows the tail — so you can dip in close briefly and
 * back off. Survive in the band for RequiredTrackSeconds and the tail succeeds.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject. The
 * adapter feeds it distance-to-target and whether the player is in the target's view
 * each tick; the same inputs always yield the same outcome — unit-tested headless
 * (Tests/TailObjectiveTest.cpp, prefix GTC.Missions.Activities.Tail).
 *
 * PURE-CORE boundary: measuring the distance, computing the target's view cone, the
 * "you're being followed" HUD, and the mission reward is the DEFERRED adapter and is
 * NOT covered by the tests. Units: Distance in cm; Suspicion/progress 0..1; times and
 * Dt in seconds.
 */
struct FTailObjective
{
    /** How the tail ends. */
    enum class EState : uint8
    {
        Tailing,       // still on them
        Succeeded,     // tracked long enough
        FailedLost,    // let them get too far away
        FailedSpotted, // got made
    };

    /** Tuning for the tail. */
    struct FParams
    {
        /** Closer than this and the target gets suspicious even if you're not in view (cm). */
        double MinDistance = 400.0;
        /** Farther than this and the "lost" grace clock starts (cm). */
        double MaxDistance = 3000.0;
        /** Seconds you can stay beyond MaxDistance before the target is lost. */
        double LostGraceSeconds = 4.0;
        /** Suspicion gained per second while too close or in the target's view. */
        double SuspicionRisePerSec = 0.4;
        /** Suspicion shed per second while safely in the band. */
        double SuspicionFallPerSec = 0.3;
        /** Seconds tracked within the band needed to complete the tail. */
        double RequiredTrackSeconds = 20.0;
    };

    /** Install the tuning and start the tail fresh. */
    void Configure(const FParams& InParams);

    /**
     * Advance one tick. `Distance` is to the target (cm); `bInTargetView` is true when
     * the player sits in the target's view. Beyond MaxDistance the lost clock runs (and
     * suspicion cools); within the band, being too close or in view raises suspicion
     * while safe riding cools it and accrues tracked time. Fails on a full suspicion
     * meter (spotted) or an expired lost clock, succeeds at RequiredTrackSeconds. A
     * non-positive Dt is a no-op, and a no-op once the tail is over.
     */
    void Update(double Distance, bool bInTargetView, double Dt);

    EState State() const { return CurrentState; }
    bool IsTailing() const { return CurrentState == EState::Tailing; }
    bool IsSucceeded() const { return CurrentState == EState::Succeeded; }
    bool IsLost() const { return CurrentState == EState::FailedLost; }
    bool IsSpotted() const { return CurrentState == EState::FailedSpotted; }

    /** How made the target is, 0..1 (1 = spotted). */
    double Suspicion() const { return SuspicionValue; }

    /** Progress toward completing the tail, 0..1. */
    double TrackProgress() const;

private:
    FParams Params;
    EState CurrentState = EState::Tailing;
    double SuspicionValue = 0.0;
    double TrackedTime = 0.0;
    double LostTimer = 0.0;
};
