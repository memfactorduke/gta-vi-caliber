// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../TurnChoice.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FTurnChoice — junction lane/turn decisions. GTC-original. Pins the
 * maneuver classification, the signed-turn sign (positive = Left), route
 * following, and the straightest free-roam fallback (no needless U-turn).
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTurnChoiceTest,
    "GTC.AI.Traffic.Turn",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTurnChoiceTest::RunTest(const FString& Parameters)
{
    using ETurn = FTurnChoice::ETurn;
    const FVector X(1, 0, 0), Z(0, 0, 1), NX(-1, 0, 0), NZ(0, 0, -1);

    // Maneuver classification.
    TestTrue(TEXT("same heading is straight"), FTurnChoice::Classify(X, X) == ETurn::Straight);
    TestTrue(TEXT("+X -> +Z is a left turn"), FTurnChoice::Classify(X, Z) == ETurn::Left);
    TestTrue(TEXT("+X -> -Z is a right turn"), FTurnChoice::Classify(X, NZ) == ETurn::Right);
    TestTrue(TEXT("reversal is a U-turn"), FTurnChoice::Classify(X, NX) == ETurn::UTurn);

    // Signed turn sign + magnitude.
    TestTrue(TEXT("left turn is +pi/2"), FMath::Abs(FTurnChoice::SignedTurn(X, Z) - (PI / 2.0)) <= Eps);
    TestTrue(TEXT("right turn is -pi/2"), FMath::Abs(FTurnChoice::SignedTurn(X, NZ) + (PI / 2.0)) <= Eps);

    // Route following.
    TestEqual(TEXT("route picks the lane to the next node"),
        FTurnChoice::ChooseByRoute(TArray<int32>({5, 7, 9}), 7), 1);
    TestEqual(TEXT("route absent -> -1"),
        FTurnChoice::ChooseByRoute(TArray<int32>({5, 7, 9}), 3), -1);

    // Free-roam straightest, U-turn excluded by default.
    {
        const TArray<FVector> Outs({Z, X, NX});
        TestEqual(TEXT("straightest continuation is +X"),
            FTurnChoice::ChooseStraightest(X, Outs, /*bAllowUTurn*/ false), 1);
    }
    TestEqual(TEXT("only a U-turn available -> none"),
        FTurnChoice::ChooseStraightest(X, TArray<FVector>({NX}), false), -1);
    TestEqual(TEXT("U-turn allowed when permitted"),
        FTurnChoice::ChooseStraightest(X, TArray<FVector>({NX}), true), 0);
    TestEqual(TEXT("degenerate incoming -> none"),
        FTurnChoice::ChooseStraightest(FVector::ZeroVector, TArray<FVector>({X}), true), -1);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
