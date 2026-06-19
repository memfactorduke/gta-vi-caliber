// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../ArrestModel.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to an assertion in the the reference reference behavior
// game/tests/unit/test_arrest_model.gd (15 test_* funcs). Literals/tolerances are
// identical to the oracle; the two compound oracle funcs (busted_at_grapple_time,
// bust_fee_complements_kept_cash) are split into their constituent assertions.
// Float grapple math uses Eps (oracle uses is_equal_approx); int cash assertions are exact.

// --- cornered -------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestCorneredWhenWantedAndCloseTest,
    "GTC.Systems.Arrest.ArrestModel.CorneredWhenWantedAndClose",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestCorneredWhenWantedAndCloseTest::RunTest(const FString& Parameters)
{
    // Oracle: ArrestModel.cornered(2, 1.5, 1.8) -> true.
    TestTrue(TEXT("cornered(2, 1.5, 1.8)"), FArrestModel::Cornered(2, 1.5, 1.8));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestNotCorneredWhenOutOfReachTest,
    "GTC.Systems.Arrest.ArrestModel.NotCorneredWhenOutOfReach",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestNotCorneredWhenOutOfReachTest::RunTest(const FString& Parameters)
{
    // Oracle: not ArrestModel.cornered(2, 5.0, 1.8).
    TestFalse(TEXT("cornered(2, 5.0, 1.8)"), FArrestModel::Cornered(2, 5.0, 1.8));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestNotCorneredWhenNotWantedTest,
    "GTC.Systems.Arrest.ArrestModel.NotCorneredWhenNotWanted",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestNotCorneredWhenNotWantedTest::RunTest(const FString& Parameters)
{
    // Oracle: not ArrestModel.cornered(0, 0.5, 1.8) — no heat, no arrest.
    TestFalse(TEXT("cornered(0, 0.5, 1.8)"), FArrestModel::Cornered(0, 0.5, 1.8));
    return true;
}

// --- tick_grapple ---------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestGrappleBuildsWhileCorneredTest,
    "GTC.Systems.Arrest.ArrestModel.GrappleBuildsWhileCornered",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestGrappleBuildsWhileCorneredTest::RunTest(const FString& Parameters)
{
    // Oracle: is_equal_approx(tick_grapple(0.5, true, 0.25), 0.75).
    TestEqual(TEXT("tick_grapple(0.5, true, 0.25)"), FArrestModel::TickGrapple(0.5, true, 0.25), 0.75, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestGrappleDecaysWhenFreeTest,
    "GTC.Systems.Arrest.ArrestModel.GrappleDecaysWhenFree",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestGrappleDecaysWhenFreeTest::RunTest(const FString& Parameters)
{
    // Oracle: is_equal_approx(tick_grapple(0.5, false, 0.2), 0.3).
    TestEqual(TEXT("tick_grapple(0.5, false, 0.2)"), FArrestModel::TickGrapple(0.5, false, 0.2), 0.3, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestGrappleNeverNegativeTest,
    "GTC.Systems.Arrest.ArrestModel.GrappleNeverNegative",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestGrappleNeverNegativeTest::RunTest(const FString& Parameters)
{
    // Oracle: is_equal_approx(tick_grapple(0.1, false, 0.5), 0.0).
    TestEqual(TEXT("tick_grapple(0.1, false, 0.5)"), FArrestModel::TickGrapple(0.1, false, 0.5), 0.0, Eps);
    return true;
}

// --- is_busted ------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestBustedAtGrappleTimeTest,
    "GTC.Systems.Arrest.ArrestModel.BustedAtGrappleTime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestBustedAtGrappleTimeTest::RunTest(const FString& Parameters)
{
    // Oracle (compound): is_busted(1.5, 1.5) and is_busted(2.0, 1.5).
    TestTrue(TEXT("is_busted(1.5, 1.5)"), FArrestModel::IsBusted(1.5, 1.5));
    TestTrue(TEXT("is_busted(2.0, 1.5)"), FArrestModel::IsBusted(2.0, 1.5));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestNotBustedBeforeGrappleTimeTest,
    "GTC.Systems.Arrest.ArrestModel.NotBustedBeforeGrappleTime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestNotBustedBeforeGrappleTimeTest::RunTest(const FString& Parameters)
{
    // Oracle: not is_busted(1.0, 1.5).
    TestFalse(TEXT("is_busted(1.0, 1.5)"), FArrestModel::IsBusted(1.0, 1.5));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestZeroGrappleTimeNeverBustsTest,
    "GTC.Systems.Arrest.ArrestModel.ZeroGrappleTimeNeverBusts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestZeroGrappleTimeNeverBustsTest::RunTest(const FString& Parameters)
{
    // Oracle: not is_busted(99.0, 0.0).
    TestFalse(TEXT("is_busted(99.0, 0.0)"), FArrestModel::IsBusted(99.0, 0.0));
    return true;
}

// --- cash penalty ---------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestCashAfterBustTakesFractionTest,
    "GTC.Systems.Arrest.ArrestModel.CashAfterBustTakesFraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestCashAfterBustTakesFractionTest::RunTest(const FString& Parameters)
{
    // Oracle: cash_after_bust(1000, 0.10) == 900.
    TestEqual(TEXT("cash_after_bust(1000, 0.10)"), FArrestModel::CashAfterBust(1000, 0.10), 900);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestCashAfterBustFloorsTest,
    "GTC.Systems.Arrest.ArrestModel.CashAfterBustFloors",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestCashAfterBustFloorsTest::RunTest(const FString& Parameters)
{
    // Oracle: cash_after_bust(333, 0.10) == 299 (333 * 0.9 = 299.7 -> floored to 299).
    TestEqual(TEXT("cash_after_bust(333, 0.10)"), FArrestModel::CashAfterBust(333, 0.10), 299);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestFullPenaltyEmptiesWalletTest,
    "GTC.Systems.Arrest.ArrestModel.FullPenaltyEmptiesWallet",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestFullPenaltyEmptiesWalletTest::RunTest(const FString& Parameters)
{
    // Oracle: cash_after_bust(1000, 1.0) == 0.
    TestEqual(TEXT("cash_after_bust(1000, 1.0)"), FArrestModel::CashAfterBust(1000, 1.0), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestZeroPenaltyKeepsWalletTest,
    "GTC.Systems.Arrest.ArrestModel.ZeroPenaltyKeepsWallet",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestZeroPenaltyKeepsWalletTest::RunTest(const FString& Parameters)
{
    // Oracle: cash_after_bust(1000, 0.0) == 1000.
    TestEqual(TEXT("cash_after_bust(1000, 0.0)"), FArrestModel::CashAfterBust(1000, 0.0), 1000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestBustFeeComplementsKeptCashTest,
    "GTC.Systems.Arrest.ArrestModel.BustFeeComplementsKeptCash",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestBustFeeComplementsKeptCashTest::RunTest(const FString& Parameters)
{
    // Oracle (compound): wallet=1000; fee = bust_fee(1000, 0.10);
    //   fee == 100 and fee + cash_after_bust(1000, 0.10) == wallet.
    const int32 Wallet = 1000;
    const int32 Fee = FArrestModel::BustFee(Wallet, 0.10);
    TestEqual(TEXT("bust_fee(1000, 0.10) == 100"), Fee, 100);
    TestEqual(TEXT("fee + cash_after_bust == wallet"), Fee + FArrestModel::CashAfterBust(Wallet, 0.10), Wallet);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestBustFeeNeverNegativeTest,
    "GTC.Systems.Arrest.ArrestModel.BustFeeNeverNegative",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestBustFeeNeverNegativeTest::RunTest(const FString& Parameters)
{
    // Oracle: bust_fee(0, 0.10) == 0.
    TestEqual(TEXT("bust_fee(0, 0.10)"), FArrestModel::BustFee(0, 0.10), 0);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
