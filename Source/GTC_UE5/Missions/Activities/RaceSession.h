// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "StreetRace.h"

/**
 * The RACE ORCHESTRATOR — the lifecycle layer that turns N independent StreetRace
 * checkpoint runs (the player plus a handful of AI rivals) into ONE playable race:
 * a countdown, a shared racing phase, a finish, and a ranked result tally. A single
 * StreetRace already knows how to advance gates, wrap laps, time splits, and finish;
 * what it does NOT know is that several racers are running the same track at once,
 * that nobody may cross a gate until the lights go green, or who actually won. That
 * multi-participant glue is FRaceSession.
 *
 * It closes a seam the rest of the activity stack was already built for: StreetRace
 * exposes Progress(), the static Placement()/GapTo() field helpers, and Reward(),
 * each with the comment that rival tracking + the result screen are "deferred to
 * W2/W3". This is that W2/W3 model, kept pure: own one StreetRace per participant
 * (index 0 is the player by convention, the rest are rivals), gate their gate-crossings
 * behind a countdown, fan a single Tick out to every participant's clock, and when the
 * field is in fold compute the ranked FResult set (placement + reward) by reusing the
 * StreetRace helpers rather than re-deriving placement logic.
 *
 * Three race phases, advanced one way only (no going backwards within a run):
 *   - Ready:    pre-grid. The countdown clock is ticking down; gate crossings are
 *               refused so nobody jumps the start. StartCountdown() (re)arms it.
 *   - Racing:   lights are green. Tick() accrues every participant's race clock and
 *               Reached(...) advances that participant's gates. Entered automatically
 *               when the countdown reaches zero, or immediately for a zero countdown.
 *   - Finished: every participant has either finished or been retired (DNF). Entered
 *               automatically once no participant is still running; Results() is then
 *               the final ranked tally.
 *
 * Ranking rule (FResult, best first): a participant that FINISHED outranks one that did
 * not; among finishers the lower Elapsed() wins; among non-finishers the greater
 * Progress() wins; a retired (DNF) participant is always last. Exact ties keep the lower
 * participant index ahead (stable), so the player (index 0) never loses a dead heat.
 * Placement in each FResult is its 1-based rank; Reward reuses StreetRace::Reward.
 *
 * PURE-CORE boundary: double precision, deterministic, no UObject, no scene. The
 * RaceController adapter (a Wave-3 actor) builds the FRaceSession from the placed
 * checkpoint ring, drives rival progress from their AI pawns, calls Reached(...) from
 * the player's checkpoint overlaps, forwards the world delta into Tick(...), and paints
 * Results() onto the HUD / awards the cash. None of that adapter is covered here.
 * Unit-tested headless (Missions/Activities/Tests/RaceSessionTest.cpp, prefix
 * GTC.Missions.Activities.RaceSession). Spatial convention matches StreetRace exactly:
 * the XZ ground plane (Y up), no Z-up remap.
 */
struct GTC_UE5_API FRaceSession
{
    /** The phase of the race lifecycle. Advances Ready -> Racing -> Finished only. */
    enum class EPhase : uint8
    {
        Ready,    // pre-grid: counting down, gate crossings refused
        Racing,   // green: clocks run, gates count
        Finished, // every participant finished or retired
    };

    /** One participant's final standing, produced by Results() (best first). */
    struct FResult
    {
        int32 ParticipantIndex = 0; // who this is (0 == player by convention)
        int32 Placement = 0;        // 1-based finishing position in the field
        bool bFinished = false;     // did they complete the race (vs. DNF / still short)
        double Progress = 0.0;      // overall race progress 0..1 at tally time
        double Elapsed = 0.0;       // their race clock (seconds) at tally time
        int32 Reward = 0;           // cash for this placement (0 for a DNF)
    };

    /**
     * Build a race for ParticipantCount racers over the shared Checkpoints/Laps,
     * with a CountdownSeconds pre-grid hold. ParticipantCount is floored at 0 (an
     * empty field is a degenerate race that is immediately Finished). Each participant
     * gets its own StreetRace over a copy of the same track. CountdownSeconds floors
     * at 0 (a zero countdown starts Racing at once — unless the track is degenerate,
     * in which case the race is Finished from the start). Laps floors at 1 (via
     * StreetRace). The player is participant index 0 by convention.
     */
    FRaceSession(int32 ParticipantCount, const TArray<FVector>& Checkpoints, int32 Laps = 1,
        double CountdownSeconds = 3.0, int32 BaseReward = 1000);

    /** Number of participants in the field (player + rivals). */
    int32 NumParticipants() const { return Racers.Num(); }

    /** Current lifecycle phase. */
    EPhase Phase() const { return RacePhase; }

    /** Seconds left on the pre-grid countdown (0 once racing/finished). */
    double CountdownRemaining() const { return RacePhase == EPhase::Ready ? Countdown : 0.0; }

    /** Convenience phase predicates. */
    bool IsReady() const { return RacePhase == EPhase::Ready; }
    bool IsRacing() const { return RacePhase == EPhase::Racing; }
    bool IsFinished() const { return RacePhase == EPhase::Finished; }

    /**
     * (Re)arm the pre-grid countdown to Seconds and return the field to Ready. Floors
     * Seconds at 0. A zero countdown over a real track drops straight to Racing; over a
     * degenerate (empty-checkpoint) track the race stays Finished. No-op once Finished
     * for a non-degenerate reason is NOT possible — StartCountdown always rewinds to the
     * grid, but a participant's own gate progress is left untouched (use Reset() to wipe
     * the whole race). Provided so an adapter can re-show the lights without rebuilding.
     */
    void StartCountdown(double Seconds);

    /**
     * Advance the session by Delta seconds. No-op for a non-positive Delta or once
     * Finished. While Ready it burns down the countdown (rolling any overshoot straight
     * into the first racing frame so a long Delta doesn't swallow race time); while
     * Racing it accrues every still-running participant's StreetRace clock. After either,
     * re-evaluates whether the whole field is in and flips to Finished if so.
     */
    void Tick(double Delta);

    /**
     * Report that participant Index crossed a gate at Pos within Radius (XZ). Only acts
     * while Racing and for a valid, still-running participant — returns false (no-op)
     * otherwise, including during the countdown so nobody jumps the start. Returns true
     * and advances that participant's gate when the crossing counts; flips the session to
     * Finished if that was the last racer still out.
     */
    bool Reached(int32 Index, const FVector& Pos, double Radius);

    /**
     * Force participant Index to retire (DNF): they stop being "still running" and tally
     * last among non-finishers regardless of their progress. No-op for an invalid index,
     * an already-finished participant, or once the session is Finished. Flips the session
     * to Finished if that was the last racer still out. Lets an adapter wreck/abandon a
     * rival without faking checkpoint hits.
     */
    bool Retire(int32 Index);

    /** Read-only view of a participant's underlying StreetRace (for HUD/progress). */
    const StreetRace& Participant(int32 Index) const { return Racers[Index]; }

    /** Overall race progress 0..1 for participant Index (0 for an invalid index). */
    double ProgressOf(int32 Index) const;

    /** Live 1-based placement of participant Index across the field right now (0 if invalid). */
    int32 PlacementOf(int32 Index) const;

    /**
     * The ranked result tally (best first), one FResult per participant. Valid in any
     * phase — mid-race it is the live standings; once Finished it is the final result with
     * settled rewards. Empty field yields an empty array. Reward is paid via
     * StreetRace::Reward(placement, BaseReward) for finishers and is 0 for a DNF.
     */
    TArray<FResult> Results() const;

    /**
     * Reset the whole race to the grid: every participant's StreetRace back to its start,
     * no retirements, countdown re-armed to its original length, phase recomputed (Ready,
     * or Finished for a degenerate/empty field).
     */
    void Reset();

private:
    /** Is participant Index still out on track (valid, not finished, not retired)? */
    bool IsRunning(int32 Index) const;

    /** True once no participant is still running (so the session should be Finished). */
    bool AllSettled() const;

    /**
     * Strict "A ranks ahead of B" by the ranking rule documented above (finish > clock >
     * progress; a DNF last). Symmetric ties (within epsilon) report false both ways and are
     * resolved by OutranksForSort's index break.
     */
    bool RanksAhead(int32 IndexA, int32 IndexB) const;

    /**
     * Total strict order used by BOTH Results() (the sort) and PlacementOf() (the count):
     * RanksAhead first, then the lower-index tie-break. Keeping one predicate means the live
     * placement and the final tally can never disagree.
     */
    bool OutranksForSort(int32 IndexA, int32 IndexB) const;

    static constexpr double Epsilon = 0.0001;

    TArray<StreetRace> Racers;     // one per participant; index 0 == player
    TArray<bool> Retired;          // parallel: participant has DNF'd
    EPhase RacePhase = EPhase::Ready;
    double Countdown = 0.0;        // seconds left on the pre-grid hold
    double InitialCountdown = 0.0; // countdown length to restore on Reset()
    int32 BaseReward = 0;          // 1st-place cash, scaled down by placement
};
