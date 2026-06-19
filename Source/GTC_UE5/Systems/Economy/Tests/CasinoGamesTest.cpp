// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../CasinoGames.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to an assertion in the the reference reference behavior
// game/tests/unit/test_casino_games.gd. Spins use a seeded FRandomStream (seed-
// reproducible WITHIN UE5, not byte-identical to Godot); the oracle only pins
// determinism and range, which FRandomStream satisfies. Blackjack cards are passed as
// FString tokens ("10","A","K") since the reference cards are int-or-string.

// --- Roulette ---

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasRouletteStraightTest,
    "GTC.Systems.Economy.CasinoGames.RouletteStraightPays35To1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasRouletteStraightTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("360"), CasinoGames::RoulettePayout(TEXT("straight"), 17, 10), 360);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasRouletteRedTest,
    "GTC.Systems.Economy.CasinoGames.RouletteRedWinAndLoss",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasRouletteRedTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("red 1 win"), CasinoGames::RoulettePayout(TEXT("red"), 1, 50), 100);
    TestEqual(TEXT("red 2 loss"), CasinoGames::RoulettePayout(TEXT("red"), 2, 50), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasRouletteParityTest,
    "GTC.Systems.Economy.CasinoGames.RouletteEvenAndOdd",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasRouletteParityTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("even 4"), CasinoGames::RoulettePayout(TEXT("even"), 4, 10), 20);
    TestEqual(TEXT("even 5"), CasinoGames::RoulettePayout(TEXT("even"), 5, 10), 0);
    TestEqual(TEXT("odd 5"), CasinoGames::RoulettePayout(TEXT("odd"), 5, 10), 20);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasRouletteDozenTest,
    "GTC.Systems.Economy.CasinoGames.RouletteDozenPays2To1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasRouletteDozenTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("dozen2 13"), CasinoGames::RoulettePayout(TEXT("dozen2"), 13, 10), 30);
    TestEqual(TEXT("dozen2 5"), CasinoGames::RoulettePayout(TEXT("dozen2"), 5, 10), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasRouletteZeroTest,
    "GTC.Systems.Economy.CasinoGames.RouletteZeroLosesOutsideBets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasRouletteZeroTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("even 0"), CasinoGames::RoulettePayout(TEXT("even"), 0, 10), 0);
    TestEqual(TEXT("red 0"), CasinoGames::RoulettePayout(TEXT("red"), 0, 10), 0);
    TestEqual(TEXT("dozen1 0"), CasinoGames::RoulettePayout(TEXT("dozen1"), 0, 10), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasRouletteSpinTest,
    "GTC.Systems.Economy.CasinoGames.RouletteSpinInRangeAndReproducible",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasRouletteSpinTest::RunTest(const FString& Parameters)
{
    FRandomStream Rng = CasinoGames::MakeRng(2024);
    for (int32 I = 0; I < 200; ++I)
    {
        const int32 N = CasinoGames::RouletteSpin(Rng);
        TestTrue(TEXT("0..36"), N >= 0 && N <= 36);
    }
    FRandomStream RngA = CasinoGames::MakeRng(99);
    FRandomStream RngB = CasinoGames::MakeRng(99);
    TestEqual(TEXT("reproducible"), CasinoGames::RouletteSpin(RngA), CasinoGames::RouletteSpin(RngB));
    return true;
}

// --- Slots ---

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasSlotTripleTest,
    "GTC.Systems.Economy.CasinoGames.SlotThreeOfAKindPaysBySymbol",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasSlotTripleTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("3 sevens"), CasinoGames::SlotPayout({ TEXT("seven"), TEXT("seven"), TEXT("seven") }, 10), 1000);
    TestEqual(TEXT("3 cherries"), CasinoGames::SlotPayout({ TEXT("cherry"), TEXT("cherry"), TEXT("cherry") }, 10), 50);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasSlotPairTest,
    "GTC.Systems.Economy.CasinoGames.SlotTwoOfAKindPartial",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasSlotPairTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("pair 2x"), CasinoGames::SlotPayout({ TEXT("bell"), TEXT("bell"), TEXT("seven") }, 10), 20);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasSlotNoMatchTest,
    "GTC.Systems.Economy.CasinoGames.SlotNoMatchReturnsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasSlotNoMatchTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("no match"), CasinoGames::SlotPayout({ TEXT("cherry"), TEXT("bell"), TEXT("seven") }, 10), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasSlotSpinTest,
    "GTC.Systems.Economy.CasinoGames.SlotSpinCountSymbolsReproducible",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasSlotSpinTest::RunTest(const FString& Parameters)
{
    FRandomStream Rng = CasinoGames::MakeRng(7);
    const TArray<FString> Result = CasinoGames::SlotSpin(Rng, 3);
    TestEqual(TEXT("3 reels"), Result.Num(), 3);
    for (const FString& Symbol : Result)
    {
        TestTrue(TEXT("valid symbol"), CasinoGames::SlotSymbols().Contains(Symbol));
    }
    FRandomStream RngA = CasinoGames::MakeRng(55);
    FRandomStream RngB = CasinoGames::MakeRng(55);
    const TArray<FString> SpinA = CasinoGames::SlotSpin(RngA, 3);
    const TArray<FString> SpinB = CasinoGames::SlotSpin(RngB, 3);
    TestTrue(TEXT("reproducible"), SpinA == SpinB);
    return true;
}

// --- Blackjack ---

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasHandHardSoftTest,
    "GTC.Systems.Economy.CasinoGames.HandValueHardAndSoftAce",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasHandHardSoftTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("hard 17"), CasinoGames::HandValue({ TEXT("10"), TEXT("7") }), 17);
    TestEqual(TEXT("soft 17"), CasinoGames::HandValue({ TEXT("A"), TEXT("6") }), 17);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasHandDemoteTest,
    "GTC.Systems.Economy.CasinoGames.HandValueAcesDemoteToAvoidBust",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasHandDemoteTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("A 6 10 == 17"), CasinoGames::HandValue({ TEXT("A"), TEXT("6"), TEXT("10") }), 17);
    TestEqual(TEXT("A A 9 == 21"), CasinoGames::HandValue({ TEXT("A"), TEXT("A"), TEXT("9") }), 21);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasHandFaceTest,
    "GTC.Systems.Economy.CasinoGames.HandValueFaceCardsAreTen",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasHandFaceTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("K Q == 20"), CasinoGames::HandValue({ TEXT("K"), TEXT("Q") }), 20);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasIsBlackjackTest,
    "GTC.Systems.Economy.CasinoGames.IsBlackjack",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasIsBlackjackTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("A K"), CasinoGames::IsBlackjack({ TEXT("A"), TEXT("K") }));
    TestFalse(TEXT("7 7 7"), CasinoGames::IsBlackjack({ TEXT("7"), TEXT("7"), TEXT("7") }));
    TestFalse(TEXT("A 5 5"), CasinoGames::IsBlackjack({ TEXT("A"), TEXT("5"), TEXT("5") }));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasBustHitTest,
    "GTC.Systems.Economy.CasinoGames.IsBustAndDealerShouldHit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasBustHitTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("22 bust"), CasinoGames::IsBust(22));
    TestFalse(TEXT("21 not bust"), CasinoGames::IsBust(21));
    TestTrue(TEXT("hit 16"), CasinoGames::DealerShouldHit(16));
    TestFalse(TEXT("stand 17"), CasinoGames::DealerShouldHit(17));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasSettleWinNaturalTest,
    "GTC.Systems.Economy.CasinoGames.BlackjackSettleWinAndNatural",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasSettleWinNaturalTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("20 beats 18"), CasinoGames::BlackjackSettle(20, 18, 100), 200);
    TestEqual(TEXT("natural 21"), CasinoGames::BlackjackSettle(21, 20, 100), 250);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasMulticard21Test,
    "GTC.Systems.Economy.CasinoGames.BlackjackMulticard21IsNotNatural",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasMulticard21Test::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("3-card 21 == 200"), CasinoGames::BlackjackSettle(21, 20, 100, 3), 200);
    TestEqual(TEXT("2-card 21 == 250"), CasinoGames::BlackjackSettle(21, 20, 100, 2), 250);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasSettlePushTest,
    "GTC.Systems.Economy.CasinoGames.BlackjackSettlePush",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasSettlePushTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("push 100"), CasinoGames::BlackjackSettle(18, 18, 100), 100);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasSettleLossTest,
    "GTC.Systems.Economy.CasinoGames.BlackjackSettleLossAndBust",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasSettleLossTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("17 vs 20 loss"), CasinoGames::BlackjackSettle(17, 20, 100), 0);
    TestEqual(TEXT("23 bust"), CasinoGames::BlackjackSettle(23, 18, 100), 0);
    return true;
}

// --- Bankroll ---

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasBankrollStartTest,
    "GTC.Systems.Economy.CasinoGames.BankrollStartsWithChips",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasBankrollStartTest::RunTest(const FString& Parameters)
{
    CasinoGames Bank(500);
    TestEqual(TEXT("chips 500"), Bank.Chips(), 500);
    TestFalse(TEXT("not broke"), Bank.IsBroke());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasPlaceBetOverTest,
    "GTC.Systems.Economy.CasinoGames.PlaceBetRejectsOverChips",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasPlaceBetOverTest::RunTest(const FString& Parameters)
{
    CasinoGames Bank(100);
    TestFalse(TEXT("reject 150"), Bank.PlaceBet(150));
    TestEqual(TEXT("chips 100"), Bank.Chips(), 100);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasPlaceBetWinTest,
    "GTC.Systems.Economy.CasinoGames.PlaceBetDeductsAndWinAdds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasPlaceBetWinTest::RunTest(const FString& Parameters)
{
    CasinoGames Bank(100);
    TestTrue(TEXT("bet 40"), Bank.PlaceBet(40));
    TestEqual(TEXT("chips 60"), Bank.Chips(), 60);
    Bank.Win(80);
    TestEqual(TEXT("chips 140"), Bank.Chips(), 140);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasBrokeTest,
    "GTC.Systems.Economy.CasinoGames.IsBrokeAfterLosingAll",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasBrokeTest::RunTest(const FString& Parameters)
{
    CasinoGames Bank(50);
    Bank.PlaceBet(50);
    TestTrue(TEXT("broke"), Bank.IsBroke());
    TestEqual(TEXT("chips 0"), Bank.Chips(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasResetTest,
    "GTC.Systems.Economy.CasinoGames.ResetRestoresStartingChips",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasResetTest::RunTest(const FString& Parameters)
{
    CasinoGames Bank(200);
    Bank.PlaceBet(120);
    Bank.Reset();
    TestEqual(TEXT("chips 200"), Bank.Chips(), 200);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCasHouseEdgeTest,
    "GTC.Systems.Economy.CasinoGames.HouseEdgeDocumented",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCasHouseEdgeTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("roulette 0.027"), CasinoGames::HouseEdge(TEXT("roulette")), 0.027, Eps);
    TestEqual(TEXT("unknown 0"), CasinoGames::HouseEdge(TEXT("unknown")), 0.0, Eps);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
