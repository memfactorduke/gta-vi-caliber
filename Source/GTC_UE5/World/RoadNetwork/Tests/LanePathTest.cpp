// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../LanePath.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FLanePath — lane offset geometry + arc-length travel. GTC-original
 * (no Godot oracle); the asserts pin the lane-side convention, arc-length pose
 * sampling, and the advance/clamp/reached-end behaviour a car relies on.
 */

namespace
{
    using GtcTest::VecNear;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLanePathTest,
    "GTC.World.LanePath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLanePathTest::RunTest(const FString& Parameters)
{
    // RightOf: rotate the travel heading -90° about +Y (XZ plane).
    TestTrue(TEXT("right of +X is -Z"), VecNear(FLanePath::RightOf(FVector(1, 0, 0)), FVector(0, 0, -1)));
    TestTrue(TEXT("right of +Z is +X"), VecNear(FLanePath::RightOf(FVector(0, 0, 1)), FVector(1, 0, 0)));
    TestTrue(TEXT("right of zero is zero"), VecNear(FLanePath::RightOf(FVector::ZeroVector), FVector::ZeroVector));

    // Straight lane along +X, offset 2 to the right (== -Z).
    {
        FLanePath Lane;
        Lane.BuildFromCenterline(TArray<FVector>({FVector(0, 0, 0), FVector(10, 0, 0), FVector(20, 0, 0)}), 2.0);

        TestEqual(TEXT("point count preserved"), Lane.NumPoints(), 3);
        TestTrue(TEXT("length is 20"), FMath::Abs(Lane.Length() - 20.0) <= Eps);
        TestTrue(TEXT("vertices shifted to -Z by offset"), VecNear(Lane.Point(0), FVector(0, 0, -2)));
        TestTrue(TEXT("last vertex shifted too"), VecNear(Lane.Point(2), FVector(20, 0, -2)));

        const FLanePath::FPose Mid = Lane.PoseAtDistance(5.0);
        TestTrue(TEXT("pose at s=5 sits mid-lane"), VecNear(Mid.Pos, FVector(5, 0, -2)));
        TestTrue(TEXT("pose heading is +X"), VecNear(Mid.Heading, FVector(1, 0, 0)));

        // Clamp past the end and the forward step's reached-end signal.
        const FLanePath::FPose End = Lane.PoseAtDistance(999.0);
        TestTrue(TEXT("pose clamps to lane end"), VecNear(End.Pos, FVector(20, 0, -2)));

        TestTrue(TEXT("distance to end from s=5"), FMath::Abs(Lane.DistanceToEnd(5.0) - 15.0) <= Eps);
        TestTrue(TEXT("distance to end clamps at 0"), FMath::Abs(Lane.DistanceToEnd(25.0)) <= Eps);

        bool bReached = false;
        const double S1 = Lane.Advance(18.0, 5.0, bReached);
        TestTrue(TEXT("advance clamps to length"), FMath::Abs(S1 - 20.0) <= Eps);
        TestTrue(TEXT("advance past end reports reached"), bReached);

        const double S2 = Lane.Advance(0.0, 5.0, bReached);
        TestTrue(TEXT("advance mid-lane moves s"), FMath::Abs(S2 - 5.0) <= Eps);
        TestFalse(TEXT("advance mid-lane not reached"), bReached);

        const double S3 = Lane.Advance(20.0, -3.0, bReached);
        TestTrue(TEXT("reverse advance moves back"), FMath::Abs(S3 - 17.0) <= Eps);
        TestFalse(TEXT("reverse advance never reports reached"), bReached);
    }

    // Right-angle corner: averaged-normal offset at the bend.
    {
        FLanePath Lane;
        Lane.BuildFromCenterline(TArray<FVector>({FVector(0, 0, 0), FVector(10, 0, 0), FVector(10, 0, 10)}), 2.0);

        TestTrue(TEXT("corner start offset -Z"), VecNear(Lane.Point(0), FVector(0, 0, -2)));
        TestTrue(TEXT("corner end offset +X"), VecNear(Lane.Point(2), FVector(12, 0, 10)));
        // Bend vertex shifts along the averaged right-normal (1,0,-1)/sqrt2 * 2.
        const double H = 2.0 / FMath::Sqrt(2.0);
        TestTrue(TEXT("corner bend uses averaged normal"),
            VecNear(Lane.Point(1), FVector(10.0 + H, 0.0, -H), 1e-4));
    }

    // Degenerate inputs never produce NaN/garbage.
    {
        FLanePath Empty;
        Empty.BuildFromCenterline(TArray<FVector>(), 2.0);
        TestEqual(TEXT("empty lane has no points"), Empty.NumPoints(), 0);
        TestTrue(TEXT("empty lane length 0"), FMath::Abs(Empty.Length()) <= Eps);
        const FLanePath::FPose P = Empty.PoseAtDistance(5.0);
        TestTrue(TEXT("empty pose is origin"), VecNear(P.Pos, FVector::ZeroVector));
        TestTrue(TEXT("empty pose forward fallback"), VecNear(P.Heading, FVector(0, 0, -1)));

        FLanePath Single;
        Single.BuildFromCenterline(TArray<FVector>({FVector(3, 0, 4)}), 2.0);
        TestEqual(TEXT("single-point lane keeps its point"), Single.NumPoints(), 1);
        TestTrue(TEXT("single lane length 0"), FMath::Abs(Single.Length()) <= Eps);
        TestTrue(TEXT("single pose is that point"), VecNear(Single.PoseAtDistance(9.0).Pos, FVector(3, 0, 4)));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
