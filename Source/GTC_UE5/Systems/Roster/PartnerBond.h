// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The bond between the two leads — the relationship at the heart of a two-protagonist
 * story. FCharacterRoster switches between them and FLeadSchedule gives the dormant
 * one a life, but nothing tracks how the pair FEEL about each other: whether pulling
 * jobs together and bailing each other out has made them ride-or-die, or whether
 * leaving a partner in the lurch has worn the trust thin. FPartnerBond is that meter
 * — one number, 0..1, that co-op perks and story beats can gate on.
 *
 * It's shaped so the relationship feels earned and alive, not grindable:
 *   - WARMING has diminishing returns: a shared win raises the bond by a slice of the
 *     room still LEFT (scaled by 1 - bond), so it asymptotes toward 1 and the last
 *     stretch to ride-or-die is the hardest to earn — you can't spam favours to max it.
 *   - COOLING takes a flat chunk: a betrayal or an abandonment costs the same whether
 *     you were close or not, so trust is cheap to lose.
 *   - NEGLECT relaxes the bond toward a baseline: stop doing things together and a high
 *     bond drifts back down, while a damaged one slowly mends back up to neutral over
 *     time. The bond is a living value, not a ratchet.
 *
 * Crossing tier thresholds (Estranged -> Wary -> Solid -> RideOrDie) is what the
 * co-op/perk layer reads.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject.
 * The same interactions over the same time always yield the same bond — unit-tested
 * headless (Tests/PartnerBondTest.cpp, prefix GTC.Systems.Roster.Bond).
 *
 * PURE-CORE boundary: deciding what counts as a warming/cooling act (a co-op finish,
 * a revive, an abandonment) and scoring its strength, and unlocking perks on a tier,
 * is the DEFERRED adapter and is NOT covered by the tests. Units: the bond and tier
 * thresholds are 0..1; Strength is a dimensionless interaction weight; Advance Dt is
 * in hours.
 */
struct FPartnerBond
{
    /** Relationship tiers the co-op/perk layer gates on. */
    enum class ETier : uint8
    {
        Estranged, // barely speaking
        Wary,      // working together, not yet trusting
        Solid,     // a real partnership
        RideOrDie, // through anything
    };

    /** Tuning for how the bond grows, frays, and drifts. */
    struct FParams
    {
        /** Bond at the start of the story. */
        double StartBond = 0.2;
        /** The level neglect pulls the bond toward (up if below, down if above). */
        double Baseline = 0.2;
        /** Warming gain per unit Strength, before the diminishing (1 - bond) scaling. */
        double WarmGain = 0.15;
        /** Cooling loss per unit Strength (flat — a betrayal costs the same regardless of bond). */
        double CoolLoss = 0.20;
        /** Fraction of the gap to the baseline closed per idle hour. */
        double DecayPerHour = 0.01;

        /** Tier floors. */
        double WaryAt = 0.25;
        double SolidAt = 0.50;
        double RideOrDieAt = 0.80;
    };

    /** Install the tuning and set the bond to StartBond. */
    void Configure(const FParams& InParams);

    /**
     * A warming act (a shared win, a revive) of weight `Strength` (clamped >= 0).
     * Raises the bond by Strength * WarmGain * (1 - bond) — diminishing as the pair get
     * closer. Returns the actual change.
     */
    double Warm(double Strength);

    /**
     * A cooling act (a betrayal, an abandonment) of weight `Strength` (clamped >= 0).
     * Lowers the bond by a flat Strength * CoolLoss, floored at 0. Returns the actual
     * (negative) change.
     */
    double Cool(double Strength);

    /**
     * Advance `Dt` hours of neglect: the bond relaxes toward Baseline, closing
     * DecayPerHour of the remaining gap each hour. A negative Dt is ignored.
     */
    void Advance(double Dt);

    /** The current bond, 0..1. */
    double Bond() const { return BondValue; }

    /** The current relationship tier. */
    ETier Tier() const;

private:
    FParams Params;
    double BondValue = 0.2;
};
