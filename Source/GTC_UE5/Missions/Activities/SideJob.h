// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure model for open-world-style SIDE JOBS / CONTRACTS — the quick paid odd-jobs
 * (taxi fare, parcel delivery, vigilante hit, towing) that hand the player a
 * fast objective and cash without touching the main mission chain.
 *
 * the reference parity: game/scripts/missions/side_job.gd (RefCounted). Two halves,
 * both scene-free: a bank of STATIC reward/eligibility helpers (fare, vigilante
 * bounty, par-time bonus, combined payout, streak multiplier) callable with no
 * instance, and a small STATEFUL active-job tracker (pickup -> dropoff ->
 * complete) a node drives during play. Any randomness is the caller's job, so
 * the model is deterministic and unit-tested headless.
 *
 * Plain C++ value type (no UObject): the activity actor / spawn / HUD wiring is
 * deferred to W2/W3 and is NOT part of this parity port.
 *
 * Typed-port gap: the reference stores a job as an untyped Dictionary
 * ({kind, pickup, dropoff, base_reward}). Here that is modelled as the nested
 * value struct FJob (mirroring the dictionary verbatim).
 */
class GTC_UE5_API SideJob
{
public:
    enum class EKind : uint8
    {
        Taxi,
        Delivery,
        Vigilante,
        Towing
    };

    /** A job moves Pickup -> Dropoff; Done marks a finished/cleared tracker. */
    enum class EStage : uint8
    {
        Pickup,
        Dropoff,
        Done
    };

    /** Default cash per metre of trip for fare-style jobs. */
    static constexpr double DefaultPerMeter = 1.5;
    /** Default cash per eliminated target for vigilante contracts. */
    static constexpr int32 DefaultPerTarget = 150;
    /** Streak bonus never multiplies a payout past this (x2). */
    static constexpr double MaxChainMultiplier = 2.0;
    /** Extra multiplier earned per back-to-back completion before the cap. */
    static constexpr double ChainStep = 0.1;

    /**
     * One job definition. Mirrors the reference untyped job Dictionary
     * ({kind, pickup, dropoff, base_reward}).
     */
    struct FJob
    {
        int32 Kind = static_cast<int32>(EKind::Taxi);
        FVector Pickup = FVector::ZeroVector;
        FVector Dropoff = FVector::ZeroVector;
        int32 BaseReward = 0;
    };

    /** Build a job (handy for callers/tests; BaseReward floored at 0). */
    static FJob MakeJob(int32 Kind, const FVector& Pickup, const FVector& Dropoff, int32 BaseReward);

    /** Stable string id for a kind (HUD/save use), or "" for an unknown value. */
    static FString KindName(int32 Kind);

    /**
     * Fare for a trip: base plus per-metre distance pay, floored at the base and
     * never negative. Negative distance/inputs are clamped to 0.
     */
    static int32 Fare(double Distance, int32 BaseReward, double PerMeter);

    /** Vigilante bounty: base plus a flat sum per eliminated target (both clamped). */
    static int32 VigilanteReward(int32 Targets, int32 BaseReward, int32 PerTarget);

    /**
     * Bonus for beating par time: full Bonus at/under par, shrinking linearly to
     * 0 as you approach 2x par, and 0 once over par. Clamped non-negative. A
     * ParTime <= 0 is degenerate (no clock), so no bonus is awarded.
     */
    static int32 TimeBonus(double TimeTaken, double ParTime, int32 Bonus);

    /**
     * Total cash for a completed job, combining the per-kind core reward with the
     * par-time bonus. Never negative.
     */
    static int32 Payout(const FJob& Job, double Distance, double TimeTaken, double ParTime);

    /**
     * Streak multiplier for back-to-back completions: 1.0 for the first job, then
     * +ChainStep each consecutive one, capped at MaxChainMultiplier. Always >= 1.
     */
    static double ChainMultiplier(int32 ConsecutiveCompleted);

    // --- Stateful active-job tracker ---------------------------------------

    SideJob();

    /** Begin a job; it starts at the Pickup stage. Replaces any current job. */
    void Start(const FJob& Job);

    bool HasActive() const;

    /** Kind of the active job, or -1 when none is active. */
    int32 ActiveKind() const;

    EStage Stage() const;

    bool IsPickupDone() const;

    /** Move Pickup -> Dropoff. No-op when there is no active job. */
    void AdvanceStage();

    /** Finish the active job: tally it and clear the tracker. No-op when none active. */
    void Complete();

    /** Abort the active job without crediting it. No-op when none active. */
    void Cancel();

    int32 CompletedCount() const;

private:
    FJob Active;
    bool bActive = false;
    EStage CurrentStage = EStage::Done;
    int32 CompletedJobs = 0;
};
