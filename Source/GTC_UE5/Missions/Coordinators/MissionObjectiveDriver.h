// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "../Core/MissionObjectiveTypes.h"

/**
 * Completes a mission's ACTIVE objective from world state instead of Area3D
 * trigger geometry, so every campaign mission can have its own locations and
 * objective kinds without per-mission scene edits.
 *
 * Godot parity: game/scripts/missions/mission_objective_driver.gd (class
 * MissionObjectiveDriver, Node). The DECISION step (Evaluate) is pure, static and
 * 1:1 unit-tested against game/tests/unit/test_mission_objective_driver.gd (8
 * funcs); the node body is just the per-tick pump.
 *
 * Two objective KINDS, decided by the merged MissionObjectiveTypes predicates:
 *   "reach" — the player comes within radius of the objective's waypoint;
 *   "hold"  — the player must STAY within radius for duration seconds (leaving
 *             resets the clock) — the stake-out / meet beat.
 * An unknown kind degrades gracefully to "reach" so a typo'd def still works.
 *
 * OWNERSHIP / DEFERRED (option-1 own-state):
 *   This is a plain C++ value type (not a UObject) — the driver OWNS its pump
 *   coordination state (per-objective Defs, the running hold clock Held, and the
 *   armed objective id). It drives the merged, parity-tested MissionObjectiveTypes
 *   (#include of Missions/Core) and does NOT re-port those predicates.
 *   The Godot _physics_process scene pump (resolving the "player" Node3D from the
 *   SceneTree, reading the live MissionController's active objective/waypoint, and
 *   calling complete()) is ENGINE WIRING DEFERRED to Wave 3: here that wiring is a
 *   coordinator method (TickObjective) a Wave 3 adapter feeds player/target
 *   positions into, so the decision stays headless-testable. The mission_trigger
 *   Area3D volumes the driver supersedes are likewise DEFERRED to Wave 3.
 */
class GTC_UE5_API FMissionObjectiveDriver
{
public:
    /**
     * One objective definition: how this objective id completes.
     * Mirrors Godot's per-id Dictionary {kind, radius, duration}; defaults match
     * the Godot get() fallbacks (kind "reach", radius 6.0, duration 3.0).
     */
    struct FObjectiveDef
    {
        /** "reach" or "hold"; any other value degrades to "reach". */
        FString Kind = TEXT("reach");
        double Radius = 6.0;
        double Duration = 3.0;

        FObjectiveDef() = default;
        FObjectiveDef(const FString& InKind, double InRadius, double InDuration)
            : Kind(InKind)
            , Radius(InRadius)
            , Duration(InDuration)
        {
        }
    };

    /**
     * Result of one decision step: whether the objective is satisfied this frame,
     * and the new hold clock to carry forward. Mirrors Godot's
     * {"satisfied": bool, "held": float}; the compound return is split into named
     * fields (bSatisfied / Held).
     */
    struct FVerdict
    {
        bool bSatisfied = false;
        double Held = 0.0;
    };

    /**
     * Pure decision step (the unit-tested core). Given an objective def, the player
     * and target positions, the accumulated hold time and the frame delta, returns
     * {bSatisfied, Held} — the new hold clock to carry forward. Unknown kinds behave
     * as "reach" so a typo'd def degrades gracefully.
     *
     * Godot parity: MissionObjectiveDriver.evaluate (static).
     */
    static FVerdict Evaluate(
        const FObjectiveDef& Def,
        const FVector& PlayerPos,
        const FVector& Target,
        double Held,
        double Delta);

    // --- Owned pump coordination state (Godot Node body, sans SceneTree) --------

    /** Active objective id -> its def. Mirrors the Godot `defs` Dictionary. */
    TMap<FString, FObjectiveDef> Defs;

    /**
     * Re-arm the driver for a fresh mission: clears the running hold clock and the
     * armed objective so a leftover hold can't leak into the next mission. Mirrors
     * Godot bind(controller) (sans storing the controller Node — Wave 3 wiring).
     */
    void Bind();

    /**
     * Headless equivalent of one _physics_process frame for a single active
     * objective: looks up `ObjectiveId` in Defs, re-arms the hold clock when the
     * active objective changes, evaluates, stores the carried hold, and returns
     * true on the frame the objective is satisfied (the Godot complete() call site).
     * A Wave 3 scene adapter feeds PlayerPos/Target from the resolved player Node3D
     * and the live controller waypoint. An id absent from Defs returns false (the
     * Godot "no declared kind" early-out — scene triggers handle those).
     */
    bool TickObjective(
        const FString& ObjectiveId,
        const FVector& PlayerPos,
        const FVector& Target,
        double Delta);

    /** Current running hold clock (owned pump state, Godot `_held`). */
    double GetHeld() const { return Held; }

    /** Currently armed objective id (owned pump state, Godot `_armed_id`). */
    const FString& GetArmedId() const { return ArmedId; }

private:
    double Held = 0.0;
    FString ArmedId;
};
