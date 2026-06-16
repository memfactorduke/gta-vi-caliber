// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../PropertyOwnership.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_property_ownership.gd. Float compares use Eps.

namespace
{
    // The oracle's _fixture(): one safehouse, two businesses.
    PropertyOwnership MakeFixture()
    {
        TArray<FPropertyDef> Catalogue;
        Catalogue.Emplace(TEXT("loft"), TEXT("Loft"), 1000, 0, true);
        Catalogue.Emplace(TEXT("club"), TEXT("Club"), 5000, 200, false);
        Catalogue.Emplace(TEXT("taxi"), TEXT("Taxi Firm"), 2000, 50, false);
        return PropertyOwnership(Catalogue);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropDefaultNonEmptyTest,
    "GTC.Systems.Economy.PropertyOwnership.DefaultCatalogueNonEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropDefaultNonEmptyTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P;
    TestTrue(TEXT("property_count >= 4"), P.PropertyCount() >= 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropFixtureCountTest,
    "GTC.Systems.Economy.PropertyOwnership.FixturePropertyCount",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropFixtureCountTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("count == 3"), MakeFixture().PropertyCount(), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropPriceTest,
    "GTC.Systems.Economy.PropertyOwnership.PriceOfKnownAndUnknown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropPriceTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    TestEqual(TEXT("club == 5000"), P.PriceOf(TEXT("club")), 5000);
    TestEqual(TEXT("ghost == -1"), P.PriceOf(TEXT("ghost")), -1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropIncomeTest,
    "GTC.Systems.Economy.PropertyOwnership.IncomeOfBusinessAndSafehouse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropIncomeTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    TestEqual(TEXT("club == 200"), P.IncomeOf(TEXT("club")), 200);
    TestEqual(TEXT("loft == 0"), P.IncomeOf(TEXT("loft")), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropBuyTest,
    "GTC.Systems.Economy.PropertyOwnership.BuyDeductsAndMarksOwned",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropBuyTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    const FPropertyBuyResult R = P.Buy(TEXT("club"), 8000);
    TestTrue(TEXT("success"), R.bSuccess);
    TestEqual(TEXT("cost == 5000"), R.Cost, 5000);
    TestEqual(TEXT("new_balance == 3000"), R.NewBalance, 3000);
    TestTrue(TEXT("reason empty"), R.Reason.IsEmpty());
    TestTrue(TEXT("owns club"), P.Owns(TEXT("club")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropBuyUnknownTest,
    "GTC.Systems.Economy.PropertyOwnership.BuyUnknownFailsUnchanged",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropBuyUnknownTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    const FPropertyBuyResult R = P.Buy(TEXT("ghost"), 9999);
    TestFalse(TEXT("not success"), R.bSuccess);
    TestEqual(TEXT("balance unchanged"), R.NewBalance, 9999);
    TestFalse(TEXT("not owned"), P.Owns(TEXT("ghost")));
    TestTrue(TEXT("reason contains unknown"), R.Reason.Contains(TEXT("unknown")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropBuyOwnedTest,
    "GTC.Systems.Economy.PropertyOwnership.BuyAlreadyOwnedFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropBuyOwnedTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    P.Buy(TEXT("taxi"), 2000);
    const FPropertyBuyResult R = P.Buy(TEXT("taxi"), 2000);
    TestFalse(TEXT("not success"), R.bSuccess);
    TestTrue(TEXT("reason contains already owned"), R.Reason.Contains(TEXT("already owned")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropBuyInsufficientTest,
    "GTC.Systems.Economy.PropertyOwnership.BuyInsufficientFailsUnchanged",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropBuyInsufficientTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    const FPropertyBuyResult R = P.Buy(TEXT("club"), 4999);
    TestFalse(TEXT("not success"), R.bSuccess);
    TestEqual(TEXT("balance unchanged"), R.NewBalance, 4999);
    TestFalse(TEXT("not owned"), P.Owns(TEXT("club")));
    TestTrue(TEXT("reason contains insufficient"), R.Reason.Contains(TEXT("insufficient")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropOwnedIdsSortedTest,
    "GTC.Systems.Economy.PropertyOwnership.OwnedIdsUpdateAndSorted",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropOwnedIdsSortedTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    P.Buy(TEXT("taxi"), 2000);
    P.Buy(TEXT("club"), 5000);
    const TArray<FString> Owned = P.OwnedIds();
    TestEqual(TEXT("count"), Owned.Num(), 2);
    TestEqual(TEXT("[0] == club"), Owned[0], FString(TEXT("club")));
    TestEqual(TEXT("[1] == taxi"), Owned[1], FString(TEXT("taxi")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropAccrueGrowsTest,
    "GTC.Systems.Economy.PropertyOwnership.AccrueGrowsPendingWithDays",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropAccrueGrowsTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    P.Buy(TEXT("club"), 5000);
    P.Accrue(3.0);
    TestEqual(TEXT("pending == 600"), P.PendingIncome(), 600.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropAccrueSumsTest,
    "GTC.Systems.Economy.PropertyOwnership.AccrueSumsOwnedBusinesses",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropAccrueSumsTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    P.Buy(TEXT("club"), 5000);
    P.Buy(TEXT("taxi"), 2000);
    P.Accrue(2.0);
    TestEqual(TEXT("pending == 500"), P.PendingIncome(), 500.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropAccrueNegativeTest,
    "GTC.Systems.Economy.PropertyOwnership.AccrueNegativeIgnored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropAccrueNegativeTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    P.Buy(TEXT("club"), 5000);
    P.Accrue(-4.0);
    TestEqual(TEXT("pending == 0"), P.PendingIncome(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropAccrueNoneOwnedTest,
    "GTC.Systems.Economy.PropertyOwnership.AccrueWithNoneOwnedStaysZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropAccrueNoneOwnedTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    P.Accrue(5.0);
    TestEqual(TEXT("pending == 0"), P.PendingIncome(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropCollectTest,
    "GTC.Systems.Economy.PropertyOwnership.CollectReturnsPendingAndZeroes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropCollectTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    P.Buy(TEXT("club"), 5000);
    P.Accrue(2.0);
    const int32 Picked = P.Collect();
    TestEqual(TEXT("picked == 400"), Picked, 400);
    TestEqual(TEXT("pending == 0"), P.PendingIncome(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropCollectNothingTest,
    "GTC.Systems.Economy.PropertyOwnership.CollectNothingPendingIsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropCollectNothingTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    TestEqual(TEXT("collect == 0"), P.Collect(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropDailyIncomeTest,
    "GTC.Systems.Economy.PropertyOwnership.DailyIncomeSumsOwned",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropDailyIncomeTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    P.Buy(TEXT("club"), 5000);
    P.Buy(TEXT("taxi"), 2000);
    TestEqual(TEXT("daily == 250"), P.DailyIncome(), 250);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropDailyIncomeZeroTest,
    "GTC.Systems.Economy.PropertyOwnership.DailyIncomeZeroWithNoneOwned",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropDailyIncomeZeroTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("daily == 0"), MakeFixture().DailyIncome(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropHasSafehouseTest,
    "GTC.Systems.Economy.PropertyOwnership.HasSafehouseAfterBuyingOne",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropHasSafehouseTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    TestFalse(TEXT("no safehouse yet"), P.HasSafehouse());
    P.Buy(TEXT("loft"), 1000);
    TestTrue(TEXT("has safehouse"), P.HasSafehouse());
    TestEqual(TEXT("nearest == loft"), P.NearestSafehouseOwned(), FString(TEXT("loft")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropTotalInvestedTest,
    "GTC.Systems.Economy.PropertyOwnership.TotalInvested",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropTotalInvestedTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    P.Buy(TEXT("loft"), 1000);
    P.Buy(TEXT("club"), 5000);
    TestEqual(TEXT("invested == 6000"), P.TotalInvested(), 6000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropSerializeRestoreTest,
    "GTC.Systems.Economy.PropertyOwnership.SerializeRestoreRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropSerializeRestoreTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    P.Buy(TEXT("club"), 5000);
    P.Buy(TEXT("loft"), 1000);
    P.Accrue(2.0);
    const FPropertySaveData Snapshot = P.Serialize();
    PropertyOwnership Q = MakeFixture();
    Q.Restore(Snapshot);
    const TArray<FString> Owned = Q.OwnedIds();
    TestEqual(TEXT("owned count"), Owned.Num(), 2);
    TestEqual(TEXT("[0] == club"), Owned[0], FString(TEXT("club")));
    TestEqual(TEXT("[1] == loft"), Owned[1], FString(TEXT("loft")));
    TestEqual(TEXT("pending == 400"), Q.PendingIncome(), 400.0, Eps);
    TestEqual(TEXT("daily == 200"), Q.DailyIncome(), 200);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropRestoreDropsUnknownTest,
    "GTC.Systems.Economy.PropertyOwnership.RestoreDropsUnknownIds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropRestoreDropsUnknownTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    FPropertySaveData Data;
    Data.Owned = { TEXT("club"), TEXT("ghost") };
    Data.Pending = 10.0;
    P.Restore(Data);
    const TArray<FString> Owned = P.OwnedIds();
    TestEqual(TEXT("owned count == 1"), Owned.Num(), 1);
    TestEqual(TEXT("[0] == club"), Owned[0], FString(TEXT("club")));
    TestEqual(TEXT("pending == 10"), P.PendingIncome(), 10.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPropResetTest,
    "GTC.Systems.Economy.PropertyOwnership.ResetClearsEverything",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPropResetTest::RunTest(const FString& Parameters)
{
    PropertyOwnership P = MakeFixture();
    P.Buy(TEXT("club"), 5000);
    P.Accrue(3.0);
    P.Reset();
    TestEqual(TEXT("owned empty"), P.OwnedIds().Num(), 0);
    TestEqual(TEXT("pending == 0"), P.PendingIncome(), 0.0, Eps);
    TestEqual(TEXT("daily == 0"), P.DailyIncome(), 0);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
