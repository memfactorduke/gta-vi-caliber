// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcMind.h"
#include "../NpcNeeds.h"
#include "../NpcSchedule.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Parity tests for FNpcMind, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_npc_mind.gd. The behaviours: a content NPC follows its
 * routine; a desperate drive hijacks it; discipline raises the override bar.
 * String/enum equality asserts exactly; urgency uses the oracle's < 0.001.
 */

namespace
{
    // The oracle's _day fixture.
    TArray<FNpcScheduleBlock> MakeMindDay()
    {
        return {
            {9.0, 17.0, TEXT("work"), TEXT("office")},
            {17.0, 9.0, TEXT("sleep"), TEXT("home")},
        };
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcMindTest,
    "GTC.NPC.Decision.NpcMind",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcMindTest::RunTest(const FString& Parameters)
{
    const TArray<FNpcScheduleBlock> Day = MakeMindDay();

    // test_content_npc_follows_schedule
    {
        FNpcNeeds Needs(1.0);
        const FNpcIntent D = FNpcMind::Decide(Day, 11.0, Needs);
        TestEqual(TEXT("content activity work"), D.Activity, FString(TEXT("work")));
        TestEqual(TEXT("content reason schedule"), D.Reason, FString(TEXT("schedule")));
    }

    // test_desperate_need_overrides_schedule
    {
        FNpcNeeds Needs(1.0);
        Needs.Values[TEXT("hunger")] = 0.1; // urgency 0.9 > 0.7 threshold
        const FNpcIntent D = FNpcMind::Decide(Day, 11.0, Needs);
        TestEqual(TEXT("desperate activity eat"), D.Activity, FString(TEXT("eat")));
        TestEqual(TEXT("desperate place diner"), D.Place, FString(TEXT("diner")));
        TestEqual(TEXT("desperate reason need:hunger"), D.Reason, FString(TEXT("need:hunger")));
    }

    // test_mild_need_does_not_override
    {
        FNpcNeeds Needs(1.0);
        Needs.Values[TEXT("fun")] = 0.5; // urgency 0.5 < 0.7
        const FNpcIntent D = FNpcMind::Decide(Day, 11.0, Needs);
        TestEqual(TEXT("mild need keeps work"), D.Activity, FString(TEXT("work")));
    }

    // test_discipline_raises_override_bar
    {
        FNpcNeeds Needs(1.0);
        Needs.Values[TEXT("social")] = 0.25; // urgency 0.75
        // Flake (discipline -1): threshold 0.5 -> overrides.
        const FNpcIntent Flake = FNpcMind::Decide(Day, 11.0, Needs, -1.0);
        // Workaholic (discipline +1): threshold 0.9 -> resists, keeps working.
        const FNpcIntent Workaholic = FNpcMind::Decide(Day, 11.0, Needs, 1.0);
        TestEqual(TEXT("flake reason need:social"), Flake.Reason, FString(TEXT("need:social")));
        TestEqual(TEXT("workaholic keeps work"), Workaholic.Activity, FString(TEXT("work")));
    }

    // test_decision_reports_urgency
    {
        FNpcNeeds Needs(1.0);
        Needs.Values[TEXT("energy")] = 0.3;
        const FNpcIntent D = FNpcMind::Decide(Day, 11.0, Needs);
        TestTrue(TEXT("decision reports urgency"), FMath::Abs(D.Urgency - 0.7) < 0.001);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
