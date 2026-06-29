// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A dormant lead's daily routine — the missing half of dual-protagonist switching.
 * FCharacterRoster parks each lead's wallet/wanted/position and resumes them
 * EXACTLY where you left them; but the genre's signature is switching back to find
 * the other lead living their own day, somewhere else, doing something. FLeadSchedule
 * is that: given a lead's routine and the current time of day, WHERE is this lead and
 * WHAT are they doing right now. The switch adapter queries it on switch-in to drop
 * the lead at a plausible haunt instead of the frozen spot, and it rides the
 * time-of-day cycle the world already runs.
 *
 * The model is a 24-hour ring of blocks. Each block has a start hour, a place the
 * lead hangs around during it, and an activity label; a block runs until the next
 * block's start, and the LAST block of the day wraps through midnight into the early
 * hours — so a lead with blocks at 06:00, 12:00 and 22:00 is at the 22:00 haunt from
 * 22:00 right through to 06:00. The routine is data-driven, so each lead gets a
 * believable, different day (one fishes at dawn, another closes the club at night)
 * without any of that coupling into this core.
 *
 * TimeUntilNextBlock lets the adapter schedule the next reposition / activity change
 * and feeds a "what's so-and-so up to" readout — the same anticipation shape as the
 * radio dial's and the traffic signal's countdowns.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject.
 * The same time of day always yields the same place + activity — unit-tested headless
 * (Tests/LeadScheduleTest.cpp, prefix GTC.Systems.Roster.Schedule).
 *
 * PURE-CORE boundary: spawning/repositioning the dormant lead actor at PlaceAt,
 * playing the activity animation, and reading the world clock is the DEFERRED adapter
 * and is NOT covered by the tests. Units: hours are in [0, 24); any real hour passed
 * to a query is folded modulo 24, so a raw world-seconds/3600 value is always safe.
 */
struct GTC_UE5_API FLeadSchedule
{
    /** One stretch of the day: from StartHour until the next block, the lead is here doing this. */
    struct FBlock
    {
        /** Hour the block begins, in [0, 24). A value outside the range is folded modulo 24. */
        double StartHour = 0.0;
        /** Where the lead hangs around during this block (world position). */
        FVector Place = FVector::ZeroVector;
        /** What the lead is doing, e.g. "fishing the pier", "closing the bar" — for the readout/anim. */
        FString Activity;
    };

    /** Hours in a day — the ring length the routine wraps around. */
    static constexpr double HoursPerDay = 24.0;

    /**
     * Install the routine. Each block's StartHour is folded into [0, 24), the blocks
     * are sorted by start hour, and a duplicate start hour keeps the first block given.
     * An empty list leaves the lead with no routine (queries return neutral).
     */
    void SetRoutine(TArray<FBlock> InBlocks);

    /** True when no routine is installed. */
    bool IsEmpty() const { return Blocks.Num() == 0; }

    /** Number of blocks in the routine. */
    int32 BlockCount() const { return Blocks.Num(); }

    /**
     * Index of the block active at time-of-day `Hours` (folded modulo 24), or
     * INDEX_NONE when the routine is empty. Before the first block's start, the day
     * is still in the previous day's last (overnight) block.
     */
    int32 BlockAt(double Hours) const;

    /** Where the lead is at `Hours` (FVector::ZeroVector when the routine is empty). */
    FVector PlaceAt(double Hours) const;

    /** What the lead is doing at `Hours` ("" when the routine is empty). */
    FString ActivityAt(double Hours) const;

    /**
     * Hours until the routine next changes block from `Hours` (folded modulo 24),
     * wrapping past midnight. 0 when the routine is empty. With a single block the day
     * never changes block, so this returns the time until that block's start next day.
     */
    double TimeUntilNextBlock(double Hours) const;

private:
    // Fold any real hour into [0, 24).
    static double WrapHours(double Hours);

    // Sorted by StartHour ascending.
    TArray<FBlock> Blocks;
};
