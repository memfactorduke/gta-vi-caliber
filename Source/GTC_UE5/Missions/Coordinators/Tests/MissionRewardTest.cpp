// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../MissionReward.h"
#include "../MissionController.h"
#include "Engine/GameInstance.h"

// BEHAVIOR tests (Wave 2 rule) — UMissionReward has NO Godot parity oracle; these
// assert ownership / lifecycle / wiring: it subscribes to a controller's objective
// and mission delegates and credits rewards. DEFERRED-OWNERSHIP: the payout lands
// on the subsystem's OWNED state (wallet snapshot + drainable pending-payout
// intent) — NO PlayerStats type is included or written. At Wave 3 the pending
// payout drains into the canonical UPlayerStatsComponent money (docs/W3_WIRING_NOTES.md,
// f0c0c36 — Stats is the canonical money owner).

namespace MissionRewardTestNS
{
    struct FRewardRig
    {
        UGameInstance* GameInstance = nullptr;
        UMissionReward* Reward = nullptr;
        UMissionController* Controller = nullptr;
    };

    FRewardRig MakeRewardRig()
    {
        FRewardRig Rig;
        Rig.GameInstance = NewObject<UGameInstance>(GetTransientPackage());
        Rig.Reward = NewObject<UMissionReward>(Rig.GameInstance);
        Rig.Controller = NewObject<UMissionController>(Rig.GameInstance);
        return Rig;
    }
} // namespace MissionRewardTestNS
using namespace MissionRewardTestNS;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionRewardDefaultsTest,
    "GTC.Missions.Reward.DefaultsMatchGodotExports",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionRewardDefaultsTest::RunTest(const FString& Parameters)
{
    FRewardRig Rig = MakeRewardRig();
    TestEqual(TEXT("objective_reward == 250"), Rig.Reward->ObjectiveReward, 250);
    TestEqual(TEXT("mission_bonus == 1000"), Rig.Reward->MissionBonus, 1000);
    TestEqual(TEXT("pending payout starts 0"), Rig.Reward->GetPendingPayout(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionRewardPaysObjectiveAndBonusTest,
    "GTC.Missions.Reward.CreditsObjectiveRewardsAndMissionBonus",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionRewardPaysObjectiveAndBonusTest::RunTest(const FString& Parameters)
{
    FRewardRig Rig = MakeRewardRig();
    Rig.Reward->SetWallet(0);
    Rig.Reward->BindController(Rig.Controller);

    Rig.Controller->ObjectiveDefs = { { TEXT("a"), TEXT("A") }, { TEXT("b"), TEXT("B") } };
    Rig.Controller->Begin();

    Rig.Controller->Complete(TEXT("a")); // +250
    TestEqual(TEXT("wallet 250 after one objective"), Rig.Reward->GetWallet(), 250);
    TestEqual(TEXT("last reward == 250"), Rig.Reward->GetLastReward(), 250);

    Rig.Controller->Complete(TEXT("b")); // +250 objective +1000 mission bonus
    TestEqual(TEXT("wallet 1500 after final objective + bonus"), Rig.Reward->GetWallet(), 1500);
    TestEqual(TEXT("last reward == 1000 (the bonus)"), Rig.Reward->GetLastReward(), 1000);
    TestEqual(TEXT("pending payout accumulates all credits"), Rig.Reward->GetPendingPayout(), 1500);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionRewardPendingPayoutDrainsTest,
    "GTC.Missions.Reward.PendingPayoutDrainsOnce",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionRewardPendingPayoutDrainsTest::RunTest(const FString& Parameters)
{
    FRewardRig Rig = MakeRewardRig();
    Rig.Reward->BindController(Rig.Controller);
    Rig.Controller->ObjectiveDefs = { { TEXT("a"), TEXT("A") } };
    Rig.Controller->Begin();
    Rig.Controller->Complete(TEXT("a")); // +250 objective +1000 bonus = 1250

    // The Wave 3 UPlayerStatsComponent adapter drains the intent exactly once.
    TestEqual(TEXT("drain returns 1250"), Rig.Reward->ConsumePendingPayout(), 1250);
    TestEqual(TEXT("pending cleared after drain"), Rig.Reward->GetPendingPayout(), 0);
    TestEqual(TEXT("second drain returns 0"), Rig.Reward->ConsumePendingPayout(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionRewardUnbindStopsPayoutTest,
    "GTC.Missions.Reward.UnbindStopsFurtherPayout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionRewardUnbindStopsPayoutTest::RunTest(const FString& Parameters)
{
    FRewardRig Rig = MakeRewardRig();
    Rig.Reward->BindController(Rig.Controller);
    Rig.Controller->ObjectiveDefs = { { TEXT("a"), TEXT("A") }, { TEXT("b"), TEXT("B") } };
    Rig.Controller->Begin();
    Rig.Controller->Complete(TEXT("a"));
    TestEqual(TEXT("paid while bound"), Rig.Reward->GetPendingPayout(), 250);

    Rig.Reward->UnbindController();
    Rig.Controller->Complete(TEXT("b")); // objective + bonus, but reward is unbound
    TestEqual(TEXT("no payout after unbind"), Rig.Reward->GetPendingPayout(), 250);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
