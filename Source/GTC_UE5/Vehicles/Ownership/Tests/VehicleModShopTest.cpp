// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleModShop.h"

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_vehicle_mod_shop.gd (22 tests).

static constexpr double ModShopEps = 1.0e-4;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopStockStartsAtZeroTest,
    "GTC.Vehicles.Ownership.VehicleModShop.StockStartsAtLevelZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopStockStartsAtZeroTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    TestEqual(TEXT("engine == 0"), Shop.LevelOf(TEXT("engine")), 0);
    TestEqual(TEXT("brakes == 0"), Shop.LevelOf(TEXT("brakes")), 0);
    TestEqual(TEXT("armor == 0"), Shop.LevelOf(TEXT("armor")), 0);
    TestEqual(TEXT("tires == 0"), Shop.LevelOf(TEXT("tires")), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopFreshIsStockTest,
    "GTC.Vehicles.Ownership.VehicleModShop.FreshShopIsStock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopFreshIsStockTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    TestTrue(TEXT("is_stock"), Shop.IsStock());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopStockMultipliersOneTest,
    "GTC.Vehicles.Ownership.VehicleModShop.StockMultipliersAreOne",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopStockMultipliersOneTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    TestEqual(TEXT("top_speed == 1.0"), Shop.TopSpeedMultiplier(), 1.0, ModShopEps);
    TestEqual(TEXT("accel == 1.0"), Shop.AccelerationMultiplier(), 1.0, ModShopEps);
    TestEqual(TEXT("brake == 1.0"), Shop.BrakeMultiplier(), 1.0, ModShopEps);
    TestEqual(TEXT("armor == 1.0"), Shop.ArmorMultiplier(), 1.0, ModShopEps);
    TestEqual(TEXT("grip == 1.0"), Shop.GripMultiplier(), 1.0, ModShopEps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopMaxLevelThreeTest,
    "GTC.Vehicles.Ownership.VehicleModShop.MaxLevelIsThree",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopMaxLevelThreeTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    TestEqual(TEXT("engine max == 3"), Shop.MaxLevel(TEXT("engine")), 3);
    TestEqual(TEXT("tires max == 3"), Shop.MaxLevel(TEXT("tires")), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopPriceForTiersTest,
    "GTC.Vehicles.Ownership.VehicleModShop.PriceForTiers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopPriceForTiersTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    TestEqual(TEXT("engine 1 == 2000"), Shop.PriceFor(TEXT("engine"), 1), 2000);
    TestEqual(TEXT("engine 2 == 5000"), Shop.PriceFor(TEXT("engine"), 2), 5000);
    TestEqual(TEXT("engine 3 == 12000"), Shop.PriceFor(TEXT("engine"), 3), 12000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopPricePastMaxTest,
    "GTC.Vehicles.Ownership.VehicleModShop.PriceForPastMaxIsMinusOne",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopPricePastMaxTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    TestEqual(TEXT("engine 4 == -1"), Shop.PriceFor(TEXT("engine"), 4), -1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopPriceUnknownTest,
    "GTC.Vehicles.Ownership.VehicleModShop.PriceForUnknownCategoryIsMinusOne",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopPriceUnknownTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    TestEqual(TEXT("nitro 1 == -1"), Shop.PriceFor(TEXT("nitro"), 1), -1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopUpgradeRaisesTest,
    "GTC.Vehicles.Ownership.VehicleModShop.UpgradeRaisesLevelAndDeducts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopUpgradeRaisesTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    const VehicleModShop::FModUpgradeResult Result = Shop.Upgrade(TEXT("engine"), 10000);
    TestTrue(TEXT("success"), Result.bSuccess);
    TestEqual(TEXT("cost == 2000"), Result.Cost, 2000);
    TestEqual(TEXT("new_balance == 8000"), Result.NewBalance, 8000);
    TestEqual(TEXT("new_level == 1"), Result.NewLevel, 1);
    TestEqual(TEXT("level == 1"), Shop.LevelOf(TEXT("engine")), 1);
    TestFalse(TEXT("not stock"), Shop.IsStock());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopUpgradeStepsTiersTest,
    "GTC.Vehicles.Ownership.VehicleModShop.UpgradeStepsThroughTiers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopUpgradeStepsTiersTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    Shop.Upgrade(TEXT("brakes"), 50000);
    const VehicleModShop::FModUpgradeResult Second = Shop.Upgrade(TEXT("brakes"), 50000);
    TestEqual(TEXT("cost == 3500"), Second.Cost, 3500);
    TestEqual(TEXT("new_level == 2"), Second.NewLevel, 2);
    TestEqual(TEXT("level == 2"), Shop.LevelOf(TEXT("brakes")), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopUpgradeFailsAtMaxTest,
    "GTC.Vehicles.Ownership.VehicleModShop.UpgradeFailsAtMax",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopUpgradeFailsAtMaxTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    Shop.Upgrade(TEXT("tires"), 100000);
    Shop.Upgrade(TEXT("tires"), 100000);
    Shop.Upgrade(TEXT("tires"), 100000);
    const VehicleModShop::FModUpgradeResult Maxed = Shop.Upgrade(TEXT("tires"), 100000);
    TestFalse(TEXT("not success"), Maxed.bSuccess);
    TestEqual(TEXT("new_level == -1"), Maxed.NewLevel, -1);
    TestEqual(TEXT("level == 3"), Shop.LevelOf(TEXT("tires")), 3);
    TestFalse(TEXT("can't upgrade"), Shop.CanUpgrade(TEXT("tires")));
    TestTrue(TEXT("reason mentions maxed"), Maxed.Reason.Contains(TEXT("maxed")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopUpgradeInsufficientTest,
    "GTC.Vehicles.Ownership.VehicleModShop.UpgradeFailsInsufficientFunds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopUpgradeInsufficientTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    const VehicleModShop::FModUpgradeResult Result = Shop.Upgrade(TEXT("armor"), 100);
    TestFalse(TEXT("not success"), Result.bSuccess);
    TestEqual(TEXT("new_balance == 100"), Result.NewBalance, 100);
    TestEqual(TEXT("level == 0"), Shop.LevelOf(TEXT("armor")), 0);
    TestTrue(TEXT("reason mentions insufficient"), Result.Reason.Contains(TEXT("insufficient")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopUpgradeUnknownSafeTest,
    "GTC.Vehicles.Ownership.VehicleModShop.UpgradeUnknownCategorySafe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopUpgradeUnknownSafeTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    const VehicleModShop::FModUpgradeResult Result = Shop.Upgrade(TEXT("turbo"), 99999);
    TestFalse(TEXT("not success"), Result.bSuccess);
    TestEqual(TEXT("new_balance == 99999"), Result.NewBalance, 99999);
    TestTrue(TEXT("is_stock"), Shop.IsStock());
    TestTrue(TEXT("reason mentions unknown"), Result.Reason.Contains(TEXT("unknown")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopCanUpgradeStateTest,
    "GTC.Vehicles.Ownership.VehicleModShop.CanUpgradeReflectsState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopCanUpgradeStateTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    const bool bFresh = Shop.CanUpgrade(TEXT("engine"));
    Shop.Upgrade(TEXT("engine"), 100000);
    Shop.Upgrade(TEXT("engine"), 100000);
    Shop.Upgrade(TEXT("engine"), 100000);
    TestTrue(TEXT("fresh can upgrade"), bFresh);
    TestFalse(TEXT("maxed cannot"), Shop.CanUpgrade(TEXT("engine")));
    TestFalse(TEXT("ghost cannot"), Shop.CanUpgrade(TEXT("ghost")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopEngineSpeedAccelTest,
    "GTC.Vehicles.Ownership.VehicleModShop.EngineRaisesSpeedAndAccel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopEngineSpeedAccelTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    Shop.Upgrade(TEXT("engine"), 100000);
    Shop.Upgrade(TEXT("engine"), 100000);
    TestEqual(TEXT("top_speed == 1.16"), Shop.TopSpeedMultiplier(), 1.16, ModShopEps);
    TestEqual(TEXT("accel == 1.12"), Shop.AccelerationMultiplier(), 1.12, ModShopEps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopTiresGripOnlyTest,
    "GTC.Vehicles.Ownership.VehicleModShop.TiresRaiseGripOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopTiresGripOnlyTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    Shop.Upgrade(TEXT("tires"), 100000);
    TestEqual(TEXT("grip == 1.07"), Shop.GripMultiplier(), 1.07, ModShopEps);
    TestEqual(TEXT("top_speed == 1.0"), Shop.TopSpeedMultiplier(), 1.0, ModShopEps);
    TestEqual(TEXT("brake == 1.0"), Shop.BrakeMultiplier(), 1.0, ModShopEps);
    TestEqual(TEXT("armor == 1.0"), Shop.ArmorMultiplier(), 1.0, ModShopEps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopBrakesBrakeOnlyTest,
    "GTC.Vehicles.Ownership.VehicleModShop.BrakesRaiseBrakeOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopBrakesBrakeOnlyTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    Shop.Upgrade(TEXT("brakes"), 100000);
    Shop.Upgrade(TEXT("brakes"), 100000);
    TestEqual(TEXT("brake == 1.2"), Shop.BrakeMultiplier(), 1.2, ModShopEps);
    TestEqual(TEXT("grip == 1.0"), Shop.GripMultiplier(), 1.0, ModShopEps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopArmorArmorOnlyTest,
    "GTC.Vehicles.Ownership.VehicleModShop.ArmorRaisesArmorOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopArmorArmorOnlyTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    Shop.Upgrade(TEXT("armor"), 100000);
    Shop.Upgrade(TEXT("armor"), 100000);
    Shop.Upgrade(TEXT("armor"), 100000);
    TestEqual(TEXT("armor == 1.75"), Shop.ArmorMultiplier(), 1.75, ModShopEps);
    TestEqual(TEXT("top_speed == 1.0"), Shop.TopSpeedMultiplier(), 1.0, ModShopEps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopMonotonicTest,
    "GTC.Vehicles.Ownership.VehicleModShop.MultipliersRiseMonotonically",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopMonotonicTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    const double Before = Shop.TopSpeedMultiplier();
    Shop.Upgrade(TEXT("engine"), 100000);
    const double Mid = Shop.TopSpeedMultiplier();
    Shop.Upgrade(TEXT("engine"), 100000);
    const double After = Shop.TopSpeedMultiplier();
    TestTrue(TEXT("before < mid"), Before < Mid);
    TestTrue(TEXT("mid < after"), Mid < After);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopTotalSpentTest,
    "GTC.Vehicles.Ownership.VehicleModShop.TotalSpentSumsTiers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopTotalSpentTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    Shop.Upgrade(TEXT("engine"), 100000);
    Shop.Upgrade(TEXT("engine"), 100000);
    Shop.Upgrade(TEXT("tires"), 100000);
    TestEqual(TEXT("total == 8000"), Shop.TotalSpent(), 2000 + 5000 + 1000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopSerializeRoundTripTest,
    "GTC.Vehicles.Ownership.VehicleModShop.SerializeRestoreRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopSerializeRoundTripTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    Shop.Upgrade(TEXT("engine"), 100000);
    Shop.Upgrade(TEXT("armor"), 100000);
    const TArray<TPair<FString, int32>> Snapshot = Shop.Serialize();
    VehicleModShop Other;
    Other.Restore(Snapshot);
    TestEqual(TEXT("engine == 1"), Other.LevelOf(TEXT("engine")), 1);
    TestEqual(TEXT("armor == 1"), Other.LevelOf(TEXT("armor")), 1);
    TestEqual(TEXT("brakes == 0"), Other.LevelOf(TEXT("brakes")), 0);
    TestEqual(TEXT("top_speed == 1.08"), Other.TopSpeedMultiplier(), 1.08, ModShopEps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopRestoreClampsTest,
    "GTC.Vehicles.Ownership.VehicleModShop.RestoreClampsOutOfRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopRestoreClampsTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    TArray<TPair<FString, int32>> State;
    State.Emplace(TEXT("engine"), 99);
    State.Emplace(TEXT("tires"), -3);
    Shop.Restore(State);
    TestEqual(TEXT("engine clamps to 3"), Shop.LevelOf(TEXT("engine")), 3);
    TestEqual(TEXT("tires clamps to 0"), Shop.LevelOf(TEXT("tires")), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleModShopResetTest,
    "GTC.Vehicles.Ownership.VehicleModShop.ResetReturnsToStock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleModShopResetTest::RunTest(const FString& Parameters)
{
    VehicleModShop Shop;
    Shop.Upgrade(TEXT("engine"), 100000);
    Shop.Upgrade(TEXT("brakes"), 100000);
    Shop.Reset();
    TestTrue(TEXT("is_stock"), Shop.IsStock());
    TestEqual(TEXT("engine == 0"), Shop.LevelOf(TEXT("engine")), 0);
    TestEqual(TEXT("brake == 1.0"), Shop.BrakeMultiplier(), 1.0, ModShopEps);
    TestEqual(TEXT("total == 0"), Shop.TotalSpent(), 0);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
