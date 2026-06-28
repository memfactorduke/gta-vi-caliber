// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Greasing a cop to cool the heat — the corrupt-officer escape valve. The Wanted
 * stack already models earning stars and evading line of sight (WantedSystem /
 * WantedEvasion); FBribeOffer is the other way down: when the heat's low enough,
 * slip the responding officer some cash and a star goes away. It's a producer FOR
 * the wanted system, not a part of it — it answers "can I bribe right now, what
 * does it cost, and what's the star level after", and the adapter charges the
 * player and sets the new heat.
 *
 * Two rules keep it from being pay-to-win. A bribe only works UP TO a heat ceiling
 * (MaxBribableStars) — once you're properly hunted no one will take the money. And
 * a GREED multiplier escalates the price for every bribe you've made recently, so
 * leaning on it again and again gets prohibitively expensive; going straight for
 * CooldownHours forgets the greed and resets the price.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject.
 * The same heat + cash + history always yields the same offer — unit-tested headless
 * (Tests/BribeOfferTest.cpp, prefix GTC.Systems.Wanted.Bribe).
 *
 * PURE-CORE boundary: reading the live wanted stars / player cash, charging the
 * deductible, lowering the WantedSubsystem heat, and the bribe interaction prompt is
 * the DEFERRED adapter and is NOT covered by the tests. Units: cost is money; Stars
 * are wanted stars; Advance Dt is in hours.
 */
struct FBribeOffer
{
    /** Tuning for what a bribe costs and when it's on the table. */
    struct FParams
    {
        /** Highest wanted level a bribe still works at; above this, no deal. */
        int32 MaxBribableStars = 3;
        /** Base cost per current star of heat. */
        double CostPerStar = 250.0;
        /** Each recent bribe raises the price by this fraction of the base. */
        double GreedPerBribe = 0.5;
        /** Hours of going straight that forget the greed and reset the price. */
        double CooldownHours = 6.0;
        /** Stars removed by a successful bribe. */
        int32 StarsReducedPerBribe = 1;
    };

    /** The outcome of a bribe attempt. */
    struct FResult
    {
        bool bAccepted = false; // did the officer take it
        double Cost = 0.0;      // what it cost (the quote, whether or not accepted)
        int32 StarsAfter = 0;   // resulting wanted level
    };

    /** Install the tuning and clear the greed history. */
    void Configure(const FParams& InParams);

    /**
     * Advance `Dt` hours of going straight. Once that idle time reaches CooldownHours,
     * the greed history is forgotten (the price resets). Any successful bribe restarts
     * the clock. A negative Dt is ignored.
     */
    void Advance(double Dt);

    /** What a bribe at `Stars` of heat costs right now, given the greed history (0 if not bribable). */
    double QuoteCost(int32 Stars) const;

    /** True if a bribe is on the table at `Stars` and affordable with `Cash`. */
    bool CanBribe(int32 Stars, double Cash) const;

    /**
     * Attempt to bribe at `Stars` of heat with `Cash` on hand. If accepted, returns the
     * cost and the lowered star level, spends a greed tick, and restarts the cooldown;
     * if not (too hot, no heat, or can't afford), bAccepted is false and the stars are
     * unchanged. The quoted Cost is filled in either way.
     */
    FResult Bribe(int32 Stars, double Cash);

    /** Successful bribes still counting toward the greed price. */
    int32 RecentBribes() const { return BribesInWindow; }

private:
    FParams Params;
    int32 BribesInWindow = 0;
    double IdleHours = 0.0;
};
