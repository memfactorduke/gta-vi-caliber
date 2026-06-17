// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure model for open-world-style STREET RACES — the checkpoint-lap races (drag/circuit
 * time-trials against rival drivers) that hand the player an ordered ring of
 * gates to clock through. A race is an ordered list of checkpoint points run
 * over N laps; the model tracks the current gate, lap count, progress, placement
 * against rivals, and race/lap timing.
 *
 * the reference parity: game/scripts/missions/street_race.gd (RefCounted). Two halves,
 * both scene-free: a STATEFUL race instance (checkpoint advance, lap wrap,
 * finish, timing) and a bank of STATIC helpers (placement, gap-to-rival, reward
 * by placement). Deterministic, unit-tested headless.
 *
 * Spatial parity: the reference reasons on the XZ ground plane (y is up) and that
 * convention is preserved verbatim — Ground() drops the y component. NO Z-up
 * remap is applied (literal parity with the the reference source).
 *
 * Plain C++ value type (no UObject): checkpoint actors / overlap volumes / HUD
 * wiring are deferred to W2/W3 and are NOT part of this parity port.
 */
class GTC_UE5_API StreetRace
{
public:
    /**
     * A race = ordered Checkpoints over Laps laps. An empty checkpoint list is a
     * degenerate race that starts already finished. Laps is floored at 1. Copies
     * the input array (the reference duplicate()), so later edits don't alias.
     */
    explicit StreetRace(const TArray<FVector>& Checkpoints, int32 Laps = 1);

    /** Drop the vertical component — racing reasons on the ground plane (XZ). */
    static FVector Ground(const FVector& V);

    /**
     * Did the racer hit the current checkpoint? Returns true and advances to the
     * next gate (wrapping into the next lap, recording the lap split) when Pos is
     * within Radius (XZ) of the current checkpoint. No-op + false when already
     * finished, when the race has no checkpoints, or when out of range.
     */
    bool Reached(const FVector& Pos, double Radius);

    /**
     * The checkpoint the racer is currently driving toward. Zero vector when the
     * race has no checkpoints or is finished.
     */
    FVector CurrentCheckpoint() const;

    /** Index of the current checkpoint within the current lap (0-based). */
    int32 CheckpointIndex() const;

    /**
     * 1-based lap the racer is currently on (1 == first lap). Reports TotalLaps()
     * once finished.
     */
    int32 CurrentLap() const;

    /** Total laps in the race. */
    int32 TotalLaps() const;

    /** Whether the race is complete (or a degenerate empty-checkpoint race). */
    bool IsFinished() const;

    /**
     * Overall progress across the whole race (checkpoints * laps), clamped 0..1.
     * 0 at the start, 1 once finished.
     */
    double Progress() const;

    /**
     * Checkpoints left to clear before the race is finished (across all remaining
     * laps). 0 once finished or when there are no checkpoints.
     */
    int32 CheckpointsRemaining() const;

    /**
     * Accrue race time by Delta seconds. No-op once finished or for a
     * non-positive delta. Drives Elapsed() and the lap-split clock.
     */
    void Tick(double Delta);

    /** Total race time accrued so far (seconds). */
    double Elapsed() const;

    /** Duration of the most recently completed lap, or 0.0 when none finished. */
    double LastLap() const;

    /** Fastest completed lap split, or 0.0 when no lap has finished. */
    double BestLap() const;

    /** Recorded lap splits in completion order (copy). */
    TArray<double> LapSplits() const;

    /**
     * Reset the race to its start: first checkpoint, lap 1, clock and splits
     * zeroed. A race with no checkpoints resets straight to finished.
     */
    void Reset();

    /**
     * Race placement (1 = first) of PlayerProgress among the field — count of
     * rivals strictly further along, plus one. Ties keep the player ahead
     * (stable). A DNF rival (negative progress) still places.
     */
    static int32 Placement(double PlayerProgress, const TArray<double>& RivalProgresses);

    /**
     * Distance the trailing racer is behind the one ahead, along a TrackLength
     * loop: (AheadProgress - MyProgress) * TrackLength, floored at 0. A
     * non-positive TrackLength or a racer already ahead yields 0.
     */
    static double GapTo(double AheadProgress, double MyProgress, double TrackLength);

    /**
     * Cash reward for finishing in Placement (1-based). 1st pays the full
     * BaseReward; each lower place scales down by a fixed step, floored at a
     * quarter of base. 0 for a DNF (placement <= 0) or a non-positive base.
     */
    static int32 Reward(int32 Placement, int32 BaseReward);

private:
    /**
     * Advance past the current checkpoint, wrapping into the next lap (recording
     * that lap's split) or finishing the race after the final gate.
     */
    void Advance();

    static constexpr double Epsilon = 0.0001;

    TArray<FVector> Checkpoints;
    int32 Laps = 1;
    int32 Index = 0;     // current checkpoint index within the current lap
    int32 Lap = 0;       // 0-based lap currently being driven
    bool bFinished = false;
    double ElapsedTime = 0.0;
    double LapStart = 0.0; // race time at which the current lap began
    TArray<double> LapSplitTimes; // completed lap durations, in order
};
