// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../StockMarket.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_stock_market.gd. Prices use a neutral multiplier (1.0) or
// relative-direction compares so they stay deterministic and engine-correct.

namespace
{
    StockMarket MakeStable()
    {
        return StockMarket({ StockMarket::FCompanyDef(TEXT("rock_corp"), TEXT("utility"), 100, 0.0) });
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockDefaultRosterTest,
    "GTC.Systems.Economy.StockMarket.DefaultRosterLoaded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockDefaultRosterTest::RunTest(const FString& Parameters)
{
    StockMarket M;
    TestEqual(TEXT("count == 7"), M.CompanyCount(), 7);
    TestTrue(TEXT("has augury_air"), M.HasCompany(TEXT("augury_air")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockDefaultSectorsTest,
    "GTC.Systems.Economy.StockMarket.DefaultSectors",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockDefaultSectorsTest::RunTest(const FString& Parameters)
{
    StockMarket M;
    TestEqual(TEXT("5 sectors"), M.Sectors().Num(), 5);
    TestTrue(TEXT("has aviation"), M.Sectors().Contains(TEXT("aviation")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockKnownLookupsTest,
    "GTC.Systems.Economy.StockMarket.KnownLookups",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockKnownLookupsTest::RunTest(const FString& Parameters)
{
    StockMarket M;
    TestEqual(TEXT("base 42"), M.BasePrice(TEXT("augury_air")), 42);
    TestEqual(TEXT("price 42"), M.Price(TEXT("augury_air")), 42);
    TestEqual(TEXT("sector"), M.SectorOf(TEXT("augury_air")), FString(TEXT("aviation")));
    TestEqual(TEXT("volatility 0.7"), M.Volatility(TEXT("augury_air")), 0.7, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockUnknownLookupsTest,
    "GTC.Systems.Economy.StockMarket.UnknownLookupsAreNeutral",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockUnknownLookupsTest::RunTest(const FString& Parameters)
{
    StockMarket M;
    TestEqual(TEXT("base -1"), M.BasePrice(TEXT("nope")), -1);
    TestEqual(TEXT("price -1"), M.Price(TEXT("nope")), -1);
    TestEqual(TEXT("sector empty"), M.SectorOf(TEXT("nope")), FString());
    TestEqual(TEXT("volatility 0"), M.Volatility(TEXT("nope")), 0.0, Eps);
    TestEqual(TEXT("multiplier 1.0"), M.Multiplier(TEXT("nope")), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockMalformedDroppedTest,
    "GTC.Systems.Economy.StockMarket.MalformedCompaniesDropped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockMalformedDroppedTest::RunTest(const FString& Parameters)
{
    // Typed-port note: Godot's "no base_price" row is unrepresentable with a typed
    // FCompanyDef; the negative-price and duplicate-id rows that ARE representable
    // cover the drop path 1:1.
    StockMarket M({
        StockMarket::FCompanyDef(TEXT("ok"), TEXT("x"), 50, 0.5),
        StockMarket::FCompanyDef(TEXT("neg"), TEXT(""), -5, 0.5),
        StockMarket::FCompanyDef(TEXT("ok"), TEXT(""), 999, 0.5),
    });
    TestEqual(TEXT("count == 1"), M.CompanyCount(), 1);
    TestTrue(TEXT("has ok"), M.HasCompany(TEXT("ok")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockVolatilityClampedTest,
    "GTC.Systems.Economy.StockMarket.VolatilityClampedOnRegister",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockVolatilityClampedTest::RunTest(const FString& Parameters)
{
    StockMarket M({ StockMarket::FCompanyDef(TEXT("wild"), TEXT("misc"), 10, 9.0) });
    TestEqual(TEXT("volatility 1.0"), M.Volatility(TEXT("wild")), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockCompanyEventBothTest,
    "GTC.Systems.Economy.StockMarket.CompanyEventMovesPriceBothWays",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockCompanyEventBothTest::RunTest(const FString& Parameters)
{
    StockMarket Up;
    Up.ApplyCompanyEvent(TEXT("augury_air"), 1.0);
    StockMarket Down;
    Down.ApplyCompanyEvent(TEXT("augury_air"), -0.5);
    TestEqual(TEXT("up == 71"), Up.Price(TEXT("augury_air")), 71);
    TestEqual(TEXT("down == 27"), Down.Price(TEXT("augury_air")), 27);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockEventUnknownTest,
    "GTC.Systems.Economy.StockMarket.EventUnknownCompanyIsFalse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockEventUnknownTest::RunTest(const FString& Parameters)
{
    StockMarket M;
    TestFalse(TEXT("false"), M.ApplyCompanyEvent(TEXT("nope"), 1.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockVolatilityScalesTest,
    "GTC.Systems.Economy.StockMarket.VolatilityScalesReaction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockVolatilityScalesTest::RunTest(const FString& Parameters)
{
    StockMarket M({
        StockMarket::FCompanyDef(TEXT("jumpy"), TEXT("misc"), 100, 1.0),
        StockMarket::FCompanyDef(TEXT("calm"), TEXT("misc"), 100, 0.2),
    });
    M.ApplyCompanyEvent(TEXT("jumpy"), 0.5);
    M.ApplyCompanyEvent(TEXT("calm"), 0.5);
    TestTrue(TEXT("jumpy > calm"), M.Price(TEXT("jumpy")) > M.Price(TEXT("calm")));
    TestTrue(TEXT("calm > 100"), M.Price(TEXT("calm")) > 100);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockZeroVolatilityTest,
    "GTC.Systems.Economy.StockMarket.ZeroVolatilityIgnoresEvents",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockZeroVolatilityTest::RunTest(const FString& Parameters)
{
    StockMarket M = MakeStable();
    M.ApplyCompanyEvent(TEXT("rock_corp"), 5.0);
    TestEqual(TEXT("price 100"), M.Price(TEXT("rock_corp")), 100);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockSectorEventTest,
    "GTC.Systems.Economy.StockMarket.SectorEventCountsMoved",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockSectorEventTest::RunTest(const FString& Parameters)
{
    StockMarket M;
    const int32 Moved = M.ApplySectorEvent(TEXT("aviation"), 0.1);
    TestEqual(TEXT("moved == 2"), Moved, 2);
    TestEqual(TEXT("bittn_tech holds"), M.Price(TEXT("bittn_tech")), 120);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockRivalryTest,
    "GTC.Systems.Economy.StockMarket.RivalryShockTargetDownRivalsUp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockRivalryTest::RunTest(const FString& Parameters)
{
    StockMarket M;
    const int32 BasePelican = M.Price(TEXT("pelican_air"));
    const bool bOk = M.ApplyRivalryShock(TEXT("augury_air"), -0.5, 1.0);
    TestTrue(TEXT("ok"), bOk);
    TestTrue(TEXT("augury < 42"), M.Price(TEXT("augury_air")) < 42);
    TestTrue(TEXT("pelican rose"), M.Price(TEXT("pelican_air")) > BasePelican);
    TestEqual(TEXT("cluckin holds"), M.Price(TEXT("cluckin_co")), 24);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockRivalryUnknownTest,
    "GTC.Systems.Economy.StockMarket.RivalryShockUnknownIsFalse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockRivalryUnknownTest::RunTest(const FString& Parameters)
{
    StockMarket M;
    TestFalse(TEXT("false"), M.ApplyRivalryShock(TEXT("nope"), 0.5, 1.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockMultiplierClampTest,
    "GTC.Systems.Economy.StockMarket.MultiplierClampsBothEnds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockMultiplierClampTest::RunTest(const FString& Parameters)
{
    StockMarket High;
    High.ApplyCompanyEvent(TEXT("augury_air"), 100.0);
    StockMarket Low;
    Low.ApplyCompanyEvent(TEXT("augury_air"), -100.0);
    TestEqual(TEXT("high == 336"), High.Price(TEXT("augury_air")), 336);
    TestEqual(TEXT("low == 4"), Low.Price(TEXT("augury_air")), 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockBuySuccessTest,
    "GTC.Systems.Economy.StockMarket.BuySuccessDeductsAndHolds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockBuySuccessTest::RunTest(const FString& Parameters)
{
    StockMarket M;
    const StockMarket::FBuyResult R = M.Buy(TEXT("augury_air"), 10, 1000);
    TestTrue(TEXT("success"), R.bSuccess);
    TestEqual(TEXT("cost 420"), R.Cost, 420);
    TestEqual(TEXT("new_balance 580"), R.NewBalance, 580);
    TestEqual(TEXT("shares 10"), M.SharesHeld(TEXT("augury_air")), 10);
    TestEqual(TEXT("avg_cost 42"), M.AvgCost(TEXT("augury_air")), 42.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockBuyInsufficientTest,
    "GTC.Systems.Economy.StockMarket.BuyInsufficientFunds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockBuyInsufficientTest::RunTest(const FString& Parameters)
{
    StockMarket M;
    const StockMarket::FBuyResult R = M.Buy(TEXT("augury_air"), 10, 100);
    TestFalse(TEXT("not success"), R.bSuccess);
    TestEqual(TEXT("balance 100"), R.NewBalance, 100);
    TestEqual(TEXT("shares 0"), M.SharesHeld(TEXT("augury_air")), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockBuyBadInputTest,
    "GTC.Systems.Economy.StockMarket.BuyRejectsBadInput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockBuyBadInputTest::RunTest(const FString& Parameters)
{
    StockMarket M;
    TestFalse(TEXT("unknown"), M.Buy(TEXT("nope"), 1, 1000).bSuccess);
    TestFalse(TEXT("zero qty"), M.Buy(TEXT("augury_air"), 0, 1000).bSuccess);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockWeightedAvgTest,
    "GTC.Systems.Economy.StockMarket.BuyWeightedAverageCost",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockWeightedAvgTest::RunTest(const FString& Parameters)
{
    StockMarket M;
    M.Buy(TEXT("augury_air"), 10, 5000);
    M.ApplyCompanyEvent(TEXT("augury_air"), 1.0);
    M.Buy(TEXT("augury_air"), 10, 5000);
    TestEqual(TEXT("shares 20"), M.SharesHeld(TEXT("augury_air")), 20);
    TestEqual(TEXT("avg 56.5"), M.AvgCost(TEXT("augury_air")), 56.5, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockSellProfitTest,
    "GTC.Systems.Economy.StockMarket.SellRealizesProfit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockSellProfitTest::RunTest(const FString& Parameters)
{
    StockMarket M;
    M.Buy(TEXT("augury_air"), 10, 5000);
    M.ApplyCompanyEvent(TEXT("augury_air"), 1.0);
    const StockMarket::FSellResult R = M.Sell(TEXT("augury_air"), 5);
    TestTrue(TEXT("success"), R.bSuccess);
    TestEqual(TEXT("proceeds 355"), R.Proceeds, 355);
    TestEqual(TEXT("realized 145"), R.Realized, 145);
    TestEqual(TEXT("shares 5"), M.SharesHeld(TEXT("augury_air")), 5);
    TestEqual(TEXT("realized_gain 145"), M.RealizedGain(), 145);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockSellTooManyTest,
    "GTC.Systems.Economy.StockMarket.SellMoreThanHeldFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockSellTooManyTest::RunTest(const FString& Parameters)
{
    StockMarket M;
    M.Buy(TEXT("augury_air"), 3, 5000);
    const StockMarket::FSellResult R = M.Sell(TEXT("augury_air"), 4);
    TestFalse(TEXT("not success"), R.bSuccess);
    TestEqual(TEXT("shares 3"), M.SharesHeld(TEXT("augury_air")), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockSellAllErasesTest,
    "GTC.Systems.Economy.StockMarket.SellAllErasesPosition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockSellAllErasesTest::RunTest(const FString& Parameters)
{
    StockMarket M;
    M.Buy(TEXT("augury_air"), 5, 5000);
    M.Sell(TEXT("augury_air"), 5);
    TestEqual(TEXT("shares 0"), M.SharesHeld(TEXT("augury_air")), 0);
    TestEqual(TEXT("avg 0"), M.AvgCost(TEXT("augury_air")), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockPortfolioTest,
    "GTC.Systems.Economy.StockMarket.PortfolioValueAndUnrealized",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockPortfolioTest::RunTest(const FString& Parameters)
{
    StockMarket M;
    M.Buy(TEXT("augury_air"), 10, 5000);
    M.ApplyCompanyEvent(TEXT("augury_air"), 1.0);
    TestEqual(TEXT("value 710"), M.PortfolioValue(), 710);
    TestEqual(TEXT("invested 420"), M.TotalInvested(), 420);
    TestEqual(TEXT("unrealized 290"), M.UnrealizedGain(), 290);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockFluctuateDetTest,
    "GTC.Systems.Economy.StockMarket.FluctuateIsDeterministic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockFluctuateDetTest::RunTest(const FString& Parameters)
{
    // RNG note: FRandomStream is seed-reproducible WITHIN UE5 (not byte-identical to
    // Godot). The oracle only pins determinism, which this satisfies.
    StockMarket A;
    StockMarket B;
    FRandomStream RngA = StockMarket::MakeRng(7);
    FRandomStream RngB = StockMarket::MakeRng(7);
    A.Fluctuate(RngA, 1.0);
    B.Fluctuate(RngB, 1.0);
    TestEqual(TEXT("same price"), A.Price(TEXT("augury_air")), B.Price(TEXT("augury_air")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStockFluctuateNoopTest,
    "GTC.Systems.Economy.StockMarket.FluctuateNoopCases",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStockFluctuateNoopTest::RunTest(const FString& Parameters)
{
    StockMarket NoRng;
    NoRng.FluctuateNoRng(1.0);
    StockMarket Stable = MakeStable();
    FRandomStream Rng = StockMarket::MakeRng(3);
    Stable.Fluctuate(Rng, 1.0);
    TestEqual(TEXT("no_rng holds 42"), NoRng.Price(TEXT("augury_air")), 42);
    TestEqual(TEXT("stable holds 100"), Stable.Price(TEXT("rock_corp")), 100);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
