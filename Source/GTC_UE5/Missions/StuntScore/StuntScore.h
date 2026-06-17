// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure freeform stunt-combo scorer — the open-world "string tricks together for a
 * multiplier, then land it clean to bank the points" loop. Plain C++ value type
 * (no UObject): a vehicle/stunt controller calls AddTrick() as tricks are detected
 * and Bank()/Wipe() on land/crash, then applies the returned score to the wallet
 * (CashFor) and PlayerProgression (RespectFor). Headless-testable (reference behavior
 * game/tests/unit/test_stunt_score.gd).
 *
 * Parity note: combo points are accumulated as double; banked/pending scores are
 * int32 via FMath::RoundToInt32 (the reference int(round())). TRICK_POINTS is iterated in
 * the reference insertion order via an ordered TArray for TrickKinds().
 */
class GTC_UE5_API FStuntScore
{
public:
    /** Multiplier rises by this per chained trick after the first... */
    static constexpr double MultStep = 0.5;
    /** ...capped here. */
    static constexpr double MaxMult = 5.0;

    /** Every recognised trick kind, in insertion order. */
    TArray<FString> TrickKinds() const;

    /**
     * Add a trick to the running combo. `Magnitude` scales the trick's base points.
     * Returns the raw points added (0 for an unknown kind or non-positive magnitude).
     */
    int32 AddTrick(const FString& Kind, double Magnitude);

    int32 ComboCount() const { return _ComboCount; }

    bool HasCombo() const { return _ComboCount > 0; }

    /** Current combo multiplier: 1.0 for a single trick, +MultStep per extra, capped. */
    double Multiplier() const;

    /** The score the current combo would bank right now: raw points times multiplier. */
    int32 PendingScore() const;

    /** Land it clean: bank the pending combo into the total, reset, return the banked score. */
    int32 Bank();

    /** Crash: lose the pending combo without banking. Returns the score forfeited. */
    int32 Wipe();

    int32 TotalScore() const { return _Total; }

    int32 BestCombo() const { return _Best; }

    /** Cash payout for a banked score (1:1 by default). */
    static int32 CashFor(int32 Score);

    /** Respect payout for a banked score (a fraction of the points). */
    static int32 RespectFor(int32 Score);

private:
    /** Base points per unit for one trick kind, kept in the reference insertion order. */
    struct FTrickPoints
    {
        FString Kind;
        double Points;
    };

    /** TRICK_POINTS in insertion order. */
    static const TArray<FTrickPoints>& TrickPoints();

    void ResetCombo();

    /** Raw points accumulated in the current (un-banked) combo. */
    double _ComboPoints = 0.0;
    /** Number of tricks in the current combo (drives the multiplier). */
    int32 _ComboCount = 0;
    /** Lifetime banked score. */
    int32 _Total = 0;
    /** Highest single combo ever banked. */
    int32 _Best = 0;
};
