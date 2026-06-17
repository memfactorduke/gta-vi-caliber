// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../AnimRouter.h"
#include "../Locomotion.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

#include <limits>

/**
 * Parity tests for FAnimRouter, mapped 1:1 from the Godot oracles
 * game/tests/unit/test_anim_router.gd (TravelTarget / MoveBlendValue /
 * DominantBlendPoint) and game/tests/unit/test_anim_router_facing.gd
 * (RotateTowardAngle / FacingTarget). FName comparisons assert exactly;
 * scalar checks use the shared GtcTest::Eps (1e-4 double) tolerance.
 */

using GtcTest::Eps;

namespace
{
    // The remaining constants keep a unique prefix so they survive unity-build
    // concatenation alongside sibling test files (the shared header handles Eps).
    constexpr double ArWalk = 5.0;
    constexpr double ArRun = 8.5;
    constexpr double ArLandSkip = 2.0;

    FName ArRoute(FLocomotion::EState State, FName Current, double PlanarSpeed = 0.0)
    {
        return FAnimRouter::TravelTarget(State, Current, PlanarSpeed, ArLandSkip);
    }
}

// --- travel_target --------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAnimRouterTravelTargetTest,
    "GTC.Player.Locomotion.AnimRouter.TravelTarget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAnimRouterTravelTargetTest::RunTest(const FString& Parameters)
{
    // test_idle_routes_to_move
    TestEqual(TEXT("idle -> move"),
        ArRoute(FLocomotion::EState::Idle, FAnimRouter::StateMove), FAnimRouter::StateMove);
    // test_walk_routes_to_move
    TestEqual(TEXT("walk -> move"),
        ArRoute(FLocomotion::EState::Walk, FAnimRouter::StateMove, ArWalk), FAnimRouter::StateMove);
    // test_run_routes_to_move
    TestEqual(TEXT("run -> move"),
        ArRoute(FLocomotion::EState::Run, FAnimRouter::StateMove, ArRun), FAnimRouter::StateMove);
    // test_climb_routes_to_move
    TestEqual(TEXT("climb -> move"),
        ArRoute(FLocomotion::EState::Climb, FAnimRouter::StateMove), FAnimRouter::StateMove);
    // test_jump_from_ground_starts_jump
    TestEqual(TEXT("jump from ground -> jump start"),
        ArRoute(FLocomotion::EState::Jump, FAnimRouter::StateMove), FAnimRouter::StateJumpStart);
    // test_jump_during_land_starts_jump
    TestEqual(TEXT("jump during land -> jump start"),
        ArRoute(FLocomotion::EState::Jump, FAnimRouter::StateLand), FAnimRouter::StateJumpStart);
    // test_rising_keeps_jump_start_playing
    TestEqual(TEXT("rising keeps jump start"),
        ArRoute(FLocomotion::EState::Jump, FAnimRouter::StateJumpStart), FAnimRouter::StateJumpStart);
    // test_rising_keeps_air_loop
    TestEqual(TEXT("rising keeps air"),
        ArRoute(FLocomotion::EState::Jump, FAnimRouter::StateAir), FAnimRouter::StateAir);
    // test_fall_lets_jump_start_finish
    TestEqual(TEXT("fall lets jump start finish"),
        ArRoute(FLocomotion::EState::Fall, FAnimRouter::StateJumpStart), FAnimRouter::StateJumpStart);
    // test_fall_off_ledge_routes_to_air
    TestEqual(TEXT("fall off ledge -> air"),
        ArRoute(FLocomotion::EState::Fall, FAnimRouter::StateMove), FAnimRouter::StateAir);
    // test_slow_landing_plays_land
    TestEqual(TEXT("slow landing -> land"),
        ArRoute(FLocomotion::EState::Idle, FAnimRouter::StateAir, 0.5), FAnimRouter::StateLand);
    // test_fast_landing_skips_land
    TestEqual(TEXT("fast landing -> move"),
        ArRoute(FLocomotion::EState::Run, FAnimRouter::StateAir, ArRun), FAnimRouter::StateMove);
    // test_short_hop_lands_from_jump_start
    TestEqual(TEXT("short hop lands from jump start"),
        ArRoute(FLocomotion::EState::Idle, FAnimRouter::StateJumpStart, 0.0), FAnimRouter::StateLand);
    // test_grounded_during_land_stays_in_land
    TestEqual(TEXT("grounded during land stays in land"),
        ArRoute(FLocomotion::EState::Idle, FAnimRouter::StateLand), FAnimRouter::StateLand);

    return true;
}

// --- move_blend_value -----------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAnimRouterMoveBlendValueTest,
    "GTC.Player.Locomotion.AnimRouter.MoveBlendValue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAnimRouterMoveBlendValueTest::RunTest(const FString& Parameters)
{
    // test_move_blend_uses_planar_speed_on_ground
    TestEqual(TEXT("uses planar speed on ground"),
        FAnimRouter::MoveBlendValue(ArWalk, ArWalk, false, ArWalk, ArRun),
        FLocomotion::MoveBlend(ArWalk, ArWalk, ArRun), Eps);
    // test_move_blend_uses_total_speed_while_climbing
    TestEqual(TEXT("uses total speed while climbing"),
        FAnimRouter::MoveBlendValue(0.0, 3.0, true, ArWalk, ArRun),
        FLocomotion::MoveBlend(3.0, ArWalk, ArRun), Eps);

    return true;
}

// --- dominant_blend_point -------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAnimRouterDominantBlendPointTest,
    "GTC.Player.Locomotion.AnimRouter.DominantBlendPoint",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAnimRouterDominantBlendPointTest::RunTest(const FString& Parameters)
{
    const TArray<float> Points = {0.5f, 0.8f, 1.0f};

    // test_dominant_point_picks_nearest
    TestEqual(TEXT("picks nearest"), FAnimRouter::DominantBlendPoint(0.55, Points), 0.5, Eps);
    // test_dominant_point_below_range_clamps_to_first
    TestEqual(TEXT("below range clamps to first"), FAnimRouter::DominantBlendPoint(0.1, Points), 0.5, Eps);
    // test_dominant_point_tie_prefers_faster_clip
    TestEqual(TEXT("tie prefers faster clip"), FAnimRouter::DominantBlendPoint(0.65, Points), 0.8, Eps);
    // test_dominant_point_at_sprint
    TestEqual(TEXT("at sprint"), FAnimRouter::DominantBlendPoint(1.0, Points), 1.0, Eps);
    // test_dominant_point_empty_points_is_nan
    TestTrue(TEXT("empty points is NaN"),
        FMath::IsNaN(FAnimRouter::DominantBlendPoint(0.5, TArray<float>())));

    return true;
}

// --- rotate_toward_angle / facing_target ----------------------------------
// Mapped 1:1 from game/tests/unit/test_anim_router_facing.gd. All inputs stay
// in the Godot XZ frame (Y up) — NO axis remap — so the oracle holds bit-for-bit.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAnimRouterFacingTest,
    "GTC.Player.Locomotion.AnimRouter.Facing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAnimRouterFacingTest::RunTest(const FString& Parameters)
{
    // test_rotate_toward_caps_step: is_equal_approx(0.25)
    TestEqual(TEXT("rotate caps step"),
        FAnimRouter::RotateTowardAngle(0.0, 1.0, 0.25), 0.25, Eps);
    // test_rotate_toward_reaches_target_within_step: is_equal_approx(0.1)
    TestEqual(TEXT("rotate reaches target within step"),
        FAnimRouter::RotateTowardAngle(0.0, 0.1, 0.25), 0.1, Eps);
    // test_rotate_toward_takes_shortest_arc: from 3.0 toward -3.0, short way crosses
    // the +PI seam (positive direction) -> is_equal_approx(3.1).
    TestEqual(TEXT("rotate takes shortest arc"),
        FAnimRouter::RotateTowardAngle(3.0, -3.0, 0.1), 3.1, Eps);

    // test_facing_prefers_aim_yaw: aim active -> is_equal_approx(1.2)
    TestEqual(TEXT("facing prefers aim yaw"),
        FAnimRouter::FacingTarget(FVector(0.0, 0.0, 5.0), 1.2), 1.2, Eps);
    // test_facing_follows_travel_direction: NAN aim -> atan2(velocity.x, velocity.z)
    TestEqual(TEXT("facing follows travel direction"),
        FAnimRouter::FacingTarget(FVector(5.0, 0.0, 0.0), std::numeric_limits<double>::quiet_NaN()),
        FMath::Atan2(5.0, 0.0), Eps);

    // test_facing_points_local_positive_z_along_velocity: the yaw rotates Godot's
    // local +Z (Vector3.BACK) about +Y (Vector3.UP) to point along the velocity.
    // Basis(UP, yaw) * BACK == (sin(yaw), 0, cos(yaw)) in the Godot XZ frame.
    {
        const FVector Velocity = FVector(2.0, 0.0, -3.0).GetSafeNormal();
        const double Yaw = FAnimRouter::FacingTarget(Velocity, std::numeric_limits<double>::quiet_NaN());
        const FVector WorldFacing(FMath::Sin(Yaw), 0.0, FMath::Cos(Yaw));
        TestTrue(TEXT("facing points local +Z along velocity"),
            FVector::DotProduct(WorldFacing, Velocity) > 0.999);
    }

    // test_facing_keeps_current_when_still_and_unaimed: zero velocity, NAN aim -> NaN
    TestTrue(TEXT("facing keeps current when still and unaimed"),
        FMath::IsNaN(FAnimRouter::FacingTarget(FVector::ZeroVector, std::numeric_limits<double>::quiet_NaN())));
    // test_facing_ignores_creep_below_idle_epsilon: sub-epsilon creep -> NaN
    TestTrue(TEXT("facing ignores creep below idle epsilon"),
        FMath::IsNaN(FAnimRouter::FacingTarget(FVector(0.05, 0.0, 0.0), std::numeric_limits<double>::quiet_NaN())));

    return true;
}

#endif // WITH_AUTOMATION_TESTS
