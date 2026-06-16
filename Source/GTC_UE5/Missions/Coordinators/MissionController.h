// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "../Core/MissionFlow.h"
#include "MissionObjectives.h"
#include "MissionController.generated.h"

/**
 * Drives one multi-step mission from world triggers and a timer, with a clean
 * retry path — the scene glue the M5 missions need beyond the kill-counter
 * MissionDirector.
 *
 * Godot parity: game/scripts/missions/mission_controller.gd (class
 * MissionController, Node; self-wires via group "mission"). This is the Wave 2
 * UE 5.7 port: a UGameInstanceSubsystem (the active mission is player-global, like
 * the Godot group "mission"). It has NO Godot parity oracle of its own EXCEPT the
 * accessors test_mission_objective_driver.gd::test_controller_reports_active_objective_id
 * pins (Begin / CurrentObjectiveId / Complete / IsComplete) — those four are oracle-
 * backed; the rest are BEHAVIOR (ownership / lifecycle) tests.
 *
 * OWNERSHIP MODEL (option-1 own-state):
 *   The controller OWNS an FMissionObjectives value member — the pure, parity-tested
 *   objective-set state machine (game/scripts/world/mission.gd: inactive -> active ->
 *   complete/failed, an ordered {id,text,done} set), extracted to its own file with
 *   its own 1:1 oracle (test_mission_objectives.gd, 7 funcs). That model was NOT part
 *   of the merged Wave 1 Missions/Core (Core merged the single-counted-objective
 *   `Mission`, the `MissionObjectiveTypes` predicates, `MissionFlow` sequencing, and
 *   `MissionChain`). Sequencing / HUD / waypoints / fail are delegated to the merged,
 *   parity-tested MissionFlow (#include of Missions/Core), not re-ported.
 *
 * DEFERRED to Wave 3 (engine wiring, not modelled here):
 *   - group "mission" self-registration, HUD polling, and the per-frame _process
 *     pump are scene wiring; here Tick(bPlayerDead, Delta) is a driver method a
 *     Wave 3 adapter feeds (the timer + fail logic is preserved headless).
 *   - _local_waypoints / FloatingOrigin offset conversion (the origin shift) is a
 *     Wave 3 concern; CurrentWaypoint here returns authored coordinates.
 *   - the player-death poll over group "player_health" becomes the bPlayerDead arg.
 */
UCLASS()
class GTC_UE5_API UMissionController : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnObjectiveCompleted, const FString& /*Id*/);
    DECLARE_MULTICAST_DELEGATE(FOnMissionCompleted);
    DECLARE_MULTICAST_DELEGATE(FOnMissionFailed);

    /** Per-objective completion (Godot signal objective_completed(id)). */
    FOnObjectiveCompleted OnObjectiveCompleted;
    /** Whole mission passed (Godot signal mission_completed). */
    FOnMissionCompleted OnMissionCompleted;
    /** Whole mission failed — timeout or death (Godot signal mission_failed). */
    FOnMissionFailed OnMissionFailed;

    /** HUD title (Godot @export title). */
    FString Title = TEXT("MISSION");

    /** Ordered objective sequence; each entry {Id, Text}. Godot @export objective_defs. */
    struct FObjectiveDef
    {
        FString Id;
        FString Text;
    };
    TArray<FObjectiveDef> ObjectiveDefs;

    /** Optional per-objective world waypoint id -> position. Godot @export waypoints. */
    TMap<FString, FVector> Waypoints;

    /** 0 = untimed; otherwise the mission fails when the clock runs out. Godot @export time_limit. */
    double TimeLimit = 0.0;

    // USubsystem lifecycle.
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Build + activate the objective set (Godot begin()). Idempotent on an already-active set. */
    void Begin();

    /**
     * Mark an objective done; emits OnObjectiveCompleted and completes the mission
     * when the last objective closes (Godot complete(id)). No-op when not active.
     */
    void Complete(const FString& Id);

    /** Fail the mission now (Godot fail()) — emits OnMissionFailed. No-op when not active. */
    void Fail();

    /** Rebuild and restart from scratch — the retry button (Godot reset()). */
    void Reset();

    bool IsActive() const { return Mission.IsActive(); }
    bool IsComplete() const { return Mission.IsComplete(); }

    /** HUD line for the current objective and progress (Godot hud_text()). */
    FString HudText() const;

    /** Id of the active (first not-done) objective, or "" when none remains (Godot current_objective_id()). */
    FString CurrentObjectiveId() const;

    /** World marker for the active objective, or Fallback (Godot current_waypoint()). */
    FVector CurrentWaypoint(const FVector& Fallback = FVector::ZeroVector) const;

    /**
     * Advance one frame of the timer + fail logic (the headless body of Godot
     * _process). bPlayerDead is PASSED IN (the Wave 3 adapter polls group
     * "player_health"). Returns true on the frame the mission fails.
     */
    bool Tick(bool bPlayerDead, double Delta);

    /** Seconds left on a timed mission (owned timer state). */
    double GetTimeLeft() const { return TimeLeft; }

private:
    /** Build the owned FMissionObjectives from ObjectiveDefs and arm the timer (Godot _build()). */
    void Build();

    /** Emit completed/failed once (Godot _finish()). */
    void Finish(bool bCompleted);

    /** Owned objective-set state machine — the extracted, parity-tested pure model. */
    FMissionObjectives Mission;
    bool bBuilt = false;

    /** Owned timer + end-latch (Godot _time_left / _ended). */
    double TimeLeft = 0.0;
    bool bEnded = false;
};
