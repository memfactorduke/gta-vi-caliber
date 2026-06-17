// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PlayerProgression.h"
#include "ProgressionTracker.generated.h"

/**
 * Live respect/XP tracker — the UE 5.7 port of Godot's self-wiring ProgressionTracker
 * Node. Awards respect/XP as the player completes mission objectives and missions, so
 * levels climb during play, and exposes level()/level_progress() for the HUD.
 *
 * Lifecycle: a UGameInstanceSubsystem (progression is player-global and outlives any
 * single world/level, like the Godot autoload-adjacent group "progression"). It owns a
 * single FPlayerProgression for its whole lifetime.
 *
 * OWNERSHIP MODEL (approved design — option 1): ProgressionTracker OWNS the progression
 * state and exposes it via getters (Level/LevelProgress/TotalXp) and a const accessor to
 * the model. PlayerStats write-through is DEFERRED — this subsystem deliberately does not
 * depend on or reference any PlayerStats type, keeping the branch decoupled. The Godot
 * MissionController signal-wiring (objective_completed / mission_completed) is Wave 3
 * engine wiring; here the driver surface is the explicit AwardObjective()/AwardMission()
 * methods a caller (or future signal adapter) invokes.
 */
UCLASS()
class GTC_UE5_API UProgressionTracker : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** Respect granted per completed mission objective (Godot @export objective_xp). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
    int32 ObjectiveXp = 120;

    /** Respect granted per completed mission (Godot @export mission_xp). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
    int32 MissionXp = 600;

    // USubsystem lifecycle.
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // --- Drivers (events in -> model updates) ------------------------------

    /** A mission objective was completed: award ObjectiveXp. */
    void AwardObjective();

    /** A mission was completed: award MissionXp. */
    void AwardMission();

    // --- HUD getters (delegate to the owned model) -------------------------

    /** Current respect level, for the HUD. */
    int32 Level() const;

    /** Progress through the current level, 0..1, for a HUD bar. */
    double LevelProgress() const;

    /** Total respect earned, for the HUD / save. */
    int32 TotalXp() const;

    /** Const access to the owned pure model (ownership stays with the subsystem). */
    const FPlayerProgression& GetProgression() const { return _Progression; }

    // --- Persistence (SaveManager) -----------------------------------------

    /**
     * Snapshot: persists lifetime XP only ({"total_xp": TotalXp()}). Named
     * SerializeState (not Serialize) to avoid hiding UObject::Serialize.
     */
    int32 SerializeState() const;

    /**
     * Rebuild from a Serialize() snapshot: reset and replay lifetime XP through the
     * curve, reconstructing level and within-level progress exactly. Negative input is
     * floored to 0 (Godot maxi(..., 0)).
     */
    void Restore(int32 InTotalXp);

    /**
     * Garbage-restore path: Godot's restore() runs the value through SaveData.number_or,
     * so a non-number ("junk") becomes 0 — a clean reset. Mirrors that branch.
     */
    void RestoreGarbage();

private:
    FPlayerProgression _Progression;
};
