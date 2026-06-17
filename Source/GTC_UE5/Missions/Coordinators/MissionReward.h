// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MissionController.h"
#include "MissionReward.generated.h"

/**
 * Makes the economy live: pays the player for mission progress so the money HUD
 * actually moves. Each completed objective credits a small reward and finishing a
 * mission pays a bonus.
 *
 * Godot parity: game/scripts/missions/mission_reward.gd (class MissionReward,
 * Node; self-wires via group "mission" + group "player_stats"). This is the Wave 2
 * UE 5.7 port: a UGameInstanceSubsystem (rewards are player-global). NO Godot
 * parity oracle — its tests are BEHAVIOR (ownership / lifecycle / wiring to a
 * controller's signals), NOT parity.
 *
 * DEFERRED-OWNERSHIP (option-1 own-state, lead-signed):
 *   Godot's _pay() reaches the live PlayerStats Node (group "player_stats") and
 *   calls add_money(). Here the reward does NOT write through to any PlayerStats
 *   type — none is included or depended on. Instead this subsystem OWNS its payout
 *   state: it accumulates the reward into an owned wallet snapshot (SetWallet /
 *   GetWallet) and exposes a drainable reward INTENT (GetPendingPayout /
 *   ConsumePendingPayout). At Wave 3 the canonical money owner is
 *   UPlayerStatsComponent (per docs/W3_WIRING_NOTES.md, commit f0c0c36 — Stats is
 *   the canonical money owner); the Wave 3 wiring drains the pending payout intent
 *   into UPlayerStatsComponent::AddMoney. No PlayerStats type is included now.
 *
 *   Self-wiring by group ("mission"/"player_stats" resolution) is Wave 3 scene
 *   wiring; here BindController() connects to a UMissionController's delegates and
 *   ResolveController-from-group is deferred.
 */
UCLASS()
class GTC_UE5_API UMissionReward : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** Cash per completed objective (Godot @export objective_reward). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MissionReward")
    int32 ObjectiveReward = 250;

    /** Cash bonus for finishing a mission (Godot @export mission_bonus). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MissionReward")
    int32 MissionBonus = 1000;

    // USubsystem lifecycle.
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * Subscribe to a controller's objective/mission completion (Godot _ready's
     * connect() to the group-"mission" node). A Wave 3 adapter resolves and passes
     * the live controller; unbinds any prior controller first.
     */
    void BindController(UMissionController* Controller);

    /** Drop the current controller subscription (e.g. mission teardown). */
    void UnbindController();

    // --- Owned payout snapshot (deferred PlayerStats coupling) -----------------

    /** Set the owned wallet snapshot rewards accrue into. */
    UFUNCTION(BlueprintCallable, Category = "MissionReward")
    void SetWallet(int32 InWallet) { Wallet = FMath::Max(InWallet, 0); }

    /** The owned wallet snapshot (after rewards already credited). */
    int32 GetWallet() const { return Wallet; }

    /**
     * Total reward credited but NOT yet drained into the canonical
     * UPlayerStatsComponent (the reward INTENT the Wave 3 adapter applies).
     */
    UFUNCTION(BlueprintCallable, Category = "MissionReward")
    int32 GetPendingPayout() const { return PendingPayout; }

    /** Drain the pending payout intent (returns the amount, then zeroes it). */
    UFUNCTION(BlueprintCallable, Category = "MissionReward")
    int32 ConsumePendingPayout()
    {
        const int32 Amount = PendingPayout;
        PendingPayout = 0;
        return Amount;
    }

    /** Last reward credited (objective or mission bonus). */
    int32 GetLastReward() const { return LastReward; }

private:
    /** Godot _on_objective: credit ObjectiveReward. */
    void HandleObjectiveCompleted(const FString& Id);

    /** Godot _on_mission: credit MissionBonus. */
    void HandleMissionCompleted();

    /** Godot _pay: add to the owned wallet + pending payout intent (no PlayerStats write). */
    void Pay(int32 Amount);

    /** The bound controller (raw delegate handles; not a UPROPERTY by design). */
    TWeakObjectPtr<UMissionController> BoundController;
    FDelegateHandle ObjectiveHandle;
    FDelegateHandle MissionHandle;

    /** Owned payout state (deferred PlayerStats coupling). */
    int32 Wallet = 0;
    int32 PendingPayout = 0;
    int32 LastReward = 0;
};
