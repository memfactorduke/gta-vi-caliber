// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure sequencing + fail-condition helpers layered on a mission's objective set.
 *
 * Godot parity: game/scripts/missions/mission_flow.gd (RefCounted, all-static).
 * MissionObjectives owns the objective set and done-flags; this decides which
 * objective is CURRENT (open-world games show one at a time with a waypoint), formats the HUD
 * line, and answers whether a fail condition (timer, player death) has tripped.
 * Static, scene-free, RNG-free — unit-tested headless.
 *
 * Typed-port gap: Godot passes an Array of {id, text, done} Dictionaries; here
 * that is FFlowObjective. Current() returns an empty (invalid) objective in place
 * of Godot's empty-{} sentinel. Waypoints are a TMap<FString,FVector> (Godot
 * id -> Vector3 Dictionary).
 */
class GTC_UE5_API MissionFlow
{
public:
    /** Sentinel returned when no objective is current (Godot NO_INDEX). */
    static constexpr int32 NoIndex = -1;

    /** One objective row: {id, text, done}. bValid marks the empty-{} sentinel. */
    struct FFlowObjective
    {
        FString Id;
        FString Text;
        bool bDone = false;
        bool bValid = false;

        FFlowObjective() = default;
        FFlowObjective(const FString& InId, const FString& InText, bool bInDone)
            : Id(InId)
            , Text(InText)
            , bDone(bInDone)
            , bValid(true)
        {
        }

        bool IsEmpty() const { return !bValid; }
    };

    /** Index of the first not-yet-done objective, or NoIndex if all done/empty. */
    static int32 CurrentIndex(const TArray<FFlowObjective>& Objectives);

    /** The current objective, or an empty (invalid) one when none remain. */
    static FFlowObjective Current(const TArray<FFlowObjective>& Objectives);

    /** Display text of the current objective, or "" when none remain. */
    static FString CurrentText(const TArray<FFlowObjective>& Objectives);

    /** How many objectives are done. */
    static int32 DoneCount(const TArray<FFlowObjective>& Objectives);

    /** One HUD line: "TITLE — <current> (k/n)", or "TITLE — complete (n/n)". */
    static FString HudLine(const FString& Title, const TArray<FFlowObjective>& Objectives);

    /** True once a timed mission's clock has run out (untimed never times out). */
    static bool TimedOut(double TimeLimit, double TimeLeft);

    /** Player death always fails; a timed mission also fails when the clock hits zero. */
    static bool ShouldFail(bool bPlayerDead, double TimeLimit, double TimeLeft);

    /**
     * World waypoint for the current objective from an id -> Vector map, falling
     * back to Fallback when there is no current objective or it has no mapped point.
     */
    static FVector CurrentWaypoint(
        const TArray<FFlowObjective>& Objectives,
        const TMap<FString, FVector>& Waypoints,
        const FVector& Fallback);
};
