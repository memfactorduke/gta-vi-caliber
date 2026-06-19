// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../GTCMinimapProjection.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

// ============================================================================
// GTC.UI.Minimap — heading-up world->disc projection. Pure math, headless, like
// the GPS parity tests. Verifies forward maps to +Y / right to +X, that turning
// the player rotates the world under the arrow, and that off-range points clamp
// to the rim. UE XY ground frame, yaw 0 faces +X.
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGTCMinimapProjectionHeadingUpTest,
    "GTC.UI.Minimap.Projection.HeadingUp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGTCMinimapProjectionHeadingUpTest::RunTest(const FString& Parameters)
{
    FGTCMinimapProjection View;
    View.Center = FVector(1000.0, 2000.0, 50.0); // nonzero center + height to confirm the delta/Z-drop
    View.RangeCm = 10000.0;
    View.HeadingRadians = 0.0; // facing +X

    // Directly ahead (+X at range) -> top of the disc (0, 1).
    {
        const FVector2D N = View.WorldToNormalized(View.Center + FVector(10000.0, 0.0, 999.0));
        TestEqual(TEXT("ahead.X"), N.X, 0.0, Eps);
        TestEqual(TEXT("ahead.Y"), N.Y, 1.0, Eps);
    }
    // To the player's right (+Y at range) -> right of the disc (1, 0).
    {
        const FVector2D N = View.WorldToNormalized(View.Center + FVector(0.0, 10000.0, 0.0));
        TestEqual(TEXT("right.X"), N.X, 1.0, Eps);
        TestEqual(TEXT("right.Y"), N.Y, 0.0, Eps);
    }

    // Turn to face +Y (yaw 90): now +Y is ahead and +X is to the LEFT.
    View.HeadingRadians = FMath::DegreesToRadians(90.0);
    {
        const FVector2D Ahead = View.WorldToNormalized(View.Center + FVector(0.0, 10000.0, 0.0));
        TestEqual(TEXT("yaw90 ahead.X"), Ahead.X, 0.0, Eps);
        TestEqual(TEXT("yaw90 ahead.Y"), Ahead.Y, 1.0, Eps);

        const FVector2D Left = View.WorldToNormalized(View.Center + FVector(10000.0, 0.0, 0.0));
        TestEqual(TEXT("yaw90 left.X"), Left.X, -1.0, Eps);
        TestEqual(TEXT("yaw90 left.Y"), Left.Y, 0.0, Eps);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGTCMinimapProjectionClampTest,
    "GTC.UI.Minimap.Projection.ClampToRim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGTCMinimapProjectionClampTest::RunTest(const FString& Parameters)
{
    FGTCMinimapProjection View;
    View.RangeCm = 10000.0;
    View.HeadingRadians = 0.0;

    // Inside the disc -> passes through, not flagged.
    {
        bool bEdge = true;
        const FVector2D P = View.Project(FVector(5000.0, 0.0, 0.0), bEdge);
        TestFalse(TEXT("inside not on edge"), bEdge);
        TestEqual(TEXT("inside.Y"), P.Y, 0.5, Eps);
    }
    // Twice the range -> pulled to the rim (length 1), flagged on-edge, direction kept.
    {
        bool bEdge = false;
        const FVector2D P = View.Project(FVector(20000.0, 0.0, 0.0), bEdge);
        TestTrue(TEXT("outside on edge"), bEdge);
        TestEqual(TEXT("rim length 1"), P.Size(), 1.0, Eps);
        TestEqual(TEXT("rim direction kept (Y)"), P.Y, 1.0, Eps);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
