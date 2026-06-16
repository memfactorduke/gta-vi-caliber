// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../Core/MissionFlow.h"

/**
 * A single mission: an ordered set of objectives and a state machine
 * (inactive -> active -> complete/failed). Pure and scene-free so the gameplay
 * rules unit-test headless.
 *
 * Godot parity: game/scripts/missions/.. game/scripts/world/mission.gd (class
 * MissionObjectives, RefCounted). This pure model is NOT on main and was NOT part
 * of the merged Wave 1 Missions/Core (Core merged the SINGLE-counted-objective
 * `Mission` from missions/mission.gd, a different file). It is parity-tested 1:1
 * against game/tests/unit/test_mission_objectives.gd (7 funcs).
 *
 * Plain C++ value type (RefCounted -> value), like the merged Missions/Core models.
 * The objective ROWS reuse the merged MissionFlow::FFlowObjective ({id,text,done}),
 * so MissionFlow sequencing/HUD/waypoint helpers consume them directly — merged
 * types are reused via #include, NOT re-ported. A UMissionController owns one of
 * these and drives it from world triggers + a timer (the scene wiring is Wave 3).
 */
class GTC_UE5_API FMissionObjectives
{
public:
    /** Inactive -> Active -> Complete/Failed. Mirrors Godot MissionObjectives.State. */
    enum class EObjectiveSetState : uint8
    {
        Inactive,
        Active,
        Complete,
        Failed
    };

    /** One objective definition fed to the constructor: {id, text}. */
    struct FObjectiveDef
    {
        FString Id;
        FString Text;
    };

    FString Title;
    EObjectiveSetState State = EObjectiveSetState::Inactive;
    /** Ordered objective rows ({id,text,done}); reuses the merged MissionFlow row. */
    TArray<MissionFlow::FFlowObjective> Objectives;

    FMissionObjectives() = default;

    /** Godot _init(title, objective_defs): build the {id,text,done=false} ordered set. */
    FMissionObjectives(const FString& MissionTitle, const TArray<FObjectiveDef>& ObjectiveDefs);

    /** Godot start(): only inactive -> active. */
    void Start();

    /**
     * Godot complete_objective(id): mark the first matching open objective done.
     * Returns true if this call changed it. Completing the last open objective
     * completes the mission. No-op (false) unless the set is active.
     */
    bool CompleteObjective(const FString& Id);

    /** Godot fail(): only active -> failed. */
    void Fail();

    bool IsActive() const { return State == EObjectiveSetState::Active; }
    bool IsComplete() const { return State == EObjectiveSetState::Complete; }

    /** Godot progress(): (done_count, total_count). */
    FIntPoint Progress() const;

private:
    /** Godot _all_done(): true once every objective is done; false for an empty set. */
    bool AllDone() const;
};
