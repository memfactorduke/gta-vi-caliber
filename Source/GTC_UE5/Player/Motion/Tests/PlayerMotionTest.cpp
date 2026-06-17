// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../PlayerMotion.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Parity tests for FPlayerMotion, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_player_motion.gd. Tolerances mirror the oracle's
 * is_equal_approx epsilon (1e-4); is_equal cases assert exactly.
 *
 * FVector / FVector2D components are doubles in UE5, so all numeric comparisons
 * are against double literals (avoids the ambiguous float/double TestEqual
 * overload).
 */

using GtcTest::Eps;

namespace
{
    void TestVectorEqual(FAutomationTestBase& T, const TCHAR* What, const FVector& A, const FVector& B, double Tol)
    {
        T.TestEqual(FString::Printf(TEXT("%s.x"), What), A.X, B.X, Tol);
        T.TestEqual(FString::Printf(TEXT("%s.y"), What), A.Y, B.Y, Tol);
        T.TestEqual(FString::Printf(TEXT("%s.z"), What), A.Z, B.Z, Tol);
    }
}

// --- direction_from_input -------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerMotionDirectionTest,
    "GTC.Player.Motion.Direction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerMotionDirectionTest::RunTest(const FString& Parameters)
{
    // test_zero_input_gives_zero_direction: is_equal(Vector3.ZERO)
    TestVectorEqual(*this, TEXT("zero input zero dir"),
        FPlayerMotion::DirectionFromInput(FVector2D::ZeroVector, 0.0), FVector::ZeroVector, 0.0);

    // test_forward_input_points_minus_z_at_zero_yaw: is_equal_approx((0,0,-1), 1e-4)
    TestVectorEqual(*this, TEXT("forward minus z"),
        FPlayerMotion::DirectionFromInput(FVector2D(0.0, -1.0), 0.0), FVector(0.0, 0.0, -1.0), Eps);

    // test_direction_is_normalized_for_diagonal_input: length ~= 1.0 (1e-4)
    {
        const FVector Dir = FPlayerMotion::DirectionFromInput(FVector2D(1.0, -1.0), 0.0);
        TestEqual(TEXT("diagonal normalized"), Dir.Size(), 1.0, Eps);
    }

    // test_yaw_rotates_direction: forward + 90deg yaw -> (-1,0,0) (1e-4)
    TestVectorEqual(*this, TEXT("yaw rotates"),
        FPlayerMotion::DirectionFromInput(FVector2D(0.0, -1.0), PI / 2.0), FVector(-1.0, 0.0, 0.0), Eps);

    return true;
}

// --- horizontal_velocity --------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerMotionHorizontalVelocityTest,
    "GTC.Player.Motion.HorizontalVelocity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerMotionHorizontalVelocityTest::RunTest(const FString& Parameters)
{
    // test_horizontal_velocity_scales_by_speed: is_equal_approx((0,0,-5), 1e-4)
    TestVectorEqual(*this, TEXT("scales by speed"),
        FPlayerMotion::HorizontalVelocity(FVector(0.0, 0.0, -1.0), 5.0), FVector(0.0, 0.0, -5.0), Eps);

    return true;
}

// --- accelerated ----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerMotionAcceleratedTest,
    "GTC.Player.Motion.Accelerated",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerMotionAcceleratedTest::RunTest(const FString& Parameters)
{
    // test_accelerated_preserves_vertical_velocity
    {
        const FVector Current(0.0, -9.8, 0.0);
        const FVector Next = FPlayerMotion::Accelerated(Current, FVector(5.0, 0.0, 0.0), 30.0, 0.016);
        TestEqual(TEXT("preserves y"), Next.Y, -9.8, Eps);
        TestTrue(TEXT("x increases"), Next.X > 0.0);
    }

    // test_accelerated_reaches_target_eventually: 100 steps -> target (1e-4)
    {
        FVector Current = FVector::ZeroVector;
        const FVector Target(5.0, 0.0, 0.0);
        for (int32 I = 0; I < 100; ++I)
        {
            Current = FPlayerMotion::Accelerated(Current, Target, 30.0, 0.016);
        }
        TestVectorEqual(*this, TEXT("reaches target"), Current, Target, Eps);
    }

    return true;
}

// --- acceleration_rate ----------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerMotionAccelerationRateTest,
    "GTC.Player.Motion.AccelerationRate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerMotionAccelerationRateTest::RunTest(const FString& Parameters)
{
    // test_acceleration_rate_brakes_with_decel_when_no_input: is_equal(45.0)
    TestEqual(TEXT("brakes with decel"), FPlayerMotion::AccelerationRate(false, true, 30.0, 45.0, 0.35), 45.0);
    // test_acceleration_rate_uses_accel_with_input: is_equal(30.0)
    TestEqual(TEXT("accel with input"), FPlayerMotion::AccelerationRate(true, true, 30.0, 45.0, 0.35), 30.0);
    // test_acceleration_rate_is_scaled_in_air: is_equal_approx(10.5, 1e-4)
    TestEqual(TEXT("scaled in air"), FPlayerMotion::AccelerationRate(true, false, 30.0, 45.0, 0.35), 10.5, Eps);

    return true;
}

// --- should_jump ----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerMotionShouldJumpTest,
    "GTC.Player.Motion.ShouldJump",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerMotionShouldJumpTest::RunTest(const FString& Parameters)
{
    // test_jump_fires_on_floor_with_fresh_press
    TestTrue(TEXT("fires on floor fresh"), FPlayerMotion::ShouldJump(0.0, 0.12, 0.0, 0.12, false));
    // test_jump_fires_within_coyote_window
    TestTrue(TEXT("fires within coyote"), FPlayerMotion::ShouldJump(0.1, 0.12, 0.0, 0.12, false));
    // test_jump_rejected_after_coyote_expires
    TestFalse(TEXT("rejected after coyote"), FPlayerMotion::ShouldJump(0.2, 0.12, 0.0, 0.12, false));
    // test_buffered_press_fires_on_landing
    TestTrue(TEXT("buffered fires on landing"), FPlayerMotion::ShouldJump(0.0, 0.12, 0.08, 0.12, false));
    // test_stale_press_does_not_fire
    TestFalse(TEXT("stale press no fire"), FPlayerMotion::ShouldJump(0.0, 0.12, 0.5, 0.12, false));
    // test_jump_rejected_when_already_spent
    TestFalse(TEXT("rejected when spent"), FPlayerMotion::ShouldJump(0.0, 0.12, 0.0, 0.12, true));

    return true;
}

// --- climb_velocity -------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerMotionClimbTest,
    "GTC.Player.Motion.Climb",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerMotionClimbTest::RunTest(const FString& Parameters)
{
    // test_climb_forward_input_goes_up: is_equal_approx((0,3,0), 1e-4)
    TestVectorEqual(*this, TEXT("forward goes up"),
        FPlayerMotion::ClimbVelocity(FVector2D(0.0, -1.0), FVector::ZeroVector, 3.0), FVector(0.0, 3.0, 0.0), Eps);
    // test_climb_back_input_goes_down: is_equal_approx((0,-3,0), 1e-4)
    TestVectorEqual(*this, TEXT("back goes down"),
        FPlayerMotion::ClimbVelocity(FVector2D(0.0, 1.0), FVector::ZeroVector, 3.0), FVector(0.0, -3.0, 0.0), Eps);
    // test_climb_without_vertical_input_hangs: is_equal(Vector3.ZERO)
    TestVectorEqual(*this, TEXT("no vertical hangs"),
        FPlayerMotion::ClimbVelocity(FVector2D::ZeroVector, FVector::ZeroVector, 3.0), FVector::ZeroVector, 0.0);
    // test_climb_keeps_half_speed_lateral_steering: is_equal_approx((1.5,0,0), 1e-4)
    TestVectorEqual(*this, TEXT("half speed lateral"),
        FPlayerMotion::ClimbVelocity(FVector2D::ZeroVector, FVector(1.0, 0.0, 0.0), 3.0), FVector(1.5, 0.0, 0.0), Eps);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
