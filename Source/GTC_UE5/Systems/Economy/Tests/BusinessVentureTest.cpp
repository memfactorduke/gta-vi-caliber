// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../BusinessVenture.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_business_venture.gd. coke_lab tuning: 10 product/day, 2.0
// supply per product, 200 max product, $2000 sale, 6 staff, 3 tiers. Float compares
// use Eps.

namespace
{
    FVentureDef MakeMinimal(const FString& Id, double ProductPerDay, int32 SaleValue)
    {
        FVentureDef Def;
        Def.Id = Id;
        Def.ProductPerDay = ProductPerDay;
        Def.SaleValue = SaleValue;
        return Def;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizDefaultLoadedTest,
    "GTC.Systems.Economy.BusinessVenture.DefaultVenturesLoaded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizDefaultLoadedTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    TestTrue(TEXT("count >= 4"), B.VentureCount() >= 4);
    TestTrue(TEXT("has coke_lab"), B.HasVenture(TEXT("coke_lab")));
    TestTrue(TEXT("has nightclub"), B.HasVenture(TEXT("nightclub")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizMalformedDroppedTest,
    "GTC.Systems.Economy.BusinessVenture.MalformedVenturesDropped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizMalformedDroppedTest::RunTest(const FString& Parameters)
{
    BusinessVenture B({
        MakeMinimal(TEXT("ok"), 10.0, 100),
        MakeMinimal(TEXT(""), 10.0, 100),
        MakeMinimal(TEXT("bad"), 0.0, 100),
        MakeMinimal(TEXT("ok"), 7.0, 100),
    });
    TestEqual(TEXT("count == 1"), B.VentureCount(), 1);
    TestTrue(TEXT("has ok"), B.HasVenture(TEXT("ok")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizAcquireTest,
    "GTC.Systems.Economy.BusinessVenture.AcquireDeductsAndMarksOwned",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizAcquireTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    const FVentureMoneyResult R = B.Acquire(TEXT("coke_lab"), 50000, 80000);
    TestTrue(TEXT("success"), R.bSuccess);
    TestEqual(TEXT("new_balance 30000"), R.NewBalance, 30000);
    TestTrue(TEXT("owns"), B.Owns(TEXT("coke_lab")));
    TestEqual(TEXT("cost 50000"), R.Cost, 50000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizAcquireRejectsTest,
    "GTC.Systems.Economy.BusinessVenture.AcquireRejectsUnownedUnknownAndInsufficient",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizAcquireRejectsTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    const FVentureMoneyResult Ghost = B.Acquire(TEXT("ghost"), 1, 9999);
    const FVentureMoneyResult Broke = B.Acquire(TEXT("nightclub"), 999999, 100);
    B.Acquire(TEXT("coke_lab"), 10, 1000);
    const FVentureMoneyResult Dup = B.Acquire(TEXT("coke_lab"), 10, 1000);
    TestFalse(TEXT("ghost fails"), Ghost.bSuccess);
    TestFalse(TEXT("broke fails"), Broke.bSuccess);
    TestEqual(TEXT("broke balance"), Broke.NewBalance, 100);
    TestFalse(TEXT("dup fails"), Dup.bSuccess);
    TestTrue(TEXT("already owned"), Dup.Reason.Contains(TEXT("already owned")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizBuySuppliesClampTest,
    "GTC.Systems.Economy.BusinessVenture.BuySuppliesRequiresOwnershipAndClamps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizBuySuppliesClampTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    const FVentureMoneyResult Unowned = B.BuySupplies(TEXT("coke_lab"), 10, 1, 1000);
    B.Acquire(TEXT("coke_lab"), 0, 1000);
    const FVentureMoneyResult R = B.BuySupplies(TEXT("coke_lab"), 1000000, 1, 10000000);
    TestFalse(TEXT("unowned fails"), Unowned.bSuccess);
    TestTrue(TEXT("success"), R.bSuccess);
    TestEqual(TEXT("supply 400"), B.SupplyIn(TEXT("coke_lab")), 400.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizAccrueConvertsTest,
    "GTC.Systems.Economy.BusinessVenture.AccrueConvertsSupplyToProduct",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizAccrueConvertsTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    B.Acquire(TEXT("coke_lab"), 0, 1000);
    B.BuySupplies(TEXT("coke_lab"), 400, 1, 1000);
    const double Rate = B.ProductionRate(TEXT("coke_lab"));
    B.Accrue(1.0);
    TestEqual(TEXT("product == rate"), B.ProductIn(TEXT("coke_lab")), Rate, Eps);
    TestEqual(TEXT("supply drained"), B.SupplyIn(TEXT("coke_lab")), 400.0 - Rate * 2.0, Eps);
    B.Accrue(0.0);
    B.Accrue(-3.0);
    TestEqual(TEXT("noop product"), B.ProductIn(TEXT("coke_lab")), Rate, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizProductionZeroTest,
    "GTC.Systems.Economy.BusinessVenture.ProductionZeroWithoutSupply",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizProductionZeroTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    B.Acquire(TEXT("coke_lab"), 0, 1000);
    const double Rate0 = B.ProductionRate(TEXT("coke_lab"));
    B.Accrue(5.0);
    TestEqual(TEXT("rate0 == 0"), Rate0, 0.0, Eps);
    TestEqual(TEXT("product == 0"), B.ProductIn(TEXT("coke_lab")), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizAccrueCapsTest,
    "GTC.Systems.Economy.BusinessVenture.AccrueCapsAtMaxProduct",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizAccrueCapsTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    B.Acquire(TEXT("coke_lab"), 0, 1000);
    B.BuySupplies(TEXT("coke_lab"), 400, 1, 1000);
    B.Accrue(9999.0);
    TestEqual(TEXT("product == 200"), B.ProductIn(TEXT("coke_lab")), 200.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizHireBoundsTest,
    "GTC.Systems.Economy.BusinessVenture.HireRaisesRateAndRespectsBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizHireBoundsTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    B.Acquire(TEXT("coke_lab"), 0, 1000);
    B.BuySupplies(TEXT("coke_lab"), 400, 1, 1000);
    const double Rate0 = B.ProductionRate(TEXT("coke_lab"));
    const bool Hired = B.Hire(TEXT("coke_lab"));
    const double Rate1 = B.ProductionRate(TEXT("coke_lab"));
    for (int32 I = 0; I < 5; ++I)
    {
        B.Hire(TEXT("coke_lab"));
    }
    const int32 AtMax = B.StaffIn(TEXT("coke_lab"));
    const bool Over = B.Hire(TEXT("coke_lab"));
    for (int32 J = 0; J < 6; ++J)
    {
        B.Fire(TEXT("coke_lab"));
    }
    const bool Under = B.Fire(TEXT("coke_lab"));
    TestTrue(TEXT("hired"), Hired);
    TestTrue(TEXT("rate1 > rate0"), Rate1 > Rate0);
    TestEqual(TEXT("at max 6"), AtMax, 6);
    TestFalse(TEXT("over false"), Over);
    TestFalse(TEXT("under false"), Under);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizUpgradeRateTest,
    "GTC.Systems.Economy.BusinessVenture.UpgradeRaisesTierAndRate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizUpgradeRateTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    B.Acquire(TEXT("coke_lab"), 0, 100000);
    B.BuySupplies(TEXT("coke_lab"), 400, 1, 1000);
    const double Rate0 = B.ProductionRate(TEXT("coke_lab"));
    const FVentureMoneyResult R = B.Upgrade(TEXT("coke_lab"), 20000, 100000);
    const int32 Tier1 = B.TierIn(TEXT("coke_lab"));
    const double Rate1 = B.ProductionRate(TEXT("coke_lab"));
    B.Upgrade(TEXT("coke_lab"), 0, 100000);
    B.Upgrade(TEXT("coke_lab"), 0, 100000);
    const FVentureMoneyResult Past = B.Upgrade(TEXT("coke_lab"), 0, 100000);
    TestTrue(TEXT("success"), R.bSuccess);
    TestEqual(TEXT("tier 1"), Tier1, 1);
    TestTrue(TEXT("rate1 > rate0"), Rate1 > Rate0);
    TestFalse(TEXT("past max fails"), Past.bSuccess);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizSalePriceScalesTest,
    "GTC.Systems.Economy.BusinessVenture.SalePriceScalesWithDemandAndHeat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizSalePriceScalesTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    const int32 Base = B.SalePrice(TEXT("coke_lab"), 1.0, 0.0);
    const int32 HighDemand = B.SalePrice(TEXT("coke_lab"), 2.0, 0.0);
    const int32 Hot = B.SalePrice(TEXT("coke_lab"), 1.0, 1.0);
    TestEqual(TEXT("base 2000"), Base, 2000);
    TestTrue(TEXT("high > base"), HighDemand > Base);
    TestEqual(TEXT("hot == discounted"), Hot, static_cast<int32>(2000 * (1.0 - BusinessVenture::HeatDiscount)));
    TestTrue(TEXT("hot < base"), Hot < Base);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizSellTest,
    "GTC.Systems.Economy.BusinessVenture.SellPaysProceedsAndClearsStock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizSellTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    B.Acquire(TEXT("coke_lab"), 0, 1000);
    B.BuySupplies(TEXT("coke_lab"), 400, 1, 1000);
    B.Accrue(1.0);
    const int32 Price = B.SalePrice(TEXT("coke_lab"), 1.0, 0.0);
    const double Before = B.ProductIn(TEXT("coke_lab"));
    const FVentureSellResult R = B.Sell(TEXT("coke_lab"), 5, 1.0, 0.0);
    const int32 ExpectSold = FMath::Min(5, static_cast<int32>(FMath::FloorToDouble(Before)));
    TestTrue(TEXT("success"), R.bSuccess);
    TestEqual(TEXT("sold"), R.Sold, ExpectSold);
    TestEqual(TEXT("proceeds"), R.Proceeds, ExpectSold * Price);
    TestEqual(TEXT("product reduced"), B.ProductIn(TEXT("coke_lab")), Before - static_cast<double>(ExpectSold), Eps);
    TestEqual(TEXT("gross"), B.GrossEarned(), R.Proceeds);
    B.Sell(TEXT("coke_lab"), 999, 1.0, 0.0);
    const FVentureSellResult Empty = B.Sell(TEXT("coke_lab"), 1, 1.0, 0.0);
    const FVentureSellResult Unowned = B.Sell(TEXT("nightclub"), 1, 1.0, 0.0);
    TestFalse(TEXT("empty fails"), Empty.bSuccess);
    TestFalse(TEXT("unowned fails"), Unowned.bSuccess);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizTotalGrossTest,
    "GTC.Systems.Economy.BusinessVenture.TotalAndGrossAggregate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizTotalGrossTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    B.Acquire(TEXT("coke_lab"), 0, 100000);
    B.Acquire(TEXT("weed_farm"), 0, 100000);
    B.BuySupplies(TEXT("coke_lab"), 400, 1, 1000);
    B.BuySupplies(TEXT("weed_farm"), 1000, 1, 1000);
    B.Accrue(1.0);
    const FVentureSellResult R1 = B.Sell(TEXT("coke_lab"), 4, 1.0, 0.0);
    const FVentureSellResult R2 = B.Sell(TEXT("weed_farm"), 2, 1.0, 0.0);
    TestEqual(TEXT("total 16"), B.TotalProduct(), 16);
    TestEqual(TEXT("gross sum"), B.GrossEarned(), R1.Proceeds + R2.Proceeds);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizSerializeRestoreTest,
    "GTC.Systems.Economy.BusinessVenture.SerializeRestoreRoundtrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizSerializeRestoreTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    B.Acquire(TEXT("coke_lab"), 0, 100000);
    B.BuySupplies(TEXT("coke_lab"), 100, 1, 1000);
    B.Hire(TEXT("coke_lab"));
    B.Hire(TEXT("coke_lab"));
    B.Upgrade(TEXT("coke_lab"), 0, 100000);
    B.Accrue(0.5);
    const FVentureSaveData Snap = B.Serialize();
    BusinessVenture Fresh;
    Fresh.Restore(Snap);
    TestEqual(TEXT("owned match"), Fresh.OwnedIds(), B.OwnedIds());
    TestEqual(TEXT("supply match"), Fresh.SupplyIn(TEXT("coke_lab")), B.SupplyIn(TEXT("coke_lab")), Eps);
    TestEqual(TEXT("product match"), Fresh.ProductIn(TEXT("coke_lab")), B.ProductIn(TEXT("coke_lab")), Eps);
    TestEqual(TEXT("staff match"), Fresh.StaffIn(TEXT("coke_lab")), B.StaffIn(TEXT("coke_lab")));
    TestEqual(TEXT("tier match"), Fresh.TierIn(TEXT("coke_lab")), B.TierIn(TEXT("coke_lab")));
    BusinessVenture Bad;
    FVentureSaveData Garbage;
    Garbage.bValid = false;
    Bad.Restore(Garbage);
    TestEqual(TEXT("bad empty"), Bad.OwnedIds().Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizChargesFittedTest,
    "GTC.Systems.Economy.BusinessVenture.BuySuppliesChargesOnlyFittedUnits",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizChargesFittedTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    B.Acquire(TEXT("coke_lab"), 0, 10000000);
    const FVentureMoneyResult R = B.BuySupplies(TEXT("coke_lab"), 1000000, 1, 10000000);
    TestEqual(TEXT("cost 400"), R.Cost, 400);
    TestEqual(TEXT("new_balance"), R.NewBalance, 10000000 - 400);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizNonPositiveCostTest,
    "GTC.Systems.Economy.BusinessVenture.BuySuppliesRejectsNonpositiveUnitCost",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizNonPositiveCostTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    B.Acquire(TEXT("coke_lab"), 0, 1000);
    const FVentureMoneyResult Neg = B.BuySupplies(TEXT("coke_lab"), 400, -5, 1000);
    const FVentureMoneyResult Zero = B.BuySupplies(TEXT("coke_lab"), 400, 0, 1000);
    TestFalse(TEXT("neg fails"), Neg.bSuccess);
    TestEqual(TEXT("neg balance"), Neg.NewBalance, 1000);
    TestFalse(TEXT("zero fails"), Zero.bSuccess);
    TestEqual(TEXT("supply 0"), B.SupplyIn(TEXT("coke_lab")), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizAlreadyFullTest,
    "GTC.Systems.Economy.BusinessVenture.BuySuppliesAlreadyFullFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizAlreadyFullTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    B.Acquire(TEXT("coke_lab"), 0, 100000);
    B.BuySupplies(TEXT("coke_lab"), 400, 1, 100000);
    const FVentureMoneyResult Again = B.BuySupplies(TEXT("coke_lab"), 50, 1, 100000);
    TestFalse(TEXT("again fails"), Again.bSuccess);
    TestEqual(TEXT("supply 400"), B.SupplyIn(TEXT("coke_lab")), 400.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizInsufficientAfterClampTest,
    "GTC.Systems.Economy.BusinessVenture.BuySuppliesInsufficientAfterClamp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizInsufficientAfterClampTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    B.Acquire(TEXT("coke_lab"), 0, 100000);
    const FVentureMoneyResult R = B.BuySupplies(TEXT("coke_lab"), 400, 100, 39999);
    TestFalse(TEXT("fails"), R.bSuccess);
    TestEqual(TEXT("balance unchanged"), R.NewBalance, 39999);
    TestEqual(TEXT("supply 0"), B.SupplyIn(TEXT("coke_lab")), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizUpgradeChargesTest,
    "GTC.Systems.Economy.BusinessVenture.UpgradeChargesWallet",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizUpgradeChargesTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    B.Acquire(TEXT("coke_lab"), 0, 100000);
    const FVentureMoneyResult R = B.Upgrade(TEXT("coke_lab"), 20000, 100000);
    TestTrue(TEXT("success"), R.bSuccess);
    TestEqual(TEXT("cost 20000"), R.Cost, 20000);
    TestEqual(TEXT("new_balance 80000"), R.NewBalance, 80000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizSalePriceUnknownTest,
    "GTC.Systems.Economy.BusinessVenture.SalePriceUnknownVentureIsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizSalePriceUnknownTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    TestEqual(TEXT("ghost 0"), B.SalePrice(TEXT("ghost"), 1.0, 0.0), 0);
    TestEqual(TEXT("empty 0"), B.SalePrice(TEXT(""), 2.0, 0.5), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizSalePriceFloorTest,
    "GTC.Systems.Economy.BusinessVenture.SalePriceFloorsCheapVentureAtOne",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizSalePriceFloorTest::RunTest(const FString& Parameters)
{
    BusinessVenture B({ MakeMinimal(TEXT("penny"), 5.0, 1) });
    TestEqual(TEXT("floor 1"), B.SalePrice(TEXT("penny"), 0.5, 1.0), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizSalePriceClampsTest,
    "GTC.Systems.Economy.BusinessVenture.SalePriceClampsDemandBand",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizSalePriceClampsTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    TestEqual(TEXT("floor eq"),
        B.SalePrice(TEXT("coke_lab"), 0.1, 0.0),
        B.SalePrice(TEXT("coke_lab"), BusinessVenture::DemandMin, 0.0));
    TestEqual(TEXT("ceil eq"),
        B.SalePrice(TEXT("coke_lab"), 99.0, 0.0),
        B.SalePrice(TEXT("coke_lab"), BusinessVenture::DemandMax, 0.0));
    TestTrue(TEXT("floored below base"),
        B.SalePrice(TEXT("coke_lab"), 0.1, 0.0) < B.SalePrice(TEXT("coke_lab"), 1.0, 0.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizGrossAccumTest,
    "GTC.Systems.Economy.BusinessVenture.GrossAccumulatesAcrossSells",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizGrossAccumTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    B.Acquire(TEXT("coke_lab"), 0, 100000);
    B.BuySupplies(TEXT("coke_lab"), 400, 1, 1000);
    B.Accrue(1.0);
    const FVentureSellResult R1 = B.Sell(TEXT("coke_lab"), 3, 1.0, 0.0);
    const FVentureSellResult R2 = B.Sell(TEXT("coke_lab"), 2, 1.0, 0.0);
    TestEqual(TEXT("gross sum"), B.GrossEarned(), R1.Proceeds + R2.Proceeds);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizRestoreClampsTest,
    "GTC.Systems.Economy.BusinessVenture.RestoreClampsOutOfRangeFields",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizRestoreClampsTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    FVentureSaveData Data;
    Data.bValid = true;
    Data.Gross = -500;
    FVentureState State;
    State.Id = TEXT("coke_lab");
    State.Supply = -10.0;
    State.Product = 999999.0;
    State.Staff = 99;
    State.Tier = 99;
    Data.Owned.Add(State);
    B.Restore(Data);
    TestEqual(TEXT("supply 0"), B.SupplyIn(TEXT("coke_lab")), 0.0, Eps);
    TestEqual(TEXT("product 200"), B.ProductIn(TEXT("coke_lab")), 200.0, Eps);
    TestEqual(TEXT("staff 6"), B.StaffIn(TEXT("coke_lab")), 6);
    TestEqual(TEXT("tier 3"), B.TierIn(TEXT("coke_lab")), 3);
    TestEqual(TEXT("gross 0"), B.GrossEarned(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBizHireFireUnownedTest,
    "GTC.Systems.Economy.BusinessVenture.HireFireUnownedReturnsFalse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBizHireFireUnownedTest::RunTest(const FString& Parameters)
{
    BusinessVenture B;
    TestFalse(TEXT("hire false"), B.Hire(TEXT("nightclub")));
    TestFalse(TEXT("fire false"), B.Fire(TEXT("nightclub")));
    return true;
}

#endif // WITH_AUTOMATION_TESTS
