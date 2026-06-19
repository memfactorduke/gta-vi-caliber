// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StatTracker.h"
#include "StatsCoordinator.generated.h"

/**
 * Live bridge for FStatTracker — the UE 5.7 port of the reference self-wiring StatsCoordinator
 * Node. Owns the pure FStatTracker model and records lifetime gameplay stats so
 * 100%-completion tracking accrues during play: missions passed off the mission system,
 * and "busts_evaded" when a wanted level rises then clears (a death-cleared wanted level
 * is NOT counted as an evasion). Exposes Stat()/CompletionPercent() for the HUD / pause
 * menu and Serialize/Restore for the save system.
 *
 * Lifecycle: a UGameInstanceSubsystem (stats are player-global, like the the reference group
 * "stats"). It owns a single FStatTracker for its whole lifetime.
 *
 * DEFERRED-OWNERSHIP (option-1 own-state): StatsCoordinator OWNS the model and takes
 * gameplay events as INPUT via explicit driver methods (OnMissionPassed / OnPlayerDied /
 * UpdateWanted). The the reference scene-graph wiring — resolving the "mission" / "player_health"
 * / "wanted" groups and binding mission_completed / died signals and polling is_wanted()
 * — is Wave 3 engine wiring. The death/evasion state machine (a wanted level that rises
 * then clears counts as one evasion, unless a death cleared it) is preserved here as the
 * UpdateWanted/OnPlayerDied drivers, so behavior stays headless-testable. The tracker
 * itself never touches armor/health/PlayerStats — abstract keyed counters only.
 */
UCLASS()
class GTC_UE5_API UStatsCoordinator : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // USubsystem lifecycle.
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // --- Event drivers (ported from the the reference Node's signal handlers / _process) ---

    /** A mission was passed: increments "missions_passed". Mirrors _on_mission_passed. */
    void OnMissionPassed();

    /**
     * The player died. If currently wanted, arms a flag so the wanted level that clears
     * on the ensuing death/respawn is NOT counted as an evasion. Mirrors _on_player_died.
     */
    void OnPlayerDied();

    /**
     * Feed the current wanted state (polled like Godot _process). A wanted level that
     * rises then clears counts as one "busts_evaded", unless a death cleared it (in which
     * case the death flag is consumed instead). Mirrors the _process body.
     */
    void UpdateWanted(bool bWanted);

    // --- Queries (delegate to the owned model) -----------------------------

    /** Read a lifetime stat (0 if never recorded), for the HUD / completion screen. */
    double Stat(const FString& StatId) const;

    /** Overall completion 0..100, for the pause-menu progress readout. */
    double CompletionPercent() const;

    /** Const access to the owned pure model (ownership stays with the subsystem). */
    const FStatTracker& GetStats() const { EnsureModel(); return *_Stats; }

    // --- Persistence (SaveManager) -----------------------------------------

    /** Snapshot the owned tracker as {id: value}. Named SerializeState to avoid hiding UObject::Serialize. */
    TArray<TPair<FString, double>> SerializeState() const;

    /** Restore tracker values from a snapshot and clear the wanted/death latch. */
    void Restore(const TArray<TPair<FString, double>>& Data);

private:
    /** Lazily allocate the owned model if not present. */
    void EnsureModel() const;

    mutable TUniquePtr<FStatTracker> _Stats;

    bool _bWasWanted = false;
    // Armed when the player dies while wanted: the wanted level that clears on the ensuing
    // death/respawn is NOT an evasion and must not be counted as one.
    bool _bDeathClearedWanted = false;
};
