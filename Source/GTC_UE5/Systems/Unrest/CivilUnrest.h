// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * District civil unrest — the tipping point between a tense street and a full riot.
 * CrowdPanic models a gunshot rippling fear through the crowd nearby; this is the
 * slower, area-wide thing underneath it: a district's DISORDER level, the kind of
 * pressure that builds from chaos and, past a point, boils over into a self-feeding
 * riot the police have to actually put down. FCivilUnrest is that one number and the
 * dynamic that makes it behave like real unrest.
 *
 * The model is deliberately BISTABLE — that's what separates unrest from every other
 * meter. Below a FLASHPOINT, disorder bleeds away on its own: a scuffle settles, the
 * street calms. Above it, the crowd feeds itself and unrest CLIMBS toward a full riot
 * no amount of waiting will settle. So the dynamic is one branch — grow while past
 * the flashpoint, decay while under it — and the consequence is hysteresis: Provoke
 * (a crime, an explosion, a brutal arrest) can shove a district over the line cheaply,
 * but once it's rioting only active Suppress (police presence) can force it back below
 * the flashpoint, after which it finally calms. Easy to start, hard to stop.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject.
 * The same provocations/suppressions over the same time always yield the same
 * district — unit-tested headless (Tests/CivilUnrestTest.cpp, prefix
 * GTC.Systems.Unrest).
 *
 * PURE-CORE boundary: turning crimes/explosions into Provoke and police presence into
 * Suppress, spawning the looters/brawls a riot implies, and the district HUD is the
 * DEFERRED adapter and is NOT covered by the tests. Units: Unrest and the thresholds
 * are 0..1; rates are per-second; Dt is seconds.
 */
struct FCivilUnrest
{
    /** Where a district sits on the disorder scale. */
    enum class EStage : uint8
    {
        Calm,    // business as usual
        Tense,   // simmering, but it'll settle on its own
        Rioting, // past the flashpoint — self-sustaining until put down
    };

    /** Tuning for how a district heats up and cools. */
    struct FParams
    {
        /** Unrest at/above which disorder becomes self-sustaining (the riot tips off). */
        double Flashpoint = 0.6;
        /** Unrest at/above which the street reads as Tense (below the flashpoint). */
        double TenseAt = 0.3;
        /** Unrest shed per second while BELOW the flashpoint (natural calming). */
        double DecayPerSec = 0.05;
        /** Unrest gained per second while AT/ABOVE the flashpoint (the riot feeding itself). */
        double GrowthPerSec = 0.04;
    };

    /** Install the tuning and reset the district to calm. */
    void Configure(const FParams& InParams);

    /** A provocation (crime, explosion, brutal arrest) of size `Amount` — adds unrest (clamped to 1). */
    void Provoke(double Amount);

    /** Police presence / dispersal of force `Amount` — removes unrest (clamped to 0). */
    void Suppress(double Amount);

    /**
     * Advance `Dt` seconds: unrest GROWS if it's at/above the flashpoint (self-sustaining
     * riot) or DECAYS if it's below (the street calming). A negative Dt is ignored.
     */
    void Advance(double Dt);

    /** The district's unrest, 0..1. */
    double Unrest() const { return UnrestValue; }

    /** The district's stage from the current unrest. */
    EStage Stage() const;

    /** True once unrest has tipped past the flashpoint into a self-feeding riot. */
    bool IsRioting() const;

private:
    FParams Params;
    double UnrestValue = 0.0;
};
