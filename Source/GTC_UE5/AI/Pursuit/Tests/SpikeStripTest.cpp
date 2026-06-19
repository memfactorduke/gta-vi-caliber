// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../SpikeStrip.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FSpikeStrip — segment-crossing detection and endpoint layout.
 * Prefix GTC.AI.Pursuit.SpikeStrip.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSpikeStripTest,
    "GTC.AI.Pursuit.SpikeStrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpikeStripTest::RunTest(const FString& Parameters)
{
    // Strip laid across the X axis at x=10, spanning y=[-5,5].
    const FVector A(10, -5, 0);
    const FVector B(10, 5, 0);

    // --- Crosses ---
    TestTrue(TEXT("driving straight over the strip crosses it"),
        FSpikeStrip::Crosses(FVector(5, 0, 0), FVector(15, 0, 0), A, B));
    TestFalse(TEXT("stopping short of the strip does not cross"),
        FSpikeStrip::Crosses(FVector(5, 0, 0), FVector(9, 0, 0), A, B));
    TestFalse(TEXT("driving parallel beside the strip does not cross"),
        FSpikeStrip::Crosses(FVector(5, 8, 0), FVector(15, 8, 0), A, B)); // y=8 is past the strip end
    TestFalse(TEXT("driving alongside (parallel to) the strip does not cross"),
        FSpikeStrip::Crosses(FVector(12, -5, 0), FVector(12, 5, 0), A, B));
    TestTrue(TEXT("a diagonal path through the strip crosses it"),
        FSpikeStrip::Crosses(FVector(8, -2, 0), FVector(12, 2, 0), A, B));
    TestFalse(TEXT("a zero-length path cannot cross"),
        FSpikeStrip::Crosses(FVector(10, 0, 0), FVector(10, 0, 0), A, B));

    // --- Endpoints ---
    {
        FVector OutA, OutB;
        FSpikeStrip::Endpoints(FVector(10, 0, 0), FVector(0, 1, 0), 10.0, OutA, OutB);
        TestTrue(TEXT("endpoint A is half-width to the left"),
            FMath::IsNearlyEqual(OutA.Y, -5.0, GtcTest::Eps));
        TestTrue(TEXT("endpoint B is half-width to the right"),
            FMath::IsNearlyEqual(OutB.Y, 5.0, GtcTest::Eps));
        TestTrue(TEXT("endpoints share the centre's X"),
            FMath::IsNearlyEqual(OutA.X, 10.0, GtcTest::Eps) && FMath::IsNearlyEqual(OutB.X, 10.0, GtcTest::Eps));

        // The laid strip is crossable by a car driving over its centre.
        TestTrue(TEXT("a car crosses the laid strip"),
            FSpikeStrip::Crosses(FVector(5, 0, 0), FVector(15, 0, 0), OutA, OutB));

        // Degenerate right collapses to the centre.
        FVector DA, DB;
        FSpikeStrip::Endpoints(FVector(10, 0, 0), FVector::ZeroVector, 10.0, DA, DB);
        TestTrue(TEXT("degenerate right collapses both endpoints to the centre"),
            (DA - FVector(10, 0, 0)).IsNearlyZero() && (DB - FVector(10, 0, 0)).IsNearlyZero());
    }

    // --- Effect multipliers are penalties (< 1) ---
    TestTrue(TEXT("deflated grip is a penalty"), FSpikeStrip::DeflatedGripMult < 1.0);
    TestTrue(TEXT("deflated top speed is a penalty"), FSpikeStrip::DeflatedTopSpeedMult < 1.0);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
