// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../PursuitMemory.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Parity tests for FPursuitMemory, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_pursuit_memory.gd. Tolerance mirrors the oracle's
 * is_equal_approx (Eps = 1e-4); enum/bool/vector equality asserts exactly.
 *
 * Prefix GTC.AI.Pursuit.PursuitMemory. PLAYER/LAST mirror the oracle constants.
 *
 * PURE-CORE boundary: AI Perception / LOS sensing and Behavior Tree wiring are
 * the deferred Wave-3 adapter — the model takes plain args (seen flag, timers,
 * RNG samples). NOT tested.
 */

namespace
{
    const FVector PursuitMemoryPlayer(20, 0, 5);
    const FVector PursuitMemoryLast(8, 0, -3);
}

// --- target -----------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPursuitMemoryTargetTest,
    "GTC.AI.Pursuit.PursuitMemory.Target",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPursuitMemoryTargetTest::RunTest(const FString& Parameters)
{
    // test_target_is_live_when_seen
    TestTrue(TEXT("seen -> live"), FPursuitMemory::Target(
        true, PursuitMemoryPlayer, PursuitMemoryLast) == PursuitMemoryPlayer);
    // test_target_is_last_known_when_blind
    TestTrue(TEXT("blind -> last known"), FPursuitMemory::Target(
        false, PursuitMemoryPlayer, PursuitMemoryLast) == PursuitMemoryLast);
    return true;
}

// --- should_give_up ---------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPursuitMemoryGiveUpTest,
    "GTC.AI.Pursuit.PursuitMemory.GiveUp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPursuitMemoryGiveUpTest::RunTest(const FString& Parameters)
{
    // test_give_up_after_timeout
    TestTrue(TEXT("give up at timeout"), FPursuitMemory::ShouldGiveUp(8.0, 8.0));
    TestTrue(TEXT("give up past timeout"), FPursuitMemory::ShouldGiveUp(9.5, 8.0));
    // test_no_give_up_before_timeout
    TestFalse(TEXT("no give up before"), FPursuitMemory::ShouldGiveUp(3.0, 8.0));
    // test_zero_give_up_time_is_relentless
    TestFalse(TEXT("relentless"), FPursuitMemory::ShouldGiveUp(999.0, 0.0));
    return true;
}

// --- state ------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPursuitMemoryStateTest,
    "GTC.AI.Pursuit.PursuitMemory.State",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPursuitMemoryStateTest::RunTest(const FString& Parameters)
{
    // test_state_pursue_when_seen
    TestTrue(TEXT("seen -> pursue"), FPursuitMemory::State(true, 99.0, true, 8.0)
        == EPursuitMemoryState::Pursue);
    // test_state_lost_when_timed_out
    TestTrue(TEXT("timed out -> lost"), FPursuitMemory::State(false, 8.0, true, 8.0)
        == EPursuitMemoryState::Lost);
    // test_state_search_when_reached_last_known
    TestTrue(TEXT("reached -> search"), FPursuitMemory::State(false, 2.0, true, 8.0)
        == EPursuitMemoryState::Search);
    // test_state_pursue_while_enroute_to_last_known
    TestTrue(TEXT("enroute -> pursue"), FPursuitMemory::State(false, 2.0, false, 8.0)
        == EPursuitMemoryState::Pursue);
    return true;
}

// --- search_point -----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPursuitMemorySearchPointTest,
    "GTC.AI.Pursuit.PursuitMemory.SearchPoint",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPursuitMemorySearchPointTest::RunTest(const FString& Parameters)
{
    // test_search_point_within_radius
    for (int32 i = 0; i < 6; ++i)
    {
        for (int32 j = 0; j < 6; ++j)
        {
            const FVector P = FPursuitMemory::SearchPoint(
                PursuitMemoryLast, double(i) / 5.0, double(j) / 5.0);
            const FVector2D Planar(P.X - PursuitMemoryLast.X, P.Z - PursuitMemoryLast.Z);
            TestTrue(TEXT("within radius"),
                Planar.Size() <= FPursuitMemory::SearchRadius + 0.001);
        }
    }
    // test_search_point_keeps_height
    {
        const FVector P = FPursuitMemory::SearchPoint(FVector(4, 7, 2), 0.5, 0.5);
        TestEqual(TEXT("keeps height"), P.Y, 7.0, Eps);
    }
    // test_search_point_custom_radius
    {
        const FVector P = FPursuitMemory::SearchPoint(PursuitMemoryLast, 1.0, 0.0, 12.0);
        const FVector2D Planar(P.X - PursuitMemoryLast.X, P.Z - PursuitMemoryLast.Z);
        TestEqual(TEXT("custom radius rim"), Planar.Size(), 12.0, Eps);
    }
    return true;
}

#endif // WITH_AUTOMATION_TESTS
