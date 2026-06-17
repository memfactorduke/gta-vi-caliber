// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../Locomotion.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Parity tests for FLocomotion, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_locomotion.gd. Tolerances mirror the oracle's
 * is_equal_approx epsilon (the shared GtcTest::Eps, 1e-4 double); enum/state and
 * exact comparisons assert exactly. Each test below names the oracle method it
 * reproduces.
 */

using GtcTest::Eps;

namespace
{
    // The remaining constants keep a unique prefix so they survive unity-build
    // concatenation alongside sibling test files (the shared header handles Eps).
    constexpr double LocoTau = UE_DOUBLE_TWO_PI;
    constexpr double LocoPi = UE_DOUBLE_PI;

    constexpr double LocoWalk = 5.0;
    constexpr double LocoRun = 8.5;

    int LocoSign(double V)
    {
        return V > 0.0 ? 1 : (V < 0.0 ? -1 : 0);
    }
}

// --- state_for ------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLocomotionStateForTest,
    "GTC.Player.Locomotion.StateFor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLocomotionStateForTest::RunTest(const FString& Parameters)
{
    // test_idle_when_barely_moving
    TestTrue(TEXT("idle barely moving"),
        FLocomotion::StateFor(0.05, true, 0.0, false, LocoWalk, LocoRun) == FLocomotion::EState::Idle);
    // test_walk_state_at_walk_speed
    TestTrue(TEXT("walk at walk speed"),
        FLocomotion::StateFor(LocoWalk, true, 0.0, false, LocoWalk, LocoRun) == FLocomotion::EState::Walk);
    // test_run_state_above_gate
    TestTrue(TEXT("run above gate"),
        FLocomotion::StateFor(LocoRun, true, 0.0, false, LocoWalk, LocoRun) == FLocomotion::EState::Run);
    // test_walk_holds_just_over_walk_speed
    TestTrue(TEXT("walk holds just over walk"),
        FLocomotion::StateFor(LocoWalk + 0.5, true, 0.0, false, LocoWalk, LocoRun) == FLocomotion::EState::Walk);
    // test_jump_when_rising_airborne
    TestTrue(TEXT("jump rising airborne"),
        FLocomotion::StateFor(3.0, false, 2.0, false, LocoWalk, LocoRun) == FLocomotion::EState::Jump);
    // test_fall_when_descending_airborne
    TestTrue(TEXT("fall descending airborne"),
        FLocomotion::StateFor(3.0, false, -2.0, false, LocoWalk, LocoRun) == FLocomotion::EState::Fall);
    // test_climb_overrides_everything
    TestTrue(TEXT("climb overrides"),
        FLocomotion::StateFor(0.0, false, -5.0, true, LocoWalk, LocoRun) == FLocomotion::EState::Climb);

    return true;
}

// --- move_blend -----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLocomotionMoveBlendTest,
    "GTC.Player.Locomotion.MoveBlend",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLocomotionMoveBlendTest::RunTest(const FString& Parameters)
{
    // test_move_blend_zero_when_still
    TestEqual(TEXT("zero when still"), FLocomotion::MoveBlend(0.0, LocoWalk, LocoRun), 0.0, Eps);
    // test_move_blend_half_at_walk_speed
    TestEqual(TEXT("half at walk speed"), FLocomotion::MoveBlend(LocoWalk, LocoWalk, LocoRun), 0.5, Eps);
    // test_move_blend_one_at_run_speed
    TestEqual(TEXT("one at run speed"), FLocomotion::MoveBlend(LocoRun, LocoWalk, LocoRun), 1.0, Eps);
    // test_move_blend_clamps_above_run
    TestEqual(TEXT("clamps above run"), FLocomotion::MoveBlend(LocoRun * 2.0, LocoWalk, LocoRun), 1.0, Eps);

    // test_move_blend_is_monotonic
    double Prev = -1.0;
    for (int32 I = 0; I < 20; ++I)
    {
        const double Speed = static_cast<double>(I) * 0.6;
        const double Blend = FLocomotion::MoveBlend(Speed, LocoWalk, LocoRun);
        TestTrue(TEXT("monotonic"), Blend >= Prev);
        Prev = Blend;
    }

    return true;
}

// --- advance_phase --------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLocomotionAdvancePhaseTest,
    "GTC.Player.Locomotion.AdvancePhase",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLocomotionAdvancePhaseTest::RunTest(const FString& Parameters)
{
    // test_phase_advances_with_speed
    TestTrue(TEXT("advances with speed"), FLocomotion::AdvancePhase(0.0, LocoWalk, 0.1) > 0.0);
    // test_phase_does_not_advance_when_stationary
    TestEqual(TEXT("no advance stationary"), FLocomotion::AdvancePhase(0.0, 0.0, 0.1), 0.0, Eps);
    // test_phase_wraps_to_tau
    const double Wrapped = FLocomotion::AdvancePhase(0.0, 1000.0, 1.0);
    TestTrue(TEXT("wraps within [0,TAU)"), Wrapped >= 0.0 && Wrapped < LocoTau);
    // test_phase_advance_scales_with_distance
    const double Slow = FLocomotion::AdvancePhase(0.0, 1.0, 0.05);
    const double Fast = FLocomotion::AdvancePhase(0.0, 2.0, 0.05);
    TestEqual(TEXT("scales with distance"), Fast, Slow * 2.0, Eps);

    return true;
}

// --- limb_swing / vertical_bob --------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLocomotionSwingBobTest,
    "GTC.Player.Locomotion.SwingBob",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLocomotionSwingBobTest::RunTest(const FString& Parameters)
{
    // test_limbs_counter_swing
    const double Left = FLocomotion::LimbSwing(0.5, 0.6);
    const double Right = FLocomotion::LimbSwing(0.5 + LocoPi, 0.6);
    TestEqual(TEXT("limbs counter-swing"), Left, -Right, Eps);
    // test_limb_swing_peaks_at_amplitude
    TestEqual(TEXT("swing peaks at amplitude"), FLocomotion::LimbSwing(LocoPi / 2.0, 0.6), 0.6, Eps);

    // test_vertical_bob_never_positive
    for (int32 I = 0; I < 16; ++I)
    {
        const double Phase = static_cast<double>(I) / 16.0 * LocoTau;
        TestTrue(TEXT("bob never positive"), FLocomotion::VerticalBob(Phase, 0.08) <= 0.0);
    }
    // test_vertical_bob_double_frequency
    const double First = FLocomotion::VerticalBob(LocoPi / 2.0, 0.08);
    const double Second = FLocomotion::VerticalBob(3.0 * LocoPi / 2.0, 0.08);
    TestEqual(TEXT("bob double frequency equal"), First, Second, Eps);
    TestTrue(TEXT("bob double frequency negative"), First < 0.0);

    return true;
}

// --- secondary body motion ------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLocomotionSecondaryMotionTest,
    "GTC.Player.Locomotion.SecondaryMotion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLocomotionSecondaryMotionTest::RunTest(const FString& Parameters)
{
    // test_secondary_body_motion_tracks_stride_side (decomposed assertion-by-assertion)
    const double LeftSway = FLocomotion::LateralSway(LocoPi / 2.0, 0.03);
    const double RightSway = FLocomotion::LateralSway(3.0 * LocoPi / 2.0, 0.03);
    const double Phase = LocoPi / 2.0;
    const double LeftYaw = FLocomotion::FootToeOut(1.0, -1.0, 0.07, 0.04);
    const double RightYaw = FLocomotion::FootToeOut(-1.0, -1.0, 0.07, 0.04);
    const double LeftBank = FLocomotion::FootBank(1.0, 0.5, 0.05);
    const double RightBank = FLocomotion::FootBank(-1.0, 0.5, 0.05);
    const double LeftTurn = FLocomotion::TurnLean(8.0, 8.0, 0.08);
    const double RightTurn = FLocomotion::TurnLean(-8.0, 8.0, 0.08);
    const double Landing = FLocomotion::LandingCompression(-9.0, 12.0, 0.09);
    const double IdleBreath = FLocomotion::IdleBreath(0.75, 0.02);
    const double IdleShift = FLocomotion::IdleWeightShift(1.4, 0.015);
    const double IdleHead = FLocomotion::IdleHeadPitch(0.75, 0.014);

    TestTrue(TEXT("left sway > 0"), LeftSway > 0.0);
    TestTrue(TEXT("right sway < 0"), RightSway < 0.0);
    TestEqual(TEXT("sway mirrors"), LeftSway, -RightSway, Eps);
    TestTrue(TEXT("pelvis roll < 0"), FLocomotion::PelvisRoll(Phase, 0.05) < 0.0);
    TestTrue(TEXT("shoulder counter-roll > 0"), FLocomotion::ShoulderCounterRoll(Phase, 0.04) > 0.0);
    TestTrue(TEXT("torso twist < 0"), FLocomotion::TorsoTwist(Phase, 0.05) < 0.0);
    TestEqual(TEXT("torso twist mirrors over PI"),
        FLocomotion::TorsoTwist(Phase, 0.05), -FLocomotion::TorsoTwist(Phase + LocoPi, 0.05), Eps);
    TestTrue(TEXT("head step pitch < 0"), FLocomotion::HeadStepPitch(Phase, 0.02) < 0.0);
    TestTrue(TEXT("head counter-roll < 0"), FLocomotion::HeadCounterRoll(Phase, 0.03) < 0.0);
    TestTrue(TEXT("left yaw > 0"), LeftYaw > 0.0);
    TestTrue(TEXT("right yaw < 0"), RightYaw < 0.0);
    TestEqual(TEXT("yaw mirrors"), LeftYaw, -RightYaw, Eps);
    TestEqual(TEXT("bank mirrors"), LeftBank, -RightBank, Eps);
    TestTrue(TEXT("left turn > 0"), LeftTurn > 0.0);
    TestEqual(TEXT("turn mirrors"), LeftTurn, -RightTurn, Eps);
    TestTrue(TEXT("landing > 0"), Landing > 0.0);
    TestEqual(TEXT("upward landing is zero"),
        FLocomotion::LandingCompression(2.0, 12.0, 0.09), 0.0, Eps);
    TestTrue(TEXT("idle breath nonzero"), !FMath::IsNearlyZero(IdleBreath));
    TestTrue(TEXT("idle shift nonzero"), !FMath::IsNearlyZero(IdleShift));
    TestTrue(TEXT("idle head opposes breath"), LocoSign(IdleHead) != LocoSign(IdleBreath));

    return true;
}

// --- lean_angle -----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLocomotionLeanAngleTest,
    "GTC.Player.Locomotion.LeanAngle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLocomotionLeanAngleTest::RunTest(const FString& Parameters)
{
    // test_lean_forward_on_acceleration
    TestTrue(TEXT("lean forward on accel"), FLocomotion::LeanAngle(30.0, 30.0, 0.25) > 0.0);
    // test_lean_back_on_braking
    TestTrue(TEXT("lean back on braking"), FLocomotion::LeanAngle(-30.0, 30.0, 0.25) < 0.0);
    // test_lean_clamps_to_max
    TestEqual(TEXT("lean clamps to max"), FLocomotion::LeanAngle(100.0, 30.0, 0.25), 0.25, Eps);
    // test_lean_zero_reference_is_safe
    TestEqual(TEXT("zero reference safe"), FLocomotion::LeanAngle(30.0, 0.0, 0.25), 0.0, Eps);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
