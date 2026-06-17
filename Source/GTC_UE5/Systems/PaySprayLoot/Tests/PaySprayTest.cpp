// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../PaySpray.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to an assertion in the the reference reference behavior
// game/tests/unit/test_pay_spray.gd. Float compares use Eps; ints/strings/bools
// are exact; deny reasons use Reason.Contains(...).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayCanEnterInsideRadiusTest,
    "GTC.Systems.PaySprayLoot.PaySpray.CanEnterInsideRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayCanEnterInsideRadiusTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("inside radius"), PaySpray::CanEnter(FVector(1, 0, 0), FVector(0, 0, 0), 2.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayCanEnterBoundaryTest,
    "GTC.Systems.PaySprayLoot.PaySpray.CanEnterOnRadiusBoundary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayCanEnterBoundaryTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("on boundary"), PaySpray::CanEnter(FVector(3, 0, 0), FVector(0, 0, 0), 3.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayCanEnterOutsideRadiusTest,
    "GTC.Systems.PaySprayLoot.PaySpray.CanEnterOutsideRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayCanEnterOutsideRadiusTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("outside radius"), PaySpray::CanEnter(FVector(5, 0, 0), FVector(0, 0, 0), 2.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayCanEnterNegativeRadiusTest,
    "GTC.Systems.PaySprayLoot.PaySpray.CanEnterNegativeRadiusNever",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayCanEnterNegativeRadiusTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("negative radius never"), PaySpray::CanEnter(FVector(0.1f, 0, 0), FVector(0, 0, 0), -5.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayCostScalesTest,
    "GTC.Systems.PaySprayLoot.PaySpray.CostScalesWithStars",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayCostScalesTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("1 star == 300"), PaySpray::CostFor(1, 200, 100), 300);
    TestEqual(TEXT("3 star == 500"), PaySpray::CostFor(3, 200, 100), 500);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayCostZeroStarsTest,
    "GTC.Systems.PaySprayLoot.PaySpray.CostZeroAtZeroStars",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayCostZeroStarsTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("0 star == 0"), PaySpray::CostFor(0, 200, 100), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayCostClampsStarsTest,
    "GTC.Systems.PaySprayLoot.PaySpray.CostClampsStarsToFive",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayCostClampsStarsTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("99 stars clamp to 5 -> 700"), PaySpray::CostFor(99, 200, 100), 700);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayCostNeverNegativeTest,
    "GTC.Systems.PaySprayLoot.PaySpray.CostNeverNegative",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayCostNeverNegativeTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("negative base/per_star floored -> 0"), PaySpray::CostFor(2, -1000, -50), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySpraySeenTrueNearCopTest,
    "GTC.Systems.PaySprayLoot.PaySpray.SeenEnteringTrueWithNearCop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySpraySeenTrueNearCopTest::RunTest(const FString& Parameters)
{
    const TArray<FVector> Police{ FVector(3, 0, 0) };
    TestTrue(TEXT("near cop sees"), PaySpray::IsSeenEntering(FVector(0, 0, 0), Police, 5.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySpraySeenFalseWhenClearTest,
    "GTC.Systems.PaySprayLoot.PaySpray.SeenEnteringFalseWhenClear",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySpraySeenFalseWhenClearTest::RunTest(const FString& Parameters)
{
    const TArray<FVector> Police{ FVector(20, 0, 0) };
    TestFalse(TEXT("far cop clear"), PaySpray::IsSeenEntering(FVector(0, 0, 0), Police, 5.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySpraySeenFalseEmptyTest,
    "GTC.Systems.PaySprayLoot.PaySpray.SeenEnteringFalseEmptyPolice",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySpraySeenFalseEmptyTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("empty police clear"), PaySpray::IsSeenEntering(FVector(0, 0, 0), TArray<FVector>(), 5.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySpraySeenIgnoresMalformedTest,
    "GTC.Systems.PaySprayLoot.PaySpray.SeenEnteringIgnoresMalformed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySpraySeenIgnoresMalformedTest::RunTest(const FString& Parameters)
{
    // Typed-port gap (APPROVED): the oracle feeds malformed dicts ({"nope":1},
    // {"pos":"bad"}) alongside one far valid cop and expects them ignored. A typed
    // TArray<FVector> cannot hold malformed entries, so this maps to the surviving
    // valid-input case: one far cop -> not seen.
    const TArray<FVector> Police{ FVector(50, 0, 0) };
    TestFalse(TEXT("only valid far cop -> clear"), PaySpray::IsSeenEntering(FVector(0, 0, 0), Police, 5.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayStartsIdleTest,
    "GTC.Systems.PaySprayLoot.PaySpray.ResprayStartsIdle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayStartsIdleTest::RunTest(const FString& Parameters)
{
    PaySpray Spray(3.0f);
    TestEqual(TEXT("progress 0"), (double)Spray.Progress(), 0.0, Eps);
    TestFalse(TEXT("not complete"), Spray.IsComplete());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayTickBeforeBeginTest,
    "GTC.Systems.PaySprayLoot.PaySpray.TickBeforeBeginIsNoop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayTickBeforeBeginTest::RunTest(const FString& Parameters)
{
    PaySpray Spray(3.0f);
    Spray.Tick(10.0f);
    TestEqual(TEXT("progress 0"), (double)Spray.Progress(), 0.0, Eps);
    TestFalse(TEXT("not complete"), Spray.IsComplete());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayRampsTest,
    "GTC.Systems.PaySprayLoot.PaySpray.ResprayRamps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayRampsTest::RunTest(const FString& Parameters)
{
    PaySpray Spray(4.0f);
    Spray.Begin();
    Spray.Tick(1.0f);
    TestEqual(TEXT("progress 0.25"), (double)Spray.Progress(), 0.25, Eps);
    TestFalse(TEXT("not complete"), Spray.IsComplete());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayCompletesTest,
    "GTC.Systems.PaySprayLoot.PaySpray.ResprayCompletes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayCompletesTest::RunTest(const FString& Parameters)
{
    PaySpray Spray(3.0f);
    Spray.Begin();
    Spray.Tick(3.0f);
    TestTrue(TEXT("complete"), Spray.IsComplete());
    TestEqual(TEXT("progress 1"), (double)Spray.Progress(), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayCompletesOnceHoldsTest,
    "GTC.Systems.PaySprayLoot.PaySpray.ResprayCompletesOnceAndHolds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayCompletesOnceHoldsTest::RunTest(const FString& Parameters)
{
    PaySpray Spray(3.0f);
    Spray.Begin();
    Spray.Tick(3.0f);
    Spray.Tick(5.0f);
    TestTrue(TEXT("complete"), Spray.IsComplete());
    TestEqual(TEXT("progress 1"), (double)Spray.Progress(), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayCancelAbortsTest,
    "GTC.Systems.PaySprayLoot.PaySpray.CancelAborts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayCancelAbortsTest::RunTest(const FString& Parameters)
{
    PaySpray Spray(3.0f);
    Spray.Begin();
    Spray.Tick(2.0f);
    Spray.Cancel();
    TestFalse(TEXT("not complete"), Spray.IsComplete());
    TestEqual(TEXT("progress 0"), (double)Spray.Progress(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayResetIdleTest,
    "GTC.Systems.PaySprayLoot.PaySpray.ResetReturnsToIdle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayResetIdleTest::RunTest(const FString& Parameters)
{
    PaySpray Spray(3.0f);
    Spray.Begin();
    Spray.Tick(3.0f);
    Spray.Reset();
    TestFalse(TEXT("not complete"), Spray.IsComplete());
    TestEqual(TEXT("progress 0"), (double)Spray.Progress(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayTickIgnoresNegativeTest,
    "GTC.Systems.PaySprayLoot.PaySpray.TickIgnoresNegativeDelta",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayTickIgnoresNegativeTest::RunTest(const FString& Parameters)
{
    PaySpray Spray(3.0f);
    Spray.Begin();
    Spray.Tick(-5.0f);
    TestEqual(TEXT("progress 0"), (double)Spray.Progress(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayResolveSuccessTest,
    "GTC.Systems.PaySprayLoot.PaySpray.ResolveSuccessDeductsAndAuthorizes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayResolveSuccessTest::RunTest(const FString& Parameters)
{
    PaySpray Spray;
    const PaySpray::FResolveResult Result = Spray.Resolve(2, 1000, false, 200, 100);
    TestTrue(TEXT("allowed"), Result.bAllowed);
    TestEqual(TEXT("cost 400"), Result.Cost, 400);
    TestEqual(TEXT("new_balance 600"), Result.NewBalance, 600);
    TestEqual(TEXT("reason empty"), Result.Reason, FString());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayResolveSeenTest,
    "GTC.Systems.PaySprayLoot.PaySpray.ResolveFailsWhenSeen",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayResolveSeenTest::RunTest(const FString& Parameters)
{
    PaySpray Spray;
    const PaySpray::FResolveResult Result = Spray.Resolve(2, 1000, true, 200, 100);
    TestFalse(TEXT("denied"), Result.bAllowed);
    TestEqual(TEXT("cost 0"), Result.Cost, 0);
    TestEqual(TEXT("new_balance 1000"), Result.NewBalance, 1000);
    TestTrue(TEXT("reason mentions seen"), Result.Reason.Contains(TEXT("seen")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayResolveBrokeTest,
    "GTC.Systems.PaySprayLoot.PaySpray.ResolveFailsWhenBroke",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayResolveBrokeTest::RunTest(const FString& Parameters)
{
    PaySpray Spray;
    const PaySpray::FResolveResult Result = Spray.Resolve(3, 100, false, 200, 100);
    TestFalse(TEXT("denied"), Result.bAllowed);
    TestEqual(TEXT("new_balance 100"), Result.NewBalance, 100);
    TestTrue(TEXT("reason mentions insufficient"), Result.Reason.Contains(TEXT("insufficient")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPaySprayResolveZeroStarsTest,
    "GTC.Systems.PaySprayLoot.PaySpray.ResolveFailsAtZeroStars",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPaySprayResolveZeroStarsTest::RunTest(const FString& Parameters)
{
    PaySpray Spray;
    const PaySpray::FResolveResult Result = Spray.Resolve(0, 1000, false, 200, 100);
    TestFalse(TEXT("denied"), Result.bAllowed);
    TestEqual(TEXT("cost 0"), Result.Cost, 0);
    TestEqual(TEXT("new_balance 1000"), Result.NewBalance, 1000);
    TestTrue(TEXT("reason mentions nothing"), Result.Reason.Contains(TEXT("nothing")));
    return true;
}

#endif // WITH_AUTOMATION_TESTS
