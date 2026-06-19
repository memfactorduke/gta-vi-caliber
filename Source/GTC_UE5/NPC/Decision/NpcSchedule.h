// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * A citizen's daily routine: the scripted "where should I be and what should I
 * be doing right now" baseline, before needs and the world interrupt it.
 *
 * Direct port of the the reference `NpcSchedule` (RefCounted) at
 * `game/scripts/npc/npc_schedule.gd`. A routine is an ordered list of blocks
 * (FNpcScheduleBlock). `Start`/`End` are clock hours in [0, 24). A block may
 * wrap past midnight (Start > End, e.g. sleep 22.0 -> 6.0). Blocks are scanned
 * in order and the first one covering the hour wins, so author them
 * most-specific-first — insertion order is observable and preserved via the
 * ordered TArray backing store.
 *
 * All static + pure (clock hour in, block out), unit-tested headless
 * (Tests/NpcScheduleTest.cpp, prefix GTC.NPC.Decision.NpcSchedule). This is the
 * dependency that unblocks NPC archetypes (W3). Computed precision is `double`
 * to match the reference implementation.
 */

/**
 * One routine block: a half-open clock window [Start, End) and what the NPC
 * does there. the reference dict {"start","end","activity","place"} preserved as a
 * plain value type.
 */
struct GTC_UE5_API FNpcScheduleBlock
{
    double Start = 0.0;
    double End = 24.0;
    FString Activity;
    FString Place;

    FNpcScheduleBlock() = default;
    FNpcScheduleBlock(double InStart, double InEnd, const FString& InActivity, const FString& InPlace)
        : Start(InStart)
        , End(InEnd)
        , Activity(InActivity)
        , Place(InPlace)
    {
    }
};

struct GTC_UE5_API FNpcSchedule
{
    /**
     * The fallback when no block matches — an NPC with a gap in its day just
     * loiters. Mirrors the the reference `IDLE` const.
     */
    static FNpcScheduleBlock Idle();

    /** Wrap a clock value into [0, 24). */
    static double WrapHour(double Hour);

    /** Does `Block` cover `Hour`? Handles midnight-wrapping blocks (Start > End). */
    static bool BlockCovers(const FNpcScheduleBlock& Block, double Hour);

    /** The active block at `Hour`, or Idle() if the routine leaves a gap. */
    static FNpcScheduleBlock ActivityAt(const TArray<FNpcScheduleBlock>& Blocks, double Hour);

    /**
     * Hours from `Hour` until the given block ends (always positive, handles
     * wrap). Lets the brain know "you only have 20 minutes of lunch left" for
     * pacing.
     */
    static double HoursUntilEnd(const FNpcScheduleBlock& Block, double Hour);
};
