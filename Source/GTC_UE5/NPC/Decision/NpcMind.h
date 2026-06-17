// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "NpcSchedule.h"

struct FNpcNeeds;

/**
 * The deliberative decision layer: it fuses the scripted daily routine
 * (FNpcSchedule) with live drives (FNpcNeeds) and personality, and emits a
 * single concrete intent — "go here and do this, because...". Sits *above* the
 * reflexive NpcBrain wander/flee FSM.
 *
 * Direct port of the Godot `NpcMind` (RefCounted) at
 * `game/scripts/npc/npc_mind.gd`. The routine is the baseline, but a drive that
 * gets desperate enough crosses the NPC's personal interrupt threshold and
 * hijacks the plan. Pure decision math (state in, intent out), scene-free,
 * unit-tested headless (Tests/NpcMindTest.cpp, prefix GTC.NPC.Decision.NpcMind).
 * Computed precision is `double` to match GDScript.
 */

/**
 * The chosen intent. Mirrors the Godot return dict {activity, place, reason,
 * urgency}. `Reason` is "schedule" or "need:<drive>" so debug HUD / dialogue can
 * explain the choice.
 */
struct GTC_UE5_API FNpcIntent
{
    FString Activity;
    FString Place;
    FString Reason;
    double Urgency = 0.0;
};

/**
 * One drive's resolution: the activity/place it sends the NPC to when it boils
 * over. Mirrors an entry of the Godot NEED_ACTIVITY dict.
 */
struct GTC_UE5_API FNpcNeedActivity
{
    FString Need;
    FString Activity;
    FString Place;
};

struct GTC_UE5_API FNpcMind
{
    /** Default urgency at which a drive overrides the schedule. Personality shifts it. */
    static constexpr double OverrideThreshold = 0.7;

    /**
     * Which activity / place resolves each drive when it boils over. Ordered to
     * mirror the Godot NEED_ACTIVITY dict (insertion order preserved).
     */
    static const TArray<FNpcNeedActivity>& NeedActivity();

    /**
     * Decide what this NPC does at `Hour` given its routine, drives and
     * personality.
     *
     * `Discipline` in [-1, 1] — how stubbornly it sticks to the schedule. A
     * disciplined NPC tolerates more discomfort before bailing; a flake bails at
     * the first twinge. Default 0 (the Godot personality dict default).
     *
     * Returns the chosen FNpcIntent.
     */
    static FNpcIntent Decide(
        const TArray<FNpcScheduleBlock>& Blocks,
        double Hour,
        const FNpcNeeds& Needs,
        double Discipline = 0.0);
};
