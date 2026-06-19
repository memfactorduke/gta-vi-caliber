// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../HelicopterPursuit.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Parity tests for FHelicopterPursuit, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_helicopter_pursuit.gd. Tolerance mirrors the oracle's
 * is_equal_approx (Eps = 1e-4); bool equality asserts exactly. The deploy
 * threshold uses the merged FPoliceResponse (HelicopterStars == 3).
 *
 * Prefix GTC.AI.Pursuit.HelicopterPursuit. CENTER mirrors the oracle constant.
 *
 * PURE-CORE boundary: spotlight actors, AI Perception and Behavior Tree wiring
 * are the deferred Wave-3 adapter — the model takes plain geometry args. NOT
 * tested.
 */

namespace
{
    const FVector HelicopterPursuitCenter(12, 0, -5);
}

// --- should_deploy ----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHelicopterPursuitDeployTest,
    "GTC.AI.Pursuit.HelicopterPursuit.Deploy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHelicopterPursuitDeployTest::RunTest(const FString& Parameters)
{
    // test_no_chopper_below_three_stars
    TestFalse(TEXT("no chopper below 3"), FHelicopterPursuit::ShouldDeploy(2));
    // test_chopper_at_three_plus_stars
    TestTrue(TEXT("chopper at 3"), FHelicopterPursuit::ShouldDeploy(3));
    TestTrue(TEXT("chopper at 5"), FHelicopterPursuit::ShouldDeploy(5));
    return true;
}

// --- orbit_point ------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHelicopterPursuitOrbitTest,
    "GTC.AI.Pursuit.HelicopterPursuit.Orbit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHelicopterPursuitOrbitTest::RunTest(const FString& Parameters)
{
    // test_orbit_sits_on_radius
    {
        const FVector P = FHelicopterPursuit::OrbitPoint(
            HelicopterPursuitCenter, 0.0, 28.0, 32.0, 1.0);
        const FVector2D Planar(P.X - HelicopterPursuitCenter.X, P.Z - HelicopterPursuitCenter.Z);
        TestEqual(TEXT("on radius"), Planar.Size(), 28.0, Eps);
    }
    // test_orbit_holds_altitude
    {
        const FVector P = FHelicopterPursuit::OrbitPoint(
            HelicopterPursuitCenter, 1.3, 28.0, 32.0, 0.7);
        TestEqual(TEXT("holds altitude"), P.Y, HelicopterPursuitCenter.Y + 32.0, Eps);
    }
    // test_orbit_advances_with_time
    {
        const FVector A = FHelicopterPursuit::OrbitPoint(
            HelicopterPursuitCenter, 0.0, 28.0, 32.0, 1.0);
        const FVector B = FHelicopterPursuit::OrbitPoint(
            HelicopterPursuitCenter, 1.0, 28.0, 32.0, 1.0);
        TestTrue(TEXT("advances"), FVector::Dist(A, B) > 0.1);
    }
    // test_orbit_periodic
    {
        const FVector A = FHelicopterPursuit::OrbitPoint(
            HelicopterPursuitCenter, 0.0, 28.0, 32.0, 1.0);
        const FVector B = FHelicopterPursuit::OrbitPoint(
            HelicopterPursuitCenter, UE_DOUBLE_TWO_PI, 28.0, 32.0, 1.0);
        TestTrue(TEXT("periodic"), FVector::Dist(A, B) < 0.001);
    }
    return true;
}

// --- cone / spotlight radius ------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHelicopterPursuitConeTest,
    "GTC.AI.Pursuit.HelicopterPursuit.Cone",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHelicopterPursuitConeTest::RunTest(const FString& Parameters)
{
    // test_cone_half_radians_converts_and_clamps
    TestEqual(TEXT("cone converts"), FHelicopterPursuit::ConeHalfRadians(22.0),
        FMath::DegreesToRadians(22.0), Eps);
    TestTrue(TEXT("cone clamps below 90"),
        FHelicopterPursuit::ConeHalfRadians(200.0) < FMath::DegreesToRadians(90.0));
    // test_spotlight_radius_grows_with_altitude
    {
        const double Half = FHelicopterPursuit::ConeHalfRadians(22.0);
        const double Low = FHelicopterPursuit::SpotlightGroundRadius(20.0, Half);
        const double High = FHelicopterPursuit::SpotlightGroundRadius(40.0, Half);
        TestTrue(TEXT("grows with altitude"), High > Low && Low > 0.0);
    }
    // test_spotlight_radius_matches_trig
    {
        const double Half = FHelicopterPursuit::ConeHalfRadians(22.0);
        TestEqual(TEXT("matches trig"),
            FHelicopterPursuit::SpotlightGroundRadius(32.0, Half), 32.0 * FMath::Tan(Half), Eps);
    }
    return true;
}

// --- target_lit -------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHelicopterPursuitTargetLitTest,
    "GTC.AI.Pursuit.HelicopterPursuit.TargetLit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHelicopterPursuitTargetLitTest::RunTest(const FString& Parameters)
{
    // test_target_lit_inside_footprint
    TestTrue(TEXT("inside footprint"), FHelicopterPursuit::TargetLit(
        FVector(0, 0, 0), FVector(3, 0, 2), 10.0));
    // test_target_dark_outside_footprint
    TestFalse(TEXT("outside footprint"), FHelicopterPursuit::TargetLit(
        FVector(0, 0, 0), FVector(20, 0, 0), 10.0));
    // test_target_lit_ignores_height
    TestTrue(TEXT("ignores height"), FHelicopterPursuit::TargetLit(
        FVector(0, 30, 0), FVector(2, -20, 1), 10.0));
    return true;
}

#endif // WITH_AUTOMATION_TESTS
