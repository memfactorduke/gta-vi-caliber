// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NpcSchedule.h"

/**
 * An INDIVIDUAL daily routine — one named person's day, not a shared archetype
 * template. Where FNpcArchetype carries a routine baked into C++ (one of four
 * templates shared across the whole census), this is a stand-alone, addressable
 * routine that a single citizen can be assigned, so two baristas can live genuinely
 * different days: one's an early riser who hits the gym at dawn, the other a night
 * owl who closes the bar. It's the unit the routine EDITOR authors, saves to JSON,
 * and hot-reloads onto live citizens.
 *
 * A routine is just an Id + a human label + the same ordered FNpcScheduleBlock list
 * FNpcMind/FNpcSchedule already consume — so an authored routine drops straight into
 * the existing decision loop with no new schedule semantics.
 *
 * PURE-CORE: scene-free, deterministic, no engine/UObject coupling. The JSON codec
 * (FNpcRoutineJson) and the live store/editor adapter (UGTCRoutineSubsystem) are
 * separate layers. Unit-tested headless (Tests/NpcRoutineTest.cpp, prefix
 * GTC.NPC.Decision.NpcRoutine).
 */
struct GTC_UE5_API FNpcRoutine
{
    /** Stable key the editor + per-citizen assignment reference (e.g.
     *  "barista_early_riser"). Lower-case, no spaces by convention. */
    FString Id;

    /** Human-readable label for the editor/HUD (e.g. "Early-Riser Barista"). */
    FString DisplayName;

    /** The day, as ordered [Start,End) blocks — first covering block wins, exactly
     *  like FNpcSchedule scanning. May wrap past midnight (Start > End for sleep). */
    TArray<FNpcScheduleBlock> Blocks;

    /** A routine is usable once it has an id and at least one block. */
    bool IsValid() const { return !Id.IsEmpty() && Blocks.Num() > 0; }
};

struct GTC_UE5_API FNpcRoutineLibrary
{
    /** Find a routine by id (case-insensitive), or nullptr if absent. */
    static const FNpcRoutine* Find(const TArray<FNpcRoutine>& Routines, const FString& Id);

    /** True if every hour of the 24h clock is covered by some block (no dead gaps
     *  where the citizen would fall back to generic idle). A "real" routine should. */
    static bool CoversFullDay(const FNpcRoutine& Routine);

    /** First overlap found between two blocks (ambiguous hours where an earlier block
     *  shadows a later one), or INDEX_NONE. Pure authoring lint for the editor. */
    static int32 FirstOverlap(const FNpcRoutine& Routine);

    /** Deterministically choose a routine id for a citizen from a pool, by seed —
     *  so a citizen with no explicit assignment still gets a STABLE individual
     *  routine (the same person every reload) rather than the flat archetype default.
     *  Returns empty if the pool is empty. */
    static FString PickIdForSeed(const TArray<FNpcRoutine>& Pool, int32 Seed);
};
