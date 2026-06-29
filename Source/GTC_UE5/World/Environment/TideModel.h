// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Coastal tides for the Miami waterfront. The Ocean stack already has a surface,
 * a shoreline blend and buoyancy, but the sea sits at one fixed datum — the water
 * never comes in or goes out. FTideModel is the slow rise and fall under all of
 * that: the still-water LEVEL the ocean surface rides on, as a function of the
 * world clock, so over a day the beach widens and narrows, boats sit higher or
 * lower against a dock, and a low tide exposes flats that a high tide drowns.
 *
 * It's built the way real tide tables are — a sum of harmonics — kept to the two
 * that matter for feel:
 *   - a fast SEMIDIURNAL term (~12.42 h) gives the familiar two highs and two lows
 *     a day; PhaseHours names a time of high tide so a level designer can anchor it.
 *   - a slow SPRING-NEAP ENVELOPE (~14.77 days) swells and shrinks that range, so
 *     some days swing hard (spring tides, sun and moon aligned) and some barely
 *     move (neap) — the same shape that makes real coastlines breathe over a month.
 *
 * Because there's no evolving state — the tide is a pure function of absolute time
 * — this core holds only the tuning and answers queries by the hours you hand it
 * (pass it the world time-of-day plus day count, or raw worldSeconds/3600). LevelAt
 * is the height offset; IsRisingAt says whether the water is coming in (a flood
 * tide) or going out (ebb); TimeToNextHigh/Low feed a "tide turns in 2 h" tell for
 * fishing/boating and the same anticipation shape as the rest of the world clocks.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject.
 * The same hour always yields the same level — unit-tested headless
 * (Tests/TideModelTest.cpp, prefix GTC.World.Tide).
 *
 * PURE-CORE boundary: feeding LevelAt into the ocean surface datum / shoreline
 * blend, floating docked boats on it, and reading the world clock is the DEFERRED
 * adapter and is NOT covered by the tests. Units: levels/amplitudes are in
 * centimetres (UE world units); all periods, phases and the query are in hours.
 */
struct GTC_UE5_API FTideModel
{
    /** Harmonic tuning for one stretch of coast. Defaults are a plausible ~1.6 m range. */
    struct FParams
    {
        /** The still-water datum the tide swings around (cm). */
        double MeanLevel = 0.0;
        /** Average semidiurnal amplitude — half the typical high-to-low range (cm). */
        double MeanAmplitude = 80.0;
        /** How much that amplitude grows/shrinks across the spring-neap cycle (cm).
         *  Keep |this| <= MeanAmplitude so the envelope stays positive and highs/lows
         *  line up with the semidiurnal extrema. */
        double SpringNeapRange = 30.0;
        /** Semidiurnal period (hours) — the principal lunar M2 constituent. */
        double PeriodHours = 12.42;
        /** Spring-neap envelope period (hours) — ~14.77 days between successive spring tides. */
        double SpringNeapHours = 354.4;
        /** Hour of a high tide — shifts the semidiurnal phase so high water lands when you want it. */
        double PhaseHours = 0.0;
        /** Hour of a spring (max-range) tide — shifts the envelope phase. */
        double SpringNeapPhaseHours = 0.0;
    };

    /** Install the tuning. With no call, the defaults above apply. */
    void Configure(const FParams& InParams) { Params = InParams; }

    /** The still-water level (cm) at world time `Hours`. MeanLevel when there's no period. */
    double LevelAt(double Hours) const;

    /**
     * The current semidiurnal envelope amplitude (cm) at `Hours` — large near a
     * spring tide, small near a neap. Clamped at 0. Lets the adapter surface
     * "spring tide today". */
    double AmplitudeAt(double Hours) const;

    /**
     * Where the level sits within the current envelope band, 0..1 (0 = this cycle's
     * low, 1 = its high). 0.5 when there's no period or a fully-cancelled envelope.
     */
    double NormalizedAt(double Hours) const;

    /** True when the water is coming in (flood tide) at `Hours`; false on the ebb or when flat. */
    bool IsRisingAt(double Hours) const;

    /**
     * Hours from `Hours` until the next high tide (the next semidiurnal peak). 0 when
     * there's no period. At an exact high, returns a full period (the high after this one).
     */
    double TimeToNextHigh(double Hours) const;

    /** Hours from `Hours` until the next low tide. 0 when there's no period. */
    double TimeToNextLow(double Hours) const;

private:
    // Time to the next time the semidiurnal phase hits `TargetRadians` (mod 2pi), in hours.
    double TimeToPhase(double Hours, double TargetRadians) const;

    FParams Params;
};
