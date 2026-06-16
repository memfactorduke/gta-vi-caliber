// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../FlowField.h"
#include "../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Parity tests for FFlowField (Dijkstra flow-field), mapped 1:1 from the C++
 * oracle engine/tests/test_worldcore.cpp (the worldcore_flow asserts).
 * Prefix GTC.Worldcore.FlowField. Core Vec2{x,z} -> FVector2D (X<-x, Y<-z).
 *
 * PURE-CORE boundary: pure integration + flow-direction math only; Mass/actor
 * integration is the deferred Wave-3 adapter and is NOT tested here.
 */

// --- flow open points to goal -----------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFlowFieldOpenTest,
    "GTC.Worldcore.FlowField.OpenPointsToGoal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlowFieldOpenTest::RunTest(const FString& Parameters)
{
    // test_flow_open_points_to_goal
    FFlowField::FGrid G{5, 5};
    TArray<double> Costs;
    Costs.Init(1.0, 25);
    const int32 Goal = G.Index(2, 2);
    const TArray<double> Dist = FFlowField::Integrate(G, Costs, Goal);
    const TArray<FVector2D> Flow = FFlowField::FlowFrom(G, Costs, Dist);
    const FVector2D F = Flow[G.Index(0, 0)]; // corner steers toward centre (+x,+z)
    TestTrue(TEXT("corner steers +x"), F.X > 0.0);
    TestTrue(TEXT("corner steers +z"), F.Y > 0.0);
    TestEqual(TEXT("goal flow x zero"), Flow[Goal].X, 0.0);
    TestEqual(TEXT("goal flow z zero"), Flow[Goal].Y, 0.0);
    return true;
}

// --- flow wall blocks unreachable -------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFlowFieldWallTest,
    "GTC.Worldcore.FlowField.WallBlocksUnreachable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlowFieldWallTest::RunTest(const FString& Parameters)
{
    // test_flow_wall_blocks_unreachable
    FFlowField::FGrid G{3, 1};
    TArray<double> Costs = {1.0, -1.0, 1.0}; // wall in the middle
    const TArray<double> Dist = FFlowField::Integrate(G, Costs, 2);
    const TArray<FVector2D> Flow = FFlowField::FlowFrom(G, Costs, Dist);
    TestFalse(TEXT("left cell unreachable past wall"), FMath::IsFinite(Dist[0]));
    TestEqual(TEXT("left flow x zero"), Flow[0].X, 0.0);
    TestEqual(TEXT("left flow z zero"), Flow[0].Y, 0.0);
    return true;
}

// --- flow routes around wall ------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFlowFieldRouteAroundTest,
    "GTC.Worldcore.FlowField.RoutesAroundWall",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlowFieldRouteAroundTest::RunTest(const FString& Parameters)
{
    // test_flow_routes_around_wall
    FFlowField::FGrid G{3, 3};
    TArray<double> Costs;
    Costs.Init(1.0, 9);
    Costs[G.Index(1, 0)] = -1.0; // wall
    Costs[G.Index(1, 1)] = -1.0; // wall — blocks the straight path to the goal
    const int32 Goal = G.Index(2, 1);
    const TArray<double> Dist = FFlowField::Integrate(G, Costs, Goal);
    const TArray<FVector2D> Flow = FFlowField::FlowFrom(G, Costs, Dist);
    const FVector2D F = Flow[G.Index(0, 1)]; // left-middle must route down around the wall
    TestTrue(TEXT("heads +z around wall"), F.Y > 0.0);
    return true;
}

// --- flow no diagonal corner cut --------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFlowFieldNoCornerCutTest,
    "GTC.Worldcore.FlowField.NoDiagonalCornerCut",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlowFieldNoCornerCutTest::RunTest(const FString& Parameters)
{
    // test_flow_no_diagonal_corner_cut
    FFlowField::FGrid G{2, 2};
    // goal at (0,0); walls at (1,0) and (0,1) box in the diagonal cell (1,1).
    TArray<double> Costs = {1.0, -1.0, -1.0, 1.0};
    const TArray<double> Dist = FFlowField::Integrate(G, Costs, G.Index(0, 0));
    const TArray<FVector2D> Flow = FFlowField::FlowFrom(G, Costs, Dist);
    // (1,1) must NOT reach the goal by cutting the wall corner.
    TestFalse(TEXT("diagonal cell unreachable"), FMath::IsFinite(Dist[G.Index(1, 1)]));
    TestEqual(TEXT("diagonal flow x zero"), Flow[G.Index(1, 1)].X, 0.0);
    TestEqual(TEXT("diagonal flow z zero"), Flow[G.Index(1, 1)].Y, 0.0);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
