// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "../Core/MissionChain.h"
#include "MissionObjectiveDriver.h"
#include "MissionController.h"
#include "MissionCampaign.generated.h"

/**
 * Turns the lone opening objective into a CAMPAIGN: drives a sequence of missions
 * through one MissionController, advancing to the next when one is passed and
 * replaying the same one when it fails (timeout / death). Every mission carries its
 * own objective list — each objective declares an id, HUD text, world position, and
 * a kind ("reach" or "hold") — so missions play at distinct locations with distinct
 * beats instead of re-theming fixed trigger zones.
 *
 * Godot parity: game/scripts/missions/mission_campaign.gd (class MissionCampaign,
 * Node; self-wires via group "campaign" + the group-"mission" controller). This is
 * the Wave 2 UE 5.7 port: a UGameInstanceSubsystem above one UMissionController. NO
 * Godot parity oracle — its tests are BEHAVIOR (ownership / lifecycle: sequencing a
 * controller through pass/fail, loading per-mission defs into controller + driver),
 * NOT parity.
 *
 * OWNERSHIP MODEL (option-1 own-state):
 *   The campaign OWNS its sequencing via the merged, parity-tested MissionChain
 *   (#include of Missions/Core — NOT re-ported) and OWNS a sibling
 *   FMissionObjectiveDriver it loads per-mission defs into. The five-mission opening
 *   arc (the Godot _missions() table, verbatim positions/kinds/radii/durations) is
 *   owned authored data here.
 *
 * DEFERRED to Wave 3 (engine wiring, not modelled here):
 *   - group "campaign"/"mission" self-resolution and the first-tick _process kick
 *     are scene wiring; here BindController() takes the live controller explicitly.
 *   - the retry-only-when-alive gate (_retry_pending vs _player_dead over group
 *     "player_health") is preserved headless: AdvanceRetryIfPending(bPlayerDead).
 *   - the driver's _physics_process player-pos pump is its own Wave 3 wiring
 *     (see FMissionObjectiveDriver); the campaign only loads its defs.
 */
UCLASS()
class GTC_UE5_API UMissionCampaign : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** One objective row in a campaign mission (authored). */
    struct FCampaignObjective
    {
        FString Id;
        FString Text;
        FVector Pos = FVector::ZeroVector;
        FString Kind = TEXT("reach");
        double Radius = 6.0;
        double Duration = 3.0;
    };

    /** One campaign mission (authored): title, optional time limit, ordered objectives. */
    struct FCampaignMission
    {
        FString Id;
        FString Title;
        double TimeLimit = 0.0;
        TArray<FCampaignObjective> Objectives;
    };

    // USubsystem lifecycle.
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * Start the campaign on a live controller (Godot's first-tick resolve + kick).
     * Builds the chain from the opening arc, subscribes to the controller's
     * pass/fail delegates, and loads the first mission. The controller is PASSED IN
     * (Wave 3 resolves it from group "mission").
     */
    void BindController(UMissionController* Controller);

    /** Drop the controller subscription (campaign teardown). */
    void UnbindController();

    /**
     * If a failed mission is pending retry and the player is back on their feet,
     * reload the current mission (Godot's _process retry gate). bPlayerDead is
     * PASSED IN. Returns true if a retry was applied this call.
     */
    bool AdvanceRetryIfPending(bool bPlayerDead);

    bool IsCampaignComplete() const { return Chain.IsCampaignComplete(); }

    /** Missions passed so far (Godot missions_done()). */
    int32 MissionsDone() const { return Chain.Completed(); }

    /** Total campaign missions (Godot mission_total()). */
    int32 MissionTotal() const { return Chain.Count(); }

    /** The owned objective driver the campaign loads per-mission defs into. */
    FMissionObjectiveDriver& GetDriver() { return Driver; }
    const FMissionObjectiveDriver& GetDriver() const { return Driver; }

    /** True once a retry is queued (owned flag, for tests/HUD). */
    bool IsRetryPending() const { return bRetryPending; }

    /** The five-mission opening arc (Godot _missions() table). Static authored data. */
    static TArray<FCampaignMission> OpeningArc();

private:
    /** Load the active mission's defs into the controller + driver (Godot _load_current). */
    void LoadCurrent();

    /** Godot _on_mission_passed: advance the chain, load the next (unless complete). */
    void HandleMissionPassed();

    /** Godot _on_mission_failed: queue a retry of the same mission. */
    void HandleMissionFailed();

    /** Owned sequencing (merged MissionChain) + sibling driver. */
    MissionChain Chain;
    FMissionObjectiveDriver Driver;

    TWeakObjectPtr<UMissionController> BoundController;
    FDelegateHandle PassedHandle;
    FDelegateHandle FailedHandle;

    bool bRetryPending = false;

    /** Compact authored-objective builders (Godot _reach / _hold). */
    static FCampaignObjective MakeReach(const FString& Id, const FString& Text, const FVector& Pos, double Radius = 6.0);
    static FCampaignObjective MakeHold(const FString& Id, const FString& Text, const FVector& Pos, double Duration, double Radius = 8.0);
};
