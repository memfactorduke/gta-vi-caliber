// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../TrafficModel.h"
#include "../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Parity tests for FTrafficModel (IDM car-following), mapped 1:1 from the C++
 * oracle engine/tests/test_worldcore.cpp (the worldcore_traffic asserts).
 * Prefix GTC.Worldcore.TrafficModel.
 *
 * PURE-CORE boundary: pure longitudinal acceleration math only; Mass/actor
 * integration is the deferred Wave-3 adapter and is NOT tested here.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTrafficModelCarFollowingTest,
    "GTC.Worldcore.TrafficModel.CarFollowing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficModelCarFollowingTest::RunTest(const FString& Parameters)
{
    // test_idm_accelerates_on_clear_road: slow car, leader far ahead and fast.
    TestTrue(TEXT("accelerates on clear road"),
        FTrafficModel::CarFollowingAccel(5.0, 1000.0, 30.0, 30.0, 1.5, 2.0, 2.0, 1.5) > 0.0);

    // test_idm_cruises_at_desired_speed: at desired speed on a clear road, ~0.
    TestTrue(TEXT("cruises at desired speed"),
        FMath::Abs(FTrafficModel::CarFollowingAccel(30.0, 1000.0, 30.0, 30.0, 1.5, 2.0, 2.0, 1.5)) < 0.05);

    // test_idm_brakes_for_close_slow_leader: fast car, small gap to a stopped leader.
    TestTrue(TEXT("brakes for close slow leader"),
        FTrafficModel::CarFollowingAccel(20.0, 5.0, 0.0, 30.0, 1.5, 2.0, 2.0, 1.5) < 0.0);

    // test_idm_brakes_hard_on_overlap: zero gap (touching the leader).
    TestTrue(TEXT("brakes hard on overlap"),
        FTrafficModel::CarFollowingAccel(20.0, 0.0, 0.0, 30.0, 1.5, 2.0, 2.0, 1.5) < 0.0);

    // test_idm_negative_param_stays_finite: negative brake param must not sqrt(neg)->NaN.
    TestTrue(TEXT("negative param stays finite"),
        FMath::IsFinite(FTrafficModel::CarFollowingAccel(20.0, 50.0, 0.0, 30.0, -1.5, 2.0, 2.0, 1.5)));

    return true;
}

#endif // WITH_AUTOMATION_TESTS
