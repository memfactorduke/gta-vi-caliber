// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../RoadRoute.h"
#include "../LanePath.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FRoadRoute — node path -> drivable centerline. GTC-original. The
 * final block threads the centerline into FLanePath to show the full route ->
 * lane pipeline a car rides.
 */

namespace
{
    bool VecNear(const FVector& A, const FVector& B, double Tol = 1e-4)
    {
        return FMath::Abs(A.X - B.X) <= Tol && FMath::Abs(A.Y - B.Y) <= Tol
            && FMath::Abs(A.Z - B.Z) <= Tol;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRoadRouteTest,
    "GTC.World.RoadRoute",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRoadRouteTest::RunTest(const FString& Parameters)
{
    const TArray<FVector> Nodes({
        FVector(0, 0, 0), FVector(10, 0, 0), FVector(10, 0, 10), FVector(20, 0, 10)});

    // Centerline is the node positions in path order.
    {
        const TArray<FVector> C = FRoadRoute::Centerline(Nodes, TArray<int32>({0, 1, 2, 3}));
        TestEqual(TEXT("centerline has 4 points"), C.Num(), 4);
        TestTrue(TEXT("first point is node 0"), VecNear(C[0], Nodes[0]));
        TestTrue(TEXT("last point is node 3"), VecNear(C[3], Nodes[3]));
    }

    // Malformed paths yield no road rather than a phantom line.
    TestTrue(TEXT("out-of-range index -> empty"), FRoadRoute::Centerline(Nodes, TArray<int32>({0, 9})).IsEmpty());
    TestTrue(TEXT("empty path -> empty"), FRoadRoute::Centerline(Nodes, TArray<int32>()).IsEmpty());

    // Threading an arbitrary spawn/destination on the ends.
    {
        const TArray<FVector> C = FRoadRoute::CenterlineThrough(
            Nodes, TArray<int32>({0, 1, 2, 3}), FVector(-5, 0, 0), FVector(25, 0, 10));
        TestEqual(TEXT("threaded centerline has 6 points"), C.Num(), 6);
        TestTrue(TEXT("threaded starts at spawn"), VecNear(C[0], FVector(-5, 0, 0)));
        TestTrue(TEXT("threaded ends at destination"), VecNear(C[5], FVector(25, 0, 10)));
    }

    // A spawn/destination that coincides with the end node is not duplicated.
    {
        const TArray<FVector> C = FRoadRoute::CenterlineThrough(
            Nodes, TArray<int32>({0, 1, 2, 3}), FVector(0, 0, 0), FVector(20, 0, 10));
        TestEqual(TEXT("coincident ends are not duplicated"), C.Num(), 4);
    }

    // route -> lane pipeline: the centerline drives an FLanePath.
    {
        const TArray<FVector> C = FRoadRoute::Centerline(Nodes, TArray<int32>({0, 1, 2, 3}));
        FLanePath Lane;
        Lane.BuildFromCenterline(C, 2.0);
        TestEqual(TEXT("lane keeps the route's vertices"), Lane.NumPoints(), 4);
        TestTrue(TEXT("lane has positive drivable length"), Lane.Length() > Eps);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
