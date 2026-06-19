// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../SwimMotion.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Parity tests for FSwimMotion, mapped 1:1 from the the reference oracle
 * game/tests/unit/test_swim_motion.gd. The oracle uses `==` for exact cases
 * (submersion zero/one/degenerate, vertical_axis, oxygen clamps) and
 * `absf(...) < 0.0001` for approx cases — mirrored here with exact-equal and
 * Eps=1e-4 respectively. target_velocity's is_equal_approx uses Eps.
 */

using GtcTest::Eps;

// --- submersion -----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSwimMotionSubmersionTest,
    "GTC.Player.Motion.SwimSubmersion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSwimMotionSubmersionTest::RunTest(const FString& Parameters)
{
    // test_submersion_zero_above_water: == 0.0
    TestEqual(TEXT("zero above water"), FSwimMotion::Submersion(1.0, 0.0, 1.8), 0.0);
    // test_submersion_half_at_chest: absf(.. - 0.5) < 1e-4
    TestEqual(TEXT("half at chest"), FSwimMotion::Submersion(0.0, 0.9, 1.8), 0.5, Eps);
    // test_submersion_clamps_when_fully_under: == 1.0
    TestEqual(TEXT("clamps fully under"), FSwimMotion::Submersion(-5.0, 0.0, 1.8), 1.0);
    // test_submersion_safe_with_zero_height: == 0.0
    TestEqual(TEXT("safe zero height"), FSwimMotion::Submersion(0.0, 1.0, 0.0), 0.0);

    return true;
}

// --- is_swimming ----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSwimMotionIsSwimmingTest,
    "GTC.Player.Motion.IsSwimming",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSwimMotionIsSwimmingTest::RunTest(const FString& Parameters)
{
    // test_is_swimming_starts_when_chest_deep
    TestTrue(TEXT("starts chest deep"), FSwimMotion::IsSwimming(0.6, false, 0.6, 0.45));
    // test_is_swimming_not_yet_when_shallow
    TestFalse(TEXT("not yet shallow"), FSwimMotion::IsSwimming(0.5, false, 0.6, 0.45));
    // test_is_swimming_holds_through_small_dip
    TestTrue(TEXT("holds small dip"), FSwimMotion::IsSwimming(0.5, true, 0.6, 0.45));
    // test_is_swimming_leaves_at_wading_depth
    TestFalse(TEXT("leaves wading"), FSwimMotion::IsSwimming(0.4, true, 0.6, 0.45));

    return true;
}

// --- vertical_axis --------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSwimMotionVerticalAxisTest,
    "GTC.Player.Motion.VerticalAxis",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSwimMotionVerticalAxisTest::RunTest(const FString& Parameters)
{
    // test_vertical_axis_up_down_and_none: == 1.0 / -1.0 / 0.0 / 0.0
    TestEqual(TEXT("up"), FSwimMotion::VerticalAxis(true, false), 1.0);
    TestEqual(TEXT("down"), FSwimMotion::VerticalAxis(false, true), -1.0);
    TestEqual(TEXT("none"), FSwimMotion::VerticalAxis(false, false), 0.0);
    TestEqual(TEXT("both"), FSwimMotion::VerticalAxis(true, true), 0.0);

    return true;
}

// --- target_velocity ------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSwimMotionTargetVelocityTest,
    "GTC.Player.Motion.SwimTargetVelocity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSwimMotionTargetVelocityTest::RunTest(const FString& Parameters)
{
    // test_target_velocity_maps_axes: is_equal_approx((4,3,0))
    const FVector V = FSwimMotion::TargetVelocity(FVector(1.0, 0.0, 0.0), 4.0, 1.0, 3.0);
    TestEqual(TEXT("vx"), V.X, 4.0, Eps);
    TestEqual(TEXT("vy"), V.Y, 3.0, Eps);
    TestEqual(TEXT("vz"), V.Z, 0.0, Eps);

    return true;
}

// --- buoyancy -------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSwimMotionBuoyancyTest,
    "GTC.Player.Motion.Buoyancy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSwimMotionBuoyancyTest::RunTest(const FString& Parameters)
{
    // test_buoyancy_rises_when_too_deep: > 0.0
    TestTrue(TEXT("rises too deep"), FSwimMotion::Buoyancy(0.9, 0.62, 6.0, 1.5) > 0.0);
    // test_buoyancy_sinks_when_too_high: < 0.0
    TestTrue(TEXT("sinks too high"), FSwimMotion::Buoyancy(0.4, 0.62, 6.0, 1.5) < 0.0);
    // test_buoyancy_clamps_to_max_speed: == 1.5
    TestEqual(TEXT("clamps to max"), FSwimMotion::Buoyancy(1.0, 0.0, 100.0, 1.5), 1.5);

    return true;
}

// --- head_underwater ------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSwimMotionHeadUnderwaterTest,
    "GTC.Player.Motion.HeadUnderwater",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSwimMotionHeadUnderwaterTest::RunTest(const FString& Parameters)
{
    // test_head_underwater_true_past_threshold
    TestTrue(TEXT("under at 0.95"), FSwimMotion::HeadUnderwater(0.95, 0.9));
    TestFalse(TEXT("not under at 0.8"), FSwimMotion::HeadUnderwater(0.8, 0.9));

    return true;
}

// --- next_oxygen ----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSwimMotionNextOxygenTest,
    "GTC.Player.Motion.NextOxygen",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSwimMotionNextOxygenTest::RunTest(const FString& Parameters)
{
    // test_oxygen_drains_underwater: absf(.. - 0.9) < 1e-4
    TestEqual(TEXT("drains underwater"), FSwimMotion::NextOxygen(1.0, true, 10.0, 0.5, 1.0), 0.9, Eps);
    // test_oxygen_refills_at_surface: absf(.. - 1.0) < 1e-4
    TestEqual(TEXT("refills at surface"), FSwimMotion::NextOxygen(0.5, false, 10.0, 0.5, 1.0), 1.0, Eps);
    // test_oxygen_never_below_zero: == 0.0
    TestEqual(TEXT("never below zero"), FSwimMotion::NextOxygen(0.05, true, 10.0, 0.5, 1.0), 0.0);
    // test_oxygen_never_above_one: == 1.0
    TestEqual(TEXT("never above one"), FSwimMotion::NextOxygen(0.95, false, 10.0, 5.0, 1.0), 1.0);
    // test_oxygen_degenerate_breath_empties: == 0.0
    TestEqual(TEXT("degenerate breath empties"), FSwimMotion::NextOxygen(1.0, true, 0.0, 0.5, 0.016), 0.0);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
