// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../CameraFeel.h"
#include "Math/UnrealMathUtility.h"

/**
 * Parity tests for FCameraFeel, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_camera_feel.gd. Tolerances mirror the oracle's
 * is_equal_approx epsilons: 1e-4 for most approx checks, 1e-2 for the
 * convergence loop; exact-equality (is_equal) cases assert exactly.
 *
 * Covers the 6 oracle-backed helpers: SprintBlend, FovForBlend, ExpSmoothed,
 * RecenterYaw, ApproachAngle, TurnRoll. LookOffset/LookReturn have NO Godot
 * oracle and are deliberately NOT tested here (untested for parity — Wave 3).
 */

namespace
{
    constexpr float Eps = 1e-4f;
    constexpr float ConvergeEps = 1e-2f;
}

// --- sprint_blend ---------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCameraFeelSprintBlendTest,
    "GTC.Camera.CameraFeel.SprintBlend",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCameraFeelSprintBlendTest::RunTest(const FString& Parameters)
{
    // test_blend_is_zero_at_walk_speed: is_equal(0.0)
    TestEqual(TEXT("zero at walk speed"), FCameraFeel::SprintBlend(5.0f, 5.0f, 8.5f), 0.0f);
    // test_blend_is_one_at_sprint_speed: is_equal(1.0)
    TestEqual(TEXT("one at sprint speed"), FCameraFeel::SprintBlend(8.5f, 5.0f, 8.5f), 1.0f);
    // test_blend_is_clamped_above_sprint_speed: is_equal(1.0)
    TestEqual(TEXT("clamped above"), FCameraFeel::SprintBlend(20.0f, 5.0f, 8.5f), 1.0f);
    // test_blend_is_clamped_below_walk_speed: is_equal(0.0)
    TestEqual(TEXT("clamped below"), FCameraFeel::SprintBlend(0.0f, 5.0f, 8.5f), 0.0f);
    // test_blend_is_proportional_between_speeds: is_equal_approx(0.5, 1e-4)
    TestEqual(TEXT("proportional mid"), FCameraFeel::SprintBlend(6.75f, 5.0f, 8.5f), 0.5f, Eps);
    // test_blend_handles_degenerate_speed_range: is_equal(0.0)
    TestEqual(TEXT("degenerate range"), FCameraFeel::SprintBlend(10.0f, 5.0f, 5.0f), 0.0f);

    return true;
}

// --- fov_for_blend --------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCameraFeelFovForBlendTest,
    "GTC.Camera.CameraFeel.FovForBlend",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCameraFeelFovForBlendTest::RunTest(const FString& Parameters)
{
    // test_fov_adds_full_kick_at_full_blend: is_equal_approx(84.0, 1e-4)
    TestEqual(TEXT("full kick at full blend"), FCameraFeel::FovForBlend(75.0f, 9.0f, 1.0f), 84.0f, Eps);
    // test_fov_is_base_at_zero_blend: is_equal_approx(75.0, 1e-4)
    TestEqual(TEXT("base at zero blend"), FCameraFeel::FovForBlend(75.0f, 9.0f, 0.0f), 75.0f, Eps);

    return true;
}

// --- exp_smoothed ---------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCameraFeelExpSmoothedTest,
    "GTC.Camera.CameraFeel.ExpSmoothed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCameraFeelExpSmoothedTest::RunTest(const FString& Parameters)
{
    // test_smoothing_converges_to_target: 200 steps -> is_equal_approx(84.0, 1e-2)
    float Fov = 75.0f;
    for (int32 I = 0; I < 200; ++I)
    {
        Fov = FCameraFeel::ExpSmoothed(Fov, 84.0f, 8.0f, 0.016f);
    }
    TestEqual(TEXT("converges to target"), Fov, 84.0f, ConvergeEps);

    // test_smoothing_is_frame_rate_independent: two half-steps == one full step (1e-4).
    const float OneStep = FCameraFeel::ExpSmoothed(75.0f, 84.0f, 8.0f, 0.032f);
    const float Half = FCameraFeel::ExpSmoothed(75.0f, 84.0f, 8.0f, 0.016f);
    const float TwoSteps = FCameraFeel::ExpSmoothed(Half, 84.0f, 8.0f, 0.016f);
    TestEqual(TEXT("frame-rate independent"), OneStep, TwoSteps, Eps);

    // test_smoothing_never_overshoots: result <= target.
    const float Overshoot = FCameraFeel::ExpSmoothed(75.0f, 84.0f, 100.0f, 1.0f);
    TestTrue(TEXT("never overshoots"), Overshoot <= 84.0f);

    return true;
}

// --- recenter_yaw ---------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCameraFeelRecenterYawTest,
    "GTC.Camera.CameraFeel.RecenterYaw",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCameraFeelRecenterYawTest::RunTest(const FString& Parameters)
{
    // test_recenter_yaw_forward_is_zero: forward (-Z) at yaw 0 -> is_equal_approx(0.0, 1e-4)
    TestEqual(TEXT("forward is zero"), FCameraFeel::RecenterYaw(0.0f, -1.0f), 0.0f, Eps);

    // test_recenter_yaw_matches_motion_convention: reproduce the direction convention
    // inline (build forward (0,0,-1), rotate by yaw about up, assert ~= (1,0,0)) instead
    // of importing PlayerMotion.
    {
        const float Yaw = FCameraFeel::RecenterYaw(1.0f, 0.0f);
        // PlayerMotion convention: forward input (0,-1) maps to local forward (0,0,-1)
        // in a RH Y-up frame, rotated about the up (+Y) axis by Yaw. For a RH rotation
        // about +Y of vector (x, y, z):
        //   x' =  x*cos + z*sin
        //   z' = -x*sin + z*cos
        // Forward = (0, 0, -1):
        const float C = FMath::Cos(Yaw);
        const float S = FMath::Sin(Yaw);
        const FVector Dir(0.0f * C + (-1.0f) * S, 0.0f, -(0.0f * S) + (-1.0f) * C);
        // FVector components are doubles in UE5 — compare against doubles to avoid
        // an ambiguous float/double TestEqual overload.
        TestEqual(TEXT("dir x"), Dir.X, 1.0, static_cast<double>(Eps));
        TestEqual(TEXT("dir y"), Dir.Y, 0.0, static_cast<double>(Eps));
        TestEqual(TEXT("dir z"), Dir.Z, 0.0, static_cast<double>(Eps));
    }

    // test_recenter_yaw_zero_velocity_safe: is_equal(0.0)
    TestEqual(TEXT("zero velocity safe"), FCameraFeel::RecenterYaw(0.0f, 0.0f), 0.0f);

    return true;
}

// --- approach_angle -------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCameraFeelApproachAngleTest,
    "GTC.Camera.CameraFeel.ApproachAngle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCameraFeelApproachAngleTest::RunTest(const FString& Parameters)
{
    // test_approach_angle_steps_toward: is_equal_approx(0.25, 1e-4)
    TestEqual(TEXT("steps toward"), FCameraFeel::ApproachAngle(0.0f, 1.0f, 0.25f), 0.25f, Eps);
    // test_approach_angle_clamps_to_target: is_equal_approx(0.1, 1e-4)
    TestEqual(TEXT("clamps to target"), FCameraFeel::ApproachAngle(0.0f, 0.1f, 0.25f), 0.1f, Eps);
    // test_approach_angle_takes_short_arc_over_wrap: from 3.0 toward -3.0, short way
    // crosses the +/-PI wrap (increasing) -> result > 3.0.
    TestTrue(TEXT("short arc over wrap"), FCameraFeel::ApproachAngle(3.0f, -3.0f, 0.1f) > 3.0f);

    return true;
}

// --- turn_roll ------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCameraFeelTurnRollTest,
    "GTC.Camera.CameraFeel.TurnRoll",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCameraFeelTurnRollTest::RunTest(const FString& Parameters)
{
    // test_turn_roll_is_zero_at_zero_speed: is_equal_approx(0.0, 1e-4)
    TestEqual(TEXT("zero at zero speed"), FCameraFeel::TurnRoll(2.0f, 0.0f, 0.05f, 0.08f), 0.0f, Eps);
    // test_turn_roll_banks_opposite_to_yaw: is_equal_approx(-0.05, 1e-4)
    TestEqual(TEXT("banks opposite to yaw"), FCameraFeel::TurnRoll(1.0f, 1.0f, 0.05f, 0.08f), -0.05f, Eps);
    // test_turn_roll_is_capped: is_equal_approx(-0.08, 1e-4)
    TestEqual(TEXT("capped"), FCameraFeel::TurnRoll(100.0f, 1.0f, 0.05f, 0.08f), -0.08f, Eps);
    // test_turn_roll_scales_with_blend: is_equal_approx(-0.025, 1e-4)
    TestEqual(TEXT("scales with blend"), FCameraFeel::TurnRoll(1.0f, 0.5f, 0.05f, 0.08f), -0.025f, Eps);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
