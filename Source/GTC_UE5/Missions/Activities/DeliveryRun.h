// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The timed delivery run — get the cargo there fast and in one piece. A courier
 * mission is three pressures at once: a COUNTDOWN (be quick), CARGO CONDITION that
 * crashes erode (drive clean), and ROUTE PROGRESS (actually arrive). FDeliveryRun is
 * that primitive, and it folds the run into a single scored payout so the activity
 * layer just reads the result.
 *
 * Two of the three pressures combine into the reward: leftover time and surviving
 * cargo. A flawless run — delivered with time to spare and the cargo intact — pays
 * full; a slow-but-careful run pays about half; a fast-but-battered one is docked in
 * proportion to the damage. Run out of time before arriving and it's a fail (too
 * slow); wreck the cargo to nothing and it's a fail (wrecked); reach the drop in time
 * with cargo left and it's delivered. The ordering that matters: arrival is checked
 * before the timeout on the same tick, so reaching the drop ON the buzzer still counts.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject. The
 * adapter feeds progress-toward-the-drop and any cargo damage each tick; the same
 * inputs always yield the same payout — unit-tested headless
 * (Tests/DeliveryRunTest.cpp, prefix GTC.Missions.Activities.Delivery).
 *
 * PURE-CORE boundary: measuring route progress, turning crashes into cargo Damage, the
 * delivery HUD/timer, and paying the reward is the DEFERRED adapter and is NOT covered
 * by the tests. Units: ProgressDelta/Progress and CargoCondition are 0..1; times and
 * Dt are seconds.
 */
struct FDeliveryRun
{
    /** How the run ends. */
    enum class EState : uint8
    {
        InProgress,
        Delivered, // reached the drop in time with cargo left
        TooSlow,   // ran out of time
        Wrecked,   // cargo destroyed
    };

    /** Per-run tuning. */
    struct FParams
    {
        /** Seconds allowed to reach the drop. */
        double TimeLimitSeconds = 120.0;
    };

    /** Install the tuning and start a fresh run (full time, full cargo, at the start). */
    void Configure(const FParams& InParams);

    /**
     * Advance one tick. `ProgressDelta` (>= 0) is the fraction of the route covered this
     * tick; `Damage` (>= 0) is cargo condition lost this tick (crashes). Wrecking the
     * cargo fails immediately; otherwise progress and the clock advance — arriving
     * (progress >= 1) delivers, and only if you haven't arrived does running the clock
     * out fail as too slow. A non-positive Dt is a no-op, and a no-op once the run ends.
     */
    void Update(double ProgressDelta, double Damage, double Dt);

    EState State() const { return CurrentState; }
    bool IsInProgress() const { return CurrentState == EState::InProgress; }
    bool IsDelivered() const { return CurrentState == EState::Delivered; }
    bool IsTooSlow() const { return CurrentState == EState::TooSlow; }
    bool IsWrecked() const { return CurrentState == EState::Wrecked; }

    /** Seconds left on the clock (0 once expired). */
    double TimeLeft() const { return TimeLeftValue; }

    /** Route covered so far, 0..1. */
    double Progress() const { return ProgressValue; }

    /** Cargo condition, 0..1 (0 = wrecked). */
    double CargoCondition() const { return CargoValue; }

    /**
     * Reward multiplier for a delivered run, 0..1: (0.5 + 0.5 * timeSpare) * cargo,
     * where timeSpare is the fraction of the limit left. 0 unless Delivered.
     */
    double PayoutFactor() const;

private:
    FParams Params;
    EState CurrentState = EState::InProgress;
    double TimeLeftValue = 0.0;
    double ProgressValue = 0.0;
    double CargoValue = 1.0;
};
