// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Street notoriety — the player's persistent infamy, the reputation that outlives any single
 * police chase. It is deliberately NOT the wanted system: FWantedLevel is transient police
 * heat that spikes on a crime and cools the moment you lie low or hit a pay-n-spray.
 * Notoriety instead accrues across a whole crime spree and bleeds away only slowly, and as it
 * climbs it draws a different kind of trouble — bounty hunters and rival-crew hit squads who
 * come looking for the price on your head even after the cops have given up. (It's also
 * distinct from FFactionStanding, which is per-faction reputation; this is one city-wide
 * infamy meter.)
 *
 * PURE-CORE: scene-free, deterministic, double precision, no engine coupling, no RNG. The
 * adapter calls Add() on notable crimes/kills/heists, Decay(dt) each frame, reads Tier()/
 * BountyAmount() for the HUD, and when HunterReady() spawns WaveSize() hunters then calls
 * TriggerWave() to start the cooldown. Times are seconds; bounty is the adapter's currency.
 * Unit-tested headless (Systems/Notoriety/Tests/NotorietyTest.cpp, prefix
 * GTC.Systems.Notoriety).
 */
enum class ENotorietyTier : uint8
{
    Anonymous, // a nobody — no bounty, no hunters
    Known,     // word's getting around
    Notorious, // the city knows your name
    Legendary, // every hunter in town wants the trophy
};

struct GTC_UE5_API FNotoriety
{
    /** Notoriety points lost per second while lying low. */
    static constexpr double DecayPerSec = 0.5;

    // Tier thresholds (points).
    static constexpr double KnownAt = 50.0;
    static constexpr double NotoriousAt = 150.0;
    static constexpr double LegendaryAt = 400.0;

    /** Bounty cash per notoriety point above the Known threshold. */
    static constexpr double BountyPerPoint = 20.0;

    /** Seconds between bounty-hunter waves. */
    static constexpr double HunterCooldownSec = 60.0;

    // Notoriety gained per notable act.
    static constexpr double ForKill = 10.0;
    static constexpr double ForCrime = 5.0;
    static constexpr double ForHeist = 40.0;

    double Notoriety = 0.0;   // current infamy points
    double HunterTimer = 0.0; // seconds until the next hunter wave may spawn

    /** Add infamy from a notable act (negatives ignored). */
    void Add(double Amount);

    /** Bleed infamy and tick the hunter cooldown down over `Dt` seconds. */
    void Decay(double Dt);

    /** Current infamy tier. */
    ENotorietyTier Tier() const;

    /** Cash on the player's head: 0 below Known, rising with infamy past it. */
    double BountyAmount() const;

    /** A hunter wave can spawn now (tier >= Known and the cooldown has elapsed). */
    bool HunterReady() const;

    /** How many hunters the current tier sends per wave. */
    int32 WaveSize() const;

    /** A wave just spawned — start the cooldown before the next. */
    void TriggerWave() { HunterTimer = HunterCooldownSec; }
};
