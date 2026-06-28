// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Money laundering — the wash cycle a heist needs. HeistCrew and the contraband
 * markets generate a pile of HOT cash, but there's no way to make it spendable;
 * it just lands in the wallet clean. FMoneyLaundering is the front that turns dirty
 * money into clean, the GTA-staple loop: drop a score into the wash, and it trickles
 * out clean over time, minus a cut.
 *
 * It behaves like a real front rather than a button. Cash sits in a dirty POOL with
 * a CAPACITY — you can't dump an unlimited score through one laundromat at once;
 * deposits past the ceiling are turned away. Each tick it launders at most a fixed
 * THROUGHPUT per hour, paying out (1 - Cut) of what it processes as clean cash and
 * keeping the rest as the fee. So a big haul washes out over in-game hours, not
 * instantly, and you always net less than you put in.
 *
 * The invariant that matters is conservation: every dirty dollar that leaves the
 * pool reappears as exactly clean + fee — the wash never mints or vanishes money.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject.
 * The same deposits over the same time always yield the same clean payout —
 * unit-tested headless (Tests/MoneyLaunderingTest.cpp, prefix
 * GTC.Systems.Economy.Laundering).
 *
 * PURE-CORE boundary: sourcing the dirty cash from heists/contraband, banking the
 * clean payout into PlayerStats, and the front's UI is the DEFERRED adapter and is
 * NOT covered by the tests. Units: all amounts are money; ThroughputPerHour is
 * money/hour; Advance Dt is in hours.
 */
struct FMoneyLaundering
{
    /** Tuning for the front's fee, speed, and how much it can hold. */
    struct FParams
    {
        /** Fraction of laundered cash kept as the fee (so clean = dirty * (1 - Cut)). */
        double Cut = 0.15;
        /** Maximum dirty cash laundered per hour. */
        double ThroughputPerHour = 1000.0;
        /** Maximum dirty cash the front can hold at once. */
        double Capacity = 50000.0;
    };

    /** Install the tuning. Existing balance is kept. */
    void Configure(const FParams& InParams) { Params = InParams; }

    /**
     * Drop dirty cash into the wash. Accepts up to the remaining capacity; returns the
     * amount actually taken (so the adapter knows how much of the player's hot cash was
     * consumed — the rest stays hot). A non-positive amount is a no-op.
     */
    double Deposit(double DirtyAmount);

    /**
     * Run the wash for `Dt` hours: launders up to ThroughputPerHour * Dt of the pool,
     * and returns the CLEAN cash produced (the laundered amount minus the cut). A
     * negative Dt is ignored. The fee is simply not paid out — money is conserved as
     * clean + fee.
     */
    double Advance(double Dt);

    /** Dirty cash still waiting to be laundered. */
    double DirtyBalance() const { return Dirty; }

    /** Remaining capacity for new deposits. */
    double SpaceRemaining() const;

    /** True when there's nothing left to launder. */
    bool IsIdle() const { return Dirty <= 0.0; }

    /** Total fees this front has skimmed over its life (for the books / UI). */
    double TotalFees() const { return FeesTaken; }

private:
    FParams Params;
    double Dirty = 0.0;
    double FeesTaken = 0.0;
};
