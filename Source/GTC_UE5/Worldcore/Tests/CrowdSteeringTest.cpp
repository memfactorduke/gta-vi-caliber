// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../CrowdSteering.h"
#include "../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Parity tests for FCrowdSteering (boids), mapped 1:1 from the C++ oracle
 * engine/tests/test_worldcore.cpp (the worldcore_crowd asserts).
 * Prefix GTC.Worldcore.CrowdSteering. Core Vec2{x,z} -> FVector2D (X<-x, Y<-z).
 *
 * PURE-CORE boundary: pure steering-force math only; Mass/actor integration is
 * the deferred Wave-3 adapter and is NOT tested here.
 */

// --- separation -------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrowdSeparationTest,
    "GTC.Worldcore.CrowdSteering.Separation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrowdSeparationTest::RunTest(const FString& Parameters)
{
    // test_separation_pushes_away: neighbour to +x => pushed -x.
    {
        const TArray<FVector2D> Neighbors = {FVector2D(1.0, 0.0)};
        const FVector2D F = FCrowdSteering::Separation(FVector2D(0.0, 0.0), Neighbors, 4.0);
        TestTrue(TEXT("pushed -x away from neighbour"), F.X < 0.0);
        TestTrue(TEXT("no z component"), FMath::Abs(F.Y) < Eps);
    }
    // test_separation_ignores_outside_radius
    {
        const TArray<FVector2D> Neighbors = {FVector2D(100.0, 0.0)};
        const FVector2D F = FCrowdSteering::Separation(FVector2D(0.0, 0.0), Neighbors, 4.0);
        TestTrue(TEXT("ignores outside radius"), F.Size() < Eps);
    }
    return true;
}

// --- cohesion ---------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrowdCohesionTest,
    "GTC.Worldcore.CrowdSteering.Cohesion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrowdCohesionTest::RunTest(const FString& Parameters)
{
    // test_cohesion_pulls_toward_centroid
    const TArray<FVector2D> Neighbors = {FVector2D(10.0, 0.0), FVector2D(10.0, 0.0)};
    const FVector2D F = FCrowdSteering::Cohesion(FVector2D(0.0, 0.0), Neighbors);
    TestTrue(TEXT("pulls toward +x cluster"), F.X > 0.0);
    return true;
}

// --- alignment --------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrowdAlignmentTest,
    "GTC.Worldcore.CrowdSteering.Alignment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrowdAlignmentTest::RunTest(const FString& Parameters)
{
    // test_alignment_is_average_heading
    const TArray<FVector2D> Vels = {FVector2D(2.0, 0.0), FVector2D(0.0, 2.0)};
    const FVector2D F = FCrowdSteering::Alignment(Vels);
    TestEqual(TEXT("avg heading x"), F.X, 1.0, Eps);
    TestEqual(TEXT("avg heading z"), F.Y, 1.0, Eps);
    return true;
}

// --- combine ----------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrowdCombineTest,
    "GTC.Worldcore.CrowdSteering.Combine",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrowdCombineTest::RunTest(const FString& Parameters)
{
    // test_combine_clamps_to_max_force
    const FVector2D Big(100.0, 0.0);
    const FVector2D F = FCrowdSteering::Combine(Big, Big, Big, 1.0, 1.0, 1.0, 8.0);
    TestTrue(TEXT("clamped to max force"), F.Size() <= 8.0 + Eps);
    return true;
}

// --- empty neighbours -------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrowdEmptyNeighborsTest,
    "GTC.Worldcore.CrowdSteering.EmptyNeighbors",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrowdEmptyNeighborsTest::RunTest(const FString& Parameters)
{
    // test_empty_neighbors_zero_force
    const TArray<FVector2D> None;
    TestTrue(TEXT("separation zero"), FCrowdSteering::Separation(FVector2D(0.0, 0.0), None, 4.0).Size() < Eps);
    TestTrue(TEXT("cohesion zero"), FCrowdSteering::Cohesion(FVector2D(0.0, 0.0), None).Size() < Eps);
    TestTrue(TEXT("alignment zero"), FCrowdSteering::Alignment(None).Size() < Eps);
    return true;
}

// --- clamp_len --------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrowdClampLenTest,
    "GTC.Worldcore.CrowdSteering.ClampLen",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrowdClampLenTest::RunTest(const FString& Parameters)
{
    // test_clamp_len_negative_budget_is_zero: negative max_len must not flip direction.
    const FVector2D F = FCrowdSteering::ClampLen(FVector2D(5.0, 0.0), -8.0);
    TestTrue(TEXT("negative budget is zero"), F.Size() < Eps);
    return true;
}

// --- arrive -----------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrowdArriveTest,
    "GTC.Worldcore.CrowdSteering.Arrive",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrowdArriveTest::RunTest(const FString& Parameters)
{
    // test_arrive_seeks_then_slows
    const FVector2D Far =
        FCrowdSteering::Arrive(FVector2D(0.0, 0.0), FVector2D(0.0, 0.0), FVector2D(100.0, 0.0), 5.0, 6.0);
    TestTrue(TEXT("far seeks +x"), Far.X > 0.0);
    TestEqual(TEXT("far at max speed"), Far.Size(), 6.0, Eps);
    const FVector2D Near =
        FCrowdSteering::Arrive(FVector2D(0.0, 0.0), FVector2D(0.0, 0.0), FVector2D(2.5, 0.0), 5.0, 6.0);
    TestTrue(TEXT("near force smaller than far"), Near.Size() < Far.Size());
    return true;
}

// --- avoid_obstacles --------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrowdAvoidObstaclesTest,
    "GTC.Worldcore.CrowdSteering.AvoidObstacles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrowdAvoidObstaclesTest::RunTest(const FString& Parameters)
{
    // test_avoid_pushes_off_obstacle
    {
        const TArray<FVector2D> Obstacles = {FVector2D(2.0, 0.0)}; // obstacle to +x
        const TArray<double> Radii = {3.0};
        const FVector2D F = FCrowdSteering::AvoidObstacles(FVector2D(0.0, 0.0), Obstacles, Radii, 1.0);
        TestTrue(TEXT("pushed -x away"), F.X < 0.0);
    }
    // Obstacle far outside radius+margin exerts no force.
    {
        const TArray<FVector2D> FarO = {FVector2D(100.0, 0.0)};
        const TArray<double> Radii = {3.0};
        TestTrue(TEXT("far obstacle no force"),
            FCrowdSteering::AvoidObstacles(FVector2D(0.0, 0.0), FarO, Radii, 1.0).Size() < Eps);
    }
    // Degenerate obstacle (zero radius + zero margin) exerts no force at the centre.
    {
        const TArray<FVector2D> Here = {FVector2D(0.0, 0.0)};
        const TArray<double> Zero = {0.0};
        TestTrue(TEXT("degenerate obstacle no force"),
            FCrowdSteering::AvoidObstacles(FVector2D(0.0, 0.0), Here, Zero, 0.0).Size() < Eps);
    }
    return true;
}

#endif // WITH_AUTOMATION_TESTS
