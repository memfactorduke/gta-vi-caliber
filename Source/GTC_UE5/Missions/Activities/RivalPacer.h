// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure pacing model for an AI RACE RIVAL — the half of a street race that decides
 * how fast an opponent driver progresses through the course over time. StreetRace
 * already CONSUMES rival progress (Placement / GapTo take rival-progress doubles in
 * 0..1) but nothing PRODUCES it: a session wrapping N StreetRaces would have to
 * hand-roll "given this rival's pace and the elapsed race time, how far along is
 * it?". This model is that decision, and only that decision.
 *
 * The base model is a target-pace clock: a rival whose solo race time is PaceSeconds
 * is at progress Elapsed / PaceSeconds, clamped to [0, 1]. That is deterministic
 * (a pure function of Elapsed and the rival's fixed pace), monotonic non-decreasing
 * (Elapsed only grows), and degenerate-safe (PaceSeconds <= 0 pins the rival at the
 * finish instead of dividing by zero).
 *
 * Rubber-banding keeps a race close WITHOUT cheating: the rival is nudged toward the
 * player's progress by a bounded fraction of the gap, and that nudge is hard-capped
 * at RubberBandMax (a few percent of the course). A leader is shaded back, a trailer
 * pulled up — but the cap means a rival can never teleport onto a player who is far
 * ahead, and Strength == 0 disables it entirely (honest pace clock). The result is
 * clamped to [0, 1] and can be floored at the rival's last progress so the adapter
 * always sees non-decreasing progress.
 *
 * Frame-free: this is a scalar progress model, not spatial. There is no XZ plane and
 * no road graph here — FTrafficAgent (a physics/IDM car on the road network) answers
 * a different question (where is a free-roaming car right now), not "how far through
 * its checkpoints should a race opponent be by now". Double precision throughout.
 *
 * PURE-CORE boundary: the adapter is deferred. A session adapter ticks the race,
 * calls ProgressAt() per rival each frame, turns the 0..1 result into a checkpoint
 * count with ProgressToCheckpoint(), and drives that rival's StreetRace via Reached()
 * — none of that engine/session wiring lives here and it is NOT covered by the tests.
 */
struct GTC_UE5_API FRivalPacer
{
    /** Default rubber-band gain: close ~30% of the player gap, before the cap. */
    static constexpr double DefaultRubberBandStrength = 0.3;

    /** Default hard cap on the rubber-band nudge (5% of the course, either way). */
    static constexpr double DefaultRubberBandMax = 0.05;

    /**
     * Base race progress (0..1) for a rival whose solo finish time is PaceSeconds,
     * after Elapsed seconds. Linear pace clock, clamped to [0, 1]. A non-positive
     * Elapsed yields 0 (the start line); a non-positive PaceSeconds is a degenerate
     * "infinitely fast" rival pinned at 1.0 rather than a divide-by-zero.
     */
    static double BaseProgress(double Elapsed, double PaceSeconds);

    /**
     * The rubber-band nudge applied to BaseProgress toward the player: Strength * the
     * signed gap (PlayerProgress - BaseRivalProgress), hard-clamped to
     * [-MaxAdjust, +MaxAdjust]. A non-positive Strength (off) or MaxAdjust (no room)
     * yields 0. Inputs outside [0, 1] are clamped first so the nudge stays bounded.
     */
    static double RubberBand(
        double BaseRivalProgress, double PlayerProgress, double Strength, double MaxAdjust);

    /**
     * A rival's race progress (0..1) after Elapsed seconds: the pace clock plus the
     * bounded rubber-band nudge toward PlayerProgress, clamped to [0, 1]. With
     * Strength == 0 this is exactly BaseProgress (an honest, player-independent clock).
     * The bounded nudge means a far-ahead player is never caught for free.
     */
    static double ProgressAt(
        double Elapsed,
        double PaceSeconds,
        double PlayerProgress,
        double Strength = DefaultRubberBandStrength,
        double MaxAdjust = DefaultRubberBandMax);

    /**
     * Same as ProgressAt but floored at PrevProgress, so the value the adapter feeds
     * back into StreetRace never goes backwards even if the player suddenly drops back
     * and the rubber-band would otherwise shade the rival down. PrevProgress is clamped
     * into [0, 1] first.
     */
    static double MonotonicProgressAt(
        double PrevProgress,
        double Elapsed,
        double PaceSeconds,
        double PlayerProgress,
        double Strength = DefaultRubberBandStrength,
        double MaxAdjust = DefaultRubberBandMax);

    /**
     * Map a 0..1 race progress to the count of checkpoints a rival should have
     * cleared on a course of TotalCheckpoints gates (checkpoints-per-lap * laps).
     * Floor of Progress * TotalCheckpoints, clamped to [0, TotalCheckpoints]. The
     * adapter calls StreetRace::Reached() until that rival's cleared count catches up.
     * A non-positive TotalCheckpoints yields 0.
     */
    static int32 ProgressToCheckpoint(double Progress, int32 TotalCheckpoints);
};
