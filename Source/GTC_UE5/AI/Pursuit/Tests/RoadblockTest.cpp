// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../Roadblock.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FRoadblock — pursuit barricade lead distance, placement, unit spread,
 * and the has-passed test. Prefix GTC.AI.Pursuit.Roadblock.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRoadblockTest,
    "GTC.AI.Pursuit.Roadblock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRoadblockTest::RunTest(const FString& Parameters)
{
    const FVector Flee(1, 0, 0);
    const FVector Right(0, 1, 0);

    // --- LeadDistance ---
    TestTrue(TEXT("a fast player gets the block set up farther ahead"),
        FRoadblock::LeadDistance(2000.0) > FRoadblock::LeadDistance(500.0));
    TestTrue(TEXT("lead never drops below the minimum"),
        FMath::IsNearlyEqual(FRoadblock::LeadDistance(0.0), FRoadblock::MinLeadDistance, GtcTest::Eps));

    // --- BlockadeCenter ---
    {
        const double Lead = 3000.0;
        const FVector C = FRoadblock::BlockadeCenter(FVector::ZeroVector, Flee, Lead);
        TestTrue(TEXT("center is placed ahead along the flee vector"),
            FMath::IsNearlyEqual(C.X, Lead, GtcTest::Eps));
        TestTrue(TEXT("degenerate flee dir keeps the player position"),
            (FRoadblock::BlockadeCenter(FVector(5, 5, 0), FVector::ZeroVector, Lead) - FVector(5, 5, 0)).IsNearlyZero());
    }

    // --- UnitsToSpan / FullyBlocks ---
    {
        const double Width = FRoadblock::UnitWidth * 3.0; // needs 3 units
        TestEqual(TEXT("units to span a 3-wide road is 3"), FRoadblock::UnitsToSpan(Width), 3);
        TestFalse(TEXT("2 units leave a gap on a 3-wide road"), FRoadblock::FullyBlocks(Width, 2));
        TestTrue(TEXT("3 units fully block a 3-wide road"), FRoadblock::FullyBlocks(Width, 3));
        TestEqual(TEXT("a degenerate width still needs at least one unit"), FRoadblock::UnitsToSpan(0.0), 1);
    }

    // --- UnitPositions ---
    {
        // A lone cruiser sits at the centre.
        const TArray<FVector> One = FRoadblock::UnitPositions(FVector::ZeroVector, Right, 1000.0, 1);
        TestEqual(TEXT("one unit -> one position"), One.Num(), 1);
        TestTrue(TEXT("the lone unit blocks the centre"), One[0].IsNearlyZero());

        // Three cruisers span edge to edge about the centre.
        const double Width = 1000.0;
        const TArray<FVector> Three = FRoadblock::UnitPositions(FVector::ZeroVector, Right, Width, 3);
        TestEqual(TEXT("three units -> three positions"), Three.Num(), 3);
        TestTrue(TEXT("first unit sits at the left edge"),
            FMath::IsNearlyEqual(Three[0].Y, -0.5 * Width, GtcTest::Eps));
        TestTrue(TEXT("middle unit sits at the centre"),
            FMath::IsNearlyEqual(Three[1].Y, 0.0, GtcTest::Eps));
        TestTrue(TEXT("last unit sits at the right edge"),
            FMath::IsNearlyEqual(Three[2].Y, 0.5 * Width, GtcTest::Eps));

        // Degenerate inputs -> empty.
        TestEqual(TEXT("zero units -> no positions"),
            FRoadblock::UnitPositions(FVector::ZeroVector, Right, Width, 0).Num(), 0);
        TestEqual(TEXT("degenerate road-right -> no positions"),
            FRoadblock::UnitPositions(FVector::ZeroVector, FVector::ZeroVector, Width, 3).Num(), 0);
    }

    // --- HasPassed ---
    {
        const FVector Center(3000, 0, 0);
        TestFalse(TEXT("player before the line has not passed"),
            FRoadblock::HasPassed(FVector(1000, 0, 0), Center, Flee));
        TestTrue(TEXT("player beyond the line has passed"),
            FRoadblock::HasPassed(FVector(4000, 0, 0), Center, Flee));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
