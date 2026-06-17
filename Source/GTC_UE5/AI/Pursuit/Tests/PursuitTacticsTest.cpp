// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../PursuitTactics.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Parity tests for FPursuitTactics, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_pursuit_tactics.gd. Tolerance mirrors the oracle's
 * is_equal_approx (Eps = 1e-4) for float/vector compares; enum/bool equality
 * asserts exactly.
 *
 * Grouped by the oracle's method sections (prefix GTC.AI.Pursuit.PursuitTactics).
 *
 * PURE-CORE boundary: EQS + AI Perception + Behavior Tree wiring is the deferred
 * Wave-3 adapter — the model takes plain position/state args. NOT tested.
 */

// --- intercept_point --------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPursuitTacticsInterceptTest,
    "GTC.AI.Pursuit.PursuitTactics.Intercept",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPursuitTacticsInterceptTest::RunTest(const FString& Parameters)
{
    // test_intercept_leads_ahead_of_moving_target
    {
        const FVector Aim = FPursuitTactics::InterceptPoint(
            FVector(10, 0, 0), FVector(5, 0, 0), FVector::ZeroVector, 10.0);
        TestEqual(TEXT("lead x"), Aim.X, 20.0, Eps);
        TestEqual(TEXT("lead z"), Aim.Z, 0.0, Eps);
    }
    // test_intercept_aim_is_beyond_current_pos
    {
        const FVector Aim = FPursuitTactics::InterceptPoint(
            FVector(0, 0, 20), FVector(0, 0, 8), FVector::ZeroVector, 14.0);
        TestTrue(TEXT("aim beyond"), Aim.Z > 20.0);
    }
    // test_intercept_stationary_falls_back_to_pos
    {
        const FVector Aim = FPursuitTactics::InterceptPoint(
            FVector(7, 0, -3), FVector::ZeroVector, FVector::ZeroVector, 12.0);
        TestTrue(TEXT("stationary fallback"), Aim.Equals(FVector(7, 0, -3), Eps));
    }
    // test_intercept_zero_pursuer_speed_falls_back
    {
        const FVector Aim = FPursuitTactics::InterceptPoint(
            FVector(5, 0, 5), FVector(3, 0, 0), FVector::ZeroVector, 0.0);
        TestTrue(TEXT("zero speed fallback"), Aim.Equals(FVector(5, 0, 5), Eps));
    }
    // test_intercept_unreachable_slow_pursuer_falls_back
    {
        const FVector Aim = FPursuitTactics::InterceptPoint(
            FVector(10, 0, 0), FVector(20, 0, 0), FVector::ZeroVector, 5.0);
        TestTrue(TEXT("unreachable fallback"), Aim.Equals(FVector(10, 0, 0), Eps));
    }
    // test_intercept_equal_speed_linear_case
    {
        const FVector Aim = FPursuitTactics::InterceptPoint(
            FVector(0, 0, 10), FVector(6, 0, 0), FVector::ZeroVector, 6.0);
        TestTrue(TEXT("equal speed fallback"), Aim.Equals(FVector(0, 0, 10), Eps));
    }
    return true;
}

// --- should_ram -------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPursuitTacticsShouldRamTest,
    "GTC.AI.Pursuit.PursuitTactics.ShouldRam",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPursuitTacticsShouldRamTest::RunTest(const FString& Parameters)
{
    // test_ram_true_when_close_aligned_and_high_stars
    TestTrue(TEXT("ram true"), FPursuitTactics::ShouldRam(
        FVector::ZeroVector, FVector(1, 0, 0), FVector(4, 0, 0), 8.0, 4));
    // test_ram_false_at_low_stars_even_when_close
    TestFalse(TEXT("ram low stars"), FPursuitTactics::ShouldRam(
        FVector::ZeroVector, FVector(1, 0, 0), FVector(4, 0, 0), 8.0, 2));
    // test_ram_false_when_out_of_range
    TestFalse(TEXT("ram out of range"), FPursuitTactics::ShouldRam(
        FVector::ZeroVector, FVector(1, 0, 0), FVector(20, 0, 0), 8.0, 5));
    // test_ram_false_when_target_off_to_the_side
    TestFalse(TEXT("ram off side"), FPursuitTactics::ShouldRam(
        FVector::ZeroVector, FVector(1, 0, 0), FVector(0, 0, 4), 8.0, 5));
    // test_ram_at_exactly_threshold_stars
    TestTrue(TEXT("ram threshold stars"), FPursuitTactics::ShouldRam(
        FVector::ZeroVector, FVector(1, 0, 0), FVector(3, 0, 0), 8.0, 3));
    return true;
}

// --- block_offset -----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPursuitTacticsBlockOffsetTest,
    "GTC.AI.Pursuit.PursuitTactics.BlockOffset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPursuitTacticsBlockOffsetTest::RunTest(const FString& Parameters)
{
    // test_block_offset_is_ahead_and_picks_side
    {
        const FVector Right = FPursuitTactics::BlockOffset(
            FVector::ZeroVector, FVector(5, 0, 0), 1.0, 10.0);
        const FVector Left = FPursuitTactics::BlockOffset(
            FVector::ZeroVector, FVector(5, 0, 0), -1.0, 10.0);
        TestTrue(TEXT("ahead and side"), Right.X > 0.0 && Right.Z < 0.0 && Left.Z > 0.0);
    }
    // test_block_offset_stationary_steps_to_side
    {
        const FVector B = FPursuitTactics::BlockOffset(
            FVector(2, 0, 2), FVector::ZeroVector, 1.0, 6.0);
        TestTrue(TEXT("stationary side"), B.Equals(FVector(2, 0, 8), Eps));
    }
    return true;
}

// --- pit_side ---------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPursuitTacticsPitSideTest,
    "GTC.AI.Pursuit.PursuitTactics.PitSide",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPursuitTacticsPitSideTest::RunTest(const FString& Parameters)
{
    // test_pit_side_right_when_pursuer_on_right
    TestTrue(TEXT("pit right"), FPursuitTactics::PitSide(
        FVector(0, 0, -3), FVector::ZeroVector, FVector(5, 0, 0)) > 0.0);
    // test_pit_side_left_when_pursuer_on_left
    TestTrue(TEXT("pit left"), FPursuitTactics::PitSide(
        FVector(0, 0, 3), FVector::ZeroVector, FVector(5, 0, 0)) < 0.0);
    // test_pit_side_defaults_when_target_still
    TestEqual(TEXT("pit default"), FPursuitTactics::PitSide(
        FVector(1, 0, 1), FVector::ZeroVector, FVector::ZeroVector), 1.0, Eps);
    return true;
}

// --- desired_speed ----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPursuitTacticsDesiredSpeedTest,
    "GTC.AI.Pursuit.PursuitTactics.DesiredSpeed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPursuitTacticsDesiredSpeedTest::RunTest(const FString& Parameters)
{
    // test_desired_speed_rises_with_distance_and_caps
    {
        const double Near = FPursuitTactics::DesiredSpeed(15.0, 20.0, 40.0);
        const double Far = FPursuitTactics::DesiredSpeed(35.0, 20.0, 40.0);
        const double Capped = FPursuitTactics::DesiredSpeed(200.0, 20.0, 40.0);
        TestTrue(TEXT("rises"), Far > Near);
        TestEqual(TEXT("caps"), Capped, 40.0, Eps);
    }
    // test_desired_speed_eases_off_when_right_behind
    {
        const double Bumper = FPursuitTactics::DesiredSpeed(1.0, 20.0, 40.0);
        TestTrue(TEXT("eases off"), Bumper < 20.0);
    }
    return true;
}

// --- should_back_off --------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPursuitTacticsBackOffTest,
    "GTC.AI.Pursuit.PursuitTactics.BackOff",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPursuitTacticsBackOffTest::RunTest(const FString& Parameters)
{
    // test_back_off_true_at_zero_stars
    TestTrue(TEXT("back off zero stars"), FPursuitTactics::ShouldBackOff(0, 10.0));
    // test_back_off_true_when_target_far
    TestTrue(TEXT("back off far"), FPursuitTactics::ShouldBackOff(4, 200.0));
    // test_back_off_false_while_engaged
    TestFalse(TEXT("back off engaged"), FPursuitTactics::ShouldBackOff(3, 30.0));
    return true;
}

// --- choose_tactic ----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPursuitTacticsChooseTest,
    "GTC.AI.Pursuit.PursuitTactics.ChooseTactic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPursuitTacticsChooseTest::RunTest(const FString& Parameters)
{
    // test_choose_back_off_when_cleared
    TestTrue(TEXT("choose back off"), FPursuitTactics::ChooseTactic(
        FVector::ZeroVector, FVector(1, 0, 0), FVector(5, 0, 0), FVector(5, 0, 0), 0, 8.0)
        == EPursuitTactic::BackOff);
    // test_choose_ram_when_lined_up_and_authorised
    TestTrue(TEXT("choose ram"), FPursuitTactics::ChooseTactic(
        FVector::ZeroVector, FVector(1, 0, 0), FVector(4, 0, 0), FVector(5, 0, 0), 4, 8.0)
        == EPursuitTactic::Ram);
    // test_choose_chase_at_low_stars
    TestTrue(TEXT("choose chase"), FPursuitTactics::ChooseTactic(
        FVector::ZeroVector, FVector(1, 0, 0), FVector(4, 0, 0), FVector(5, 0, 0), 1, 8.0)
        == EPursuitTactic::Chase);
    // test_choose_pit_when_alongside_and_authorised
    TestTrue(TEXT("choose pit"), FPursuitTactics::ChooseTactic(
        FVector(0, 0, -5), FVector(1, 0, 0), FVector(5, 0, 0), FVector(5, 0, 0), 4, 8.0)
        == EPursuitTactic::Pit);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
