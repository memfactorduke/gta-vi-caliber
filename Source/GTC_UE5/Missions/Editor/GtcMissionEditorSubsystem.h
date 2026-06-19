// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "GtcMissionDefinition.h"
#include "../Coordinators/MissionObjectiveDriver.h"
#include "GtcMissionEditorSubsystem.generated.h"

class UMissionController;
class UPlayerStatsComponent;
class UPlayerHealthComponent;
class AGtcMissionObjectiveMarker;
class FGtcJsonObject;

/** A compact, Blueprint-friendly row describing one authored mission (for list UIs). */
USTRUCT(BlueprintType)
struct GTC_UE5_API FGtcMissionSummary
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "GTC|MissionEditor")
    FGuid MissionId;

    UPROPERTY(BlueprintReadOnly, Category = "GTC|MissionEditor")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "GTC|MissionEditor")
    FString Title;

    UPROPERTY(BlueprintReadOnly, Category = "GTC|MissionEditor")
    EGtcMissionContentType ContentType = EGtcMissionContentType::MainMission;

    UPROPERTY(BlueprintReadOnly, Category = "GTC|MissionEditor")
    int32 ObjectiveCount = 0;

    /** False when the mission has validation errors (won't play). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|MissionEditor")
    bool bValid = true;
};

/**
 * UGtcMissionEditorSubsystem — the UObject layer of the in-game god-mode Mission Editor.
 *
 * It is a thin adapter: it owns the reflected bank of authored missions that the editor UI
 * binds to, exposes Blueprint-callable authoring/IO entry points, and forwards every
 * operation to the pure, headless-tested GtcMissionBank functions. No mission logic lives
 * here. A UWorldSubsystem (matching UGTCPlaceRegistrySubsystem) because later phases place
 * and select objective markers in the world; it reaches GameInstance subsystems
 * (controller, reward, progression) via GetGameInstance()->GetSubsystem<...>() at playback.
 */
UCLASS()
class GTC_UE5_API UGtcMissionEditorSubsystem : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // --- FTickableGameObject: auto-completes the active mission's reach/hold objectives ---
    // Ticks only while a mission is being played (Conditional + IsTickable), never in editor.
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return bPlaying || bTeardownPending || bWorldTriggersEnabled; }
    virtual bool IsTickableInEditor() const override { return false; }
    virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }

    /** True while the designer is in creator/god mode (toggled by input in a later phase). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|MissionEditor")
    bool bGodModeActive = false;

    /** The authored mission bank — reflected so the details panel / UI binds to it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|MissionEditor")
    TArray<FGtcMissionDefinition> Missions;

    /** Create a new mission of the given type; returns its stable id. */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    FGuid CreateMission(EGtcMissionContentType Type);

    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    bool AddObjective(FGuid MissionId, const FGtcObjectiveDefinition& Objective);

    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    bool RemoveObjective(FGuid MissionId, const FString& ObjectiveId);

    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    bool ReorderObjective(FGuid MissionId, int32 FromIndex, int32 ToIndex);

    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    bool RemoveMission(FGuid MissionId);

    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    int32 GetMissionCount() const { return Missions.Num(); }

    /** Validate a mission; returns true when there are no errors. Fills out errors/warnings. */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    bool ValidateMission(FGuid MissionId, TArray<FString>& OutErrors, TArray<FString>& OutWarnings) const;

    /** Write a mission to an openable .mission.json file. */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    bool SaveMissionToFile(FGuid MissionId, const FString& AbsolutePath) const;

    /** Load a .mission.json file into the bank (replacing one with the same id, else appending). */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    bool LoadMissionFromFile(const FString& AbsolutePath);

    /**
     * Validate, compile, and load an authored NARRATIVE mission (main mission / side quest)
     * into the live UMissionController, then start it. Returns false if the mission is
     * missing, invalid, an activity (those run through their own models — a later phase),
     * or no controller is available.
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    bool PlayMission(FGuid MissionId);

    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    void ToggleGodMode() { bGodModeActive = !bGodModeActive; }

    // --- God-mode authoring surface ----------------------------------------------------

    /** Compact rows for a list UI / the console `GTC.Mission.List` command. */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    TArray<FGtcMissionSummary> GetMissionSummaries() const;

    /** Set an objective's waypoint to a world location (god-mode "place here"); respawns markers. */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    bool SetObjectiveWaypoint(FGuid MissionId, const FString& ObjectiveId, FVector WorldLocation);

    /** Spawn a visible in-world marker at each placed objective of a mission. Returns the count. */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    int32 SpawnMarkersForMission(FGuid MissionId);

    /** Destroy all spawned objective markers. */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    void ClearObjectiveMarkers();

    // --- In-world mission discovery (the GTA "walk up to a blip to start" loop) ---------

    /**
     * Enable world start-markers: spawn a giver blip for every currently-available mission
     * (opted in, prerequisites met, not already done) and start it when the player walks in.
     * Stays enabled across missions (re-shows newly unlocked ones on completion). Returns count.
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    int32 ArmWorldTriggers();

    /** Disable world start-markers and remove their blips. */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    void DisarmWorldTriggers();

    /** Mark a mission as world-available with its start point at WorldLocation. */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    bool SetMissionStart(FGuid MissionId, FVector WorldLocation);

    /** Record a mission id as completed (gates prerequisite-locked missions). */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    void MarkMissionCompleted(FGuid MissionId) { CompletedMissions.Add(MissionId); }

    /** True if a mission has granted this feature/item id (persisted). Gameplay gates content on it. */
    UFUNCTION(BlueprintPure, Category = "GTC|MissionEditor")
    bool HasUnlock(const FString& FeatureId) const { return GrantedUnlocks.Contains(FeatureId); }

    /** All feature/item ids granted by completed missions (persisted in the savegame). */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    TArray<FString> GetUnlocks() const { return GrantedUnlocks.Array(); }

    /**
     * Advance the active Collect/Eliminate objective by Amount (call from combat/pickup code on a
     * confirmed kill or collected item). Completes the objective when it reaches its TargetCount.
     * Returns the remaining count, or -1 if no count objective is active. No-op unless playing.
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    int32 NotifyObjectiveProgress(int32 Amount = 1);

    /** Convenience: one kill/pickup toward the active count objective. */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    int32 NotifyKill() { return NotifyObjectiveProgress(1); }

    /**
     * Complete the active Interact objective (call when the player presses the interact button).
     * If the objective has a waypoint, the player must be within its radius. Returns true if it
     * completed one; no-op unless an Interact objective is active.
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    bool NotifyInteract();

private:
    // --- Reward payout wiring (active only while a played mission is running) ----------
    // Configures the existing UMissionReward from the authored reward, binds the controller
    // (so payout accrues), and on each objective/mission completion drains that payout into
    // the canonical wallet (UPlayerStatsComponent) and awards progression XP.
    void ConfigureAndBindRewards(const FGtcMissionDefinition& Mission, UMissionController* Controller);
    void UnbindRewards();
    void HandleObjectiveCompleted(const FString& ObjectiveId);
    void HandleMissionCompleted();
    void HandleMissionFailed();
    void DrainPayoutToWallet();
    UPlayerStatsComponent* FindPlayerStats() const;

    TWeakObjectPtr<UMissionController> RewardBoundController;
    FDelegateHandle RewardObjectiveHandle;
    FDelegateHandle RewardMissionHandle;
    FDelegateHandle RewardFailHandle;
    /** The authored headline lump added to the wallet on mission completion. */
    int32 ActiveHeadlineMoney = 0;

    /** Live god-mode markers (weak — they live in the world, destroyed on clear/teardown). */
    TArray<TWeakObjectPtr<AGtcMissionObjectiveMarker>> SpawnedMarkers;
    FGuid MarkedMissionId;

    // --- World-trigger state ---
    void SpawnGiverMarkers();
    void ClearGiverMarkers();
    void CheckGiverProximity();

    /** Sticky opt-in: the player enabled world start-markers (re-spawned after each mission). */
    bool bWorldTriggersEnabled = false;
    /** Currently-spawned giver blips (empty while a mission is running). */
    TArray<TWeakObjectPtr<AGtcMissionObjectiveMarker>> GiverMarkers;
    /** Missions completed this session (gates prerequisite-locked missions). */
    TSet<FGuid> CompletedMissions;
    /** Feature/item ids granted by completed missions' rewards (persisted, queryable via HasUnlock). */
    TSet<FString> GrantedUnlocks;
    /** The mission currently being played (recorded as completed on success). */
    FGuid PlayingMissionId;

    /** Running tally for the active Collect/Eliminate objective (reset when it changes/ends). */
    FString ActiveCountObjId;
    int32 ActiveCountProgress = 0;

    /** Elapsed time on the active Survive objective (reset when it changes/ends). */
    FString ActiveTimerObjId;
    double ActiveTimerElapsed = 0.0;

    /** Player pawn location this frame; false if there's no pawn. */
    bool GetPlayerLocation(FVector& OutLocation) const;

    /** The player's health component (for the death/fail poll), or null. */
    UPlayerHealthComponent* FindPlayerHealth() const;

    // Savegame persistence: write/restore the authored bank as a "mission_editor" section.
    void OnSaveMissions(const TSharedRef<FGtcJsonObject>& SectionOut);
    void OnLoadMissions(const TSharedRef<FGtcJsonObject>& SectionIn);

    /** True while a narrative mission is being auto-driven each frame. */
    bool bPlaying = false;

    /** Set when a mission ends; defers the delegate teardown to the next tick (out of any broadcast). */
    bool bTeardownPending = false;

    /** Whether we registered the savegame hook (only from a game world; unregister on teardown). */
    bool bSaveHookRegistered = false;

    /** Per-objective reach/hold satisfaction driver for the mission currently playing. */
    FMissionObjectiveDriver ObjectiveDriver;
};
