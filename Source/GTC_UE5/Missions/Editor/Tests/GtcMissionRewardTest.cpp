// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../GtcMissionReward.h"
#include "../GtcMissionDefinition.h"

// The reward field-mapping + clamping is pure and headless; the live payout wiring
// (UMissionReward / wallet / progression) is exercised in PIE.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcMissionRewardPlanTest,
    "GTC.Missions.Editor.Reward.Plan",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcMissionRewardPlanTest::RunTest(const FString& Parameters)
{
    // Authored values map straight through.
    {
        FGtcRewardDefinition R;
        R.Money = 5000;
        R.ObjectiveReward = 300;
        R.MissionBonus = 2500;
        R.Xp = 120;
        R.Respect = 15;
        const FGtcRewardPlan P = GtcMissionReward::Plan(R);
        TestEqual(TEXT("headline"), P.HeadlineMoney, 5000);
        TestEqual(TEXT("objective reward"), P.ObjectiveReward, 300);
        TestEqual(TEXT("mission bonus"), P.MissionBonus, 2500);
        TestEqual(TEXT("xp"), P.MissionXp, 120);
        TestEqual(TEXT("respect"), P.Respect, 15);
    }

    // Negative amounts clamp to 0 (they are silent no-ops downstream).
    {
        FGtcRewardDefinition R;
        R.Money = -100;
        R.ObjectiveReward = -1;
        R.MissionBonus = -50;
        R.Xp = -5;
        R.Respect = -3;
        const FGtcRewardPlan P = GtcMissionReward::Plan(R);
        TestEqual(TEXT("headline clamped"), P.HeadlineMoney, 0);
        TestEqual(TEXT("objective clamped"), P.ObjectiveReward, 0);
        TestEqual(TEXT("bonus clamped"), P.MissionBonus, 0);
        TestEqual(TEXT("xp clamped"), P.MissionXp, 0);
        TestEqual(TEXT("respect clamped"), P.Respect, 0);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
