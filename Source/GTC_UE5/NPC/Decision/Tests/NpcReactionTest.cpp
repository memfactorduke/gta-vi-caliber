// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcReaction.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Parity tests for FNpcReaction, mapped 1:1 from the the reference oracle
 * game/tests/unit/test_npc_reaction.gd. Verb strings and panic booleans assert
 * exactly; threat scores use the oracle's threshold comparisons.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcReactionTest,
    "GTC.NPC.Decision.NpcReaction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcReactionTest::RunTest(const FString& Parameters)
{
    // test_distant_player_is_ignored
    TestEqual(TEXT("distant player ignored"), FNpcReaction::Decide(50.0, 1.0, 0.0, 1.0), FString(TEXT("ignore")));

    // test_armed_sprinter_reads_as_high_threat
    TestTrue(TEXT("armed sprinter high threat"), FNpcReaction::ThreatFrom(10.0, true) >= 0.9);

    // test_unarmed_stroller_reads_as_low_threat
    TestTrue(TEXT("unarmed stroller low threat"), FNpcReaction::ThreatFrom(1.0, false) < 0.2);

    // test_scared_npc_flees_high_threat
    TestEqual(TEXT("scared npc flees"), FNpcReaction::Decide(8.0, 0.9, 0.1, 0.0), FString(TEXT("flee")));

    // test_brave_npc_holds_ground
    TestEqual(TEXT("brave npc holds ground"), FNpcReaction::Decide(8.0, 0.6, 0.9, 0.1), FString(TEXT("ignore")));

    // test_curious_npc_gawks_at_harmless_player
    TestEqual(TEXT("curious npc gawks"), FNpcReaction::Decide(10.0, 0.0, 0.5, 0.9), FString(TEXT("gawk")));

    // test_personal_space_triggers_flee_even_unarmed
    TestEqual(TEXT("personal space triggers flee"), FNpcReaction::Decide(2.0, 0.2, 0.0, 0.0), FString(TEXT("flee")));

    // test_threat_extends_flee_range
    {
        const FString Calm = FNpcReaction::Decide(10.0, 0.1, 0.0, 0.0);
        const FString Armed = FNpcReaction::Decide(10.0, 1.0, 0.0, 0.0);
        TestNotEqual(TEXT("calm is not flee"), Calm, FString(TEXT("flee")));
        TestEqual(TEXT("armed reaches that far"), Armed, FString(TEXT("flee")));
    }

    // test_panic_spreads_to_close_timid_neighbours
    TestTrue(TEXT("panic spreads to close timid"), FNpcReaction::CatchesPanic(4.0, 0.3));

    // test_panic_does_not_reach_far_neighbours
    TestFalse(TEXT("panic does not reach far"), FNpcReaction::CatchesPanic(20.0, 0.3));

    // test_brave_neighbours_resist_panic
    TestFalse(TEXT("brave neighbours resist panic"), FNpcReaction::CatchesPanic(2.0, 0.95));

    return true;
}

#endif // WITH_AUTOMATION_TESTS
