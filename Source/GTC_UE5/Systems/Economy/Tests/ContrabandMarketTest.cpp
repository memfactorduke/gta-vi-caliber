// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../ContrabandMarket.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_contraband_market.gd. District multipliers come from a stable
// hash we don't hardcode; price assertions are derived from the model's own
// MultiplierFor()/BasePrice (engine-correct, RNG/hash-agnostic).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraDefaultLoadedTest,
    "GTC.Systems.Economy.ContrabandMarket.DefaultGoodsLoaded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraDefaultLoadedTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M;
    TestEqual(TEXT("count == 4"), M.GoodsCount(), 4);
    TestTrue(TEXT("has jewelry"), M.HasGood(TEXT("jewelry")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraBaseKnownTest,
    "GTC.Systems.Economy.ContrabandMarket.BasePriceKnown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraBaseKnownTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M({ ContrabandMarket::FGoodDef(TEXT("product"), 1500) });
    TestEqual(TEXT("product == 1500"), M.BasePrice(TEXT("product")), 1500);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraBaseUnknownTest,
    "GTC.Systems.Economy.ContrabandMarket.BasePriceUnknown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraBaseUnknownTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M;
    TestEqual(TEXT("nuke == -1"), M.BasePrice(TEXT("nuke")), -1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraMalformedDroppedTest,
    "GTC.Systems.Economy.ContrabandMarket.MalformedGoodsDropped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraMalformedDroppedTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M({
        ContrabandMarket::FGoodDef(TEXT("ok"), 200),
        ContrabandMarket::FGoodDef(TEXT(""), 50),
        ContrabandMarket::FGoodDef(TEXT("free"), 0),
        ContrabandMarket::FGoodDef(TEXT("neg"), -10),
    });
    TestEqual(TEXT("count == 1"), M.GoodsCount(), 1);
    TestEqual(TEXT("ok == 200"), M.BasePrice(TEXT("ok")), 200);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraPriceMatchesMultTest,
    "GTC.Systems.Economy.ContrabandMarket.PriceInMatchesMultiplier",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraPriceMatchesMultTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M({ ContrabandMarket::FGoodDef(TEXT("g"), 1000) });
    const int32 Expected = static_cast<int32>(FMath::RoundToDouble(1000.0 * M.MultiplierFor(TEXT("downtown"))));
    TestEqual(TEXT("price == expected"), M.PriceIn(TEXT("g"), TEXT("downtown")), Expected);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraPriceVariesTest,
    "GTC.Systems.Economy.ContrabandMarket.PriceInVariesByDistrict",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraPriceVariesTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M({ ContrabandMarket::FGoodDef(TEXT("g"), 1000) });
    const int32 A = M.PriceIn(TEXT("g"), TEXT("downtown"));
    const int32 B = M.PriceIn(TEXT("g"), TEXT("docks"));
    TestNotEqual(TEXT("downtown != docks"), A, B);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraPriceUnknownGoodTest,
    "GTC.Systems.Economy.ContrabandMarket.PriceInUnknownGood",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraPriceUnknownGoodTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M;
    TestEqual(TEXT("nuke == -1"), M.PriceIn(TEXT("nuke"), TEXT("downtown")), -1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraMultBandTest,
    "GTC.Systems.Economy.ContrabandMarket.MultiplierWithinBand",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraMultBandTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M;
    const double Mult = M.MultiplierFor(TEXT("little_havana"));
    TestTrue(TEXT(">= min"), Mult >= ContrabandMarket::MultiplierMin);
    TestTrue(TEXT("<= max"), Mult <= ContrabandMarket::MultiplierMax);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraBuyDeductsTest,
    "GTC.Systems.Economy.ContrabandMarket.BuyDeductsDistrictPrice",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraBuyDeductsTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M({ ContrabandMarket::FGoodDef(TEXT("g"), 500) });
    const int32 Unit = M.PriceIn(TEXT("g"), TEXT("beach"));
    const ContrabandMarket::FBuyResult Result = M.Buy(TEXT("g"), 3, TEXT("beach"), 100000);
    TestTrue(TEXT("success"), Result.bSuccess);
    TestEqual(TEXT("cost == unit*3"), Result.Cost, Unit * 3);
    TestEqual(TEXT("new_balance"), Result.NewBalance, 100000 - Unit * 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraBuyFailsTest,
    "GTC.Systems.Economy.ContrabandMarket.BuyFailsUnknownZeroAndBroke",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraBuyFailsTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M({ ContrabandMarket::FGoodDef(TEXT("g"), 500) });
    const ContrabandMarket::FBuyResult Unknown = M.Buy(TEXT("nuke"), 1, TEXT("beach"), 100000);
    const ContrabandMarket::FBuyResult Zero = M.Buy(TEXT("g"), 0, TEXT("beach"), 100000);
    const ContrabandMarket::FBuyResult Broke = M.Buy(TEXT("g"), 100, TEXT("beach"), 10);
    TestFalse(TEXT("unknown fails"), Unknown.bSuccess);
    TestEqual(TEXT("unknown balance"), Unknown.NewBalance, 100000);
    TestFalse(TEXT("zero fails"), Zero.bSuccess);
    TestEqual(TEXT("zero cost"), Zero.Cost, 0);
    TestFalse(TEXT("broke fails"), Broke.bSuccess);
    TestEqual(TEXT("broke balance"), Broke.NewBalance, 10);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraSellRevenueTest,
    "GTC.Systems.Economy.ContrabandMarket.SellRevenue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraSellRevenueTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M({ ContrabandMarket::FGoodDef(TEXT("g"), 500) });
    const int32 Unit = M.PriceIn(TEXT("g"), TEXT("docks"));
    TestEqual(TEXT("sell 4 == unit*4"), M.Sell(TEXT("g"), 4, TEXT("docks")), Unit * 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraSellZeroTest,
    "GTC.Systems.Economy.ContrabandMarket.SellUnknownOrZeroIsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraSellZeroTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M;
    TestEqual(TEXT("nuke == 0"), M.Sell(TEXT("nuke"), 5, TEXT("docks")), 0);
    TestEqual(TEXT("jewelry qty 0 == 0"), M.Sell(TEXT("jewelry"), 0, TEXT("docks")), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraBestMarketTest,
    "GTC.Systems.Economy.ContrabandMarket.BestMarketPicksHighest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraBestMarketTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M({ ContrabandMarket::FGoodDef(TEXT("g"), 1000) });
    const TArray<FString> Districts = { TEXT("downtown"), TEXT("beach"), TEXT("docks"), TEXT("little_havana") };
    const FString Winner = M.BestMarket(TEXT("g"), Districts);
    const int32 WinnerPrice = M.PriceIn(TEXT("g"), Winner);
    bool bOk = true;
    for (const FString& D : Districts)
    {
        if (M.PriceIn(TEXT("g"), D) > WinnerPrice)
        {
            bOk = false;
        }
    }
    TestTrue(TEXT("winner is highest"), bOk);
    TestFalse(TEXT("winner not empty"), Winner.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraBestMarketEdgeTest,
    "GTC.Systems.Economy.ContrabandMarket.BestMarketSingleUnknownAndEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraBestMarketEdgeTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M;
    TestEqual(TEXT("single"), M.BestMarket(TEXT("jewelry"), { TEXT("solo") }), FString(TEXT("solo")));
    TestEqual(TEXT("unknown"), M.BestMarket(TEXT("nuke"), { TEXT("a") }), FString());
    TestEqual(TEXT("empty list"), M.BestMarket(TEXT("jewelry"), {}), FString());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraProfitPositiveTest,
    "GTC.Systems.Economy.ContrabandMarket.ProfitPositiveForGoodArbitrage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraProfitPositiveTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M({ ContrabandMarket::FGoodDef(TEXT("g"), 1000) });
    const TArray<FString> Districts = { TEXT("downtown"), TEXT("beach"), TEXT("docks"), TEXT("little_havana") };
    const FString High = M.BestMarket(TEXT("g"), Districts);
    FString Low = High;
    int32 LowPrice = M.PriceIn(TEXT("g"), High);
    for (const FString& D : Districts)
    {
        const int32 P = M.PriceIn(TEXT("g"), D);
        if (P < LowPrice)
        {
            LowPrice = P;
            Low = D;
        }
    }
    TestTrue(TEXT("profit > 0"), M.Profit(TEXT("g"), Low, High, 5) > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraProfitNegativeTest,
    "GTC.Systems.Economy.ContrabandMarket.ProfitNegativeForBadRoute",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraProfitNegativeTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M({ ContrabandMarket::FGoodDef(TEXT("g"), 1000) });
    const TArray<FString> Districts = { TEXT("downtown"), TEXT("beach"), TEXT("docks"), TEXT("little_havana") };
    const FString High = M.BestMarket(TEXT("g"), Districts);
    FString Low = High;
    int32 LowPrice = M.PriceIn(TEXT("g"), High);
    for (const FString& D : Districts)
    {
        const int32 P = M.PriceIn(TEXT("g"), D);
        if (P < LowPrice)
        {
            LowPrice = P;
            Low = D;
        }
    }
    TestTrue(TEXT("profit < 0"), M.Profit(TEXT("g"), High, Low, 5) < 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraProfitZeroTest,
    "GTC.Systems.Economy.ContrabandMarket.ProfitUnknownOrZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraProfitZeroTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M;
    TestEqual(TEXT("unknown == 0"), M.Profit(TEXT("nuke"), TEXT("a"), TEXT("b"), 5), 0);
    TestEqual(TEXT("qty 0 == 0"), M.Profit(TEXT("jewelry"), TEXT("a"), TEXT("b"), 0), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraCarryTest,
    "GTC.Systems.Economy.ContrabandMarket.CarryAccumulatesAndRejectsGarbage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraCarryTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M;
    M.Carry(TEXT("jewelry"), 3);
    M.Carry(TEXT("jewelry"), 2);
    M.Carry(TEXT("jewelry"), -4);
    M.Carry(TEXT("nuke"), 10);
    TestEqual(TEXT("jewelry == 5"), M.Carried(TEXT("jewelry")), 5);
    TestEqual(TEXT("nuke == 0"), M.Carried(TEXT("nuke")), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraDropTest,
    "GTC.Systems.Economy.ContrabandMarket.DropNeverNegative",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraDropTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M;
    M.Carry(TEXT("product"), 2);
    M.Drop(TEXT("product"), 10);
    TestEqual(TEXT("product == 0"), M.Carried(TEXT("product")), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraTotalCarriedTest,
    "GTC.Systems.Economy.ContrabandMarket.TotalCarried",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraTotalCarriedTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M;
    M.Carry(TEXT("jewelry"), 3);
    M.Carry(TEXT("product"), 4);
    TestEqual(TEXT("total == 7"), M.TotalCarried(), 7);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraBustRiskTest,
    "GTC.Systems.Economy.ContrabandMarket.BustRiskRisesWithLoadAndClamps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraBustRiskTest::RunTest(const FString& Parameters)
{
    const double Light = ContrabandMarket::BustRisk(2, 0.1);
    const double Heavy = ContrabandMarket::BustRisk(8, 0.1);
    TestTrue(TEXT("heavy > light"), Heavy > Light);
    TestEqual(TEXT("light == 0.2"), Light, 0.2, Eps);
    TestEqual(TEXT("clamp high == 1.0"), ContrabandMarket::BustRisk(100, 0.5), 1.0, Eps);
    TestEqual(TEXT("clamp low == 0.0"), ContrabandMarket::BustRisk(0, -1.0), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraFluctuateDetTest,
    "GTC.Systems.Economy.ContrabandMarket.FluctuateDeterministicWithSeed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraFluctuateDetTest::RunTest(const FString& Parameters)
{
    // RNG note: FRandomStream is seed-reproducible WITHIN UE5 (not byte-identical to
    // Godot). The oracle only pins determinism, which this satisfies.
    ContrabandMarket A({ ContrabandMarket::FGoodDef(TEXT("g"), 1000) });
    ContrabandMarket B({ ContrabandMarket::FGoodDef(TEXT("g"), 1000) });
    A.PriceIn(TEXT("g"), TEXT("downtown"));
    B.PriceIn(TEXT("g"), TEXT("downtown"));
    FRandomStream RngA = ContrabandMarket::MakeRng(42);
    FRandomStream RngB = ContrabandMarket::MakeRng(42);
    A.Fluctuate(RngA, 1.0);
    B.Fluctuate(RngB, 1.0);
    TestEqual(TEXT("same multiplier"), A.MultiplierFor(TEXT("downtown")), B.MultiplierFor(TEXT("downtown")), Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraFluctuateShiftsTest,
    "GTC.Systems.Economy.ContrabandMarket.FluctuateShiftsPrices",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraFluctuateShiftsTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M({ ContrabandMarket::FGoodDef(TEXT("g"), 1000) });
    const double Before = M.MultiplierFor(TEXT("downtown"));
    FRandomStream Rng = ContrabandMarket::MakeRng(7);
    M.Fluctuate(Rng, 1.0);
    const double After = M.MultiplierFor(TEXT("downtown"));
    TestTrue(TEXT("multiplier shifted"), !FMath::IsNearlyEqual(Before, After, Eps));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContraFluctuateNoRngTest,
    "GTC.Systems.Economy.ContrabandMarket.FluctuateNullRngNoop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContraFluctuateNoRngTest::RunTest(const FString& Parameters)
{
    ContrabandMarket M({ ContrabandMarket::FGoodDef(TEXT("g"), 1000) });
    const double Before = M.MultiplierFor(TEXT("downtown"));
    M.FluctuateNoRng(1.0);
    TestEqual(TEXT("unchanged"), M.MultiplierFor(TEXT("downtown")), Before, Eps);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
