// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../ShopModel.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to an assertion in the the reference reference behavior
// game/tests/unit/test_shop_model.gd. Deterministic, concrete numbers.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopDefaultNonEmptyTest,
    "GTC.Systems.Economy.ShopModel.DefaultCatalogueNonEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopDefaultNonEmptyTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    TestTrue(TEXT("item_count > 0"), Shop.ItemCount() > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopDefaultStaticMatchesTest,
    "GTC.Systems.Economy.ShopModel.DefaultCatalogueStaticMatches",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopDefaultStaticMatchesTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    TestEqual(TEXT("count == default size"), Shop.ItemCount(), ShopModel::DefaultCatalogue().Num());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopHasItemTest,
    "GTC.Systems.Economy.ShopModel.HasItemKnownAndUnknown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopHasItemTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    TestTrue(TEXT("has pistol"), Shop.HasItem(TEXT("pistol")));
    TestFalse(TEXT("not nope"), Shop.HasItem(TEXT("nope")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopPriceKnownTest,
    "GTC.Systems.Economy.ShopModel.PriceOfKnown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopPriceKnownTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    TestEqual(TEXT("pistol == 500"), Shop.PriceOf(TEXT("pistol")), 500);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopPriceUnknownTest,
    "GTC.Systems.Economy.ShopModel.PriceOfUnknownIsMinusOne",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopPriceUnknownTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    TestEqual(TEXT("unknown == -1"), Shop.PriceOf(TEXT("rocket_launcher")), -1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopCanAffordAboveTest,
    "GTC.Systems.Economy.ShopModel.CanAffordAbovePrice",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopCanAffordAboveTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    TestTrue(TEXT("afford 1000"), Shop.CanAfford(TEXT("pistol"), 1000));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopCanAffordBoundaryTest,
    "GTC.Systems.Economy.ShopModel.CanAffordAtExactBoundary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopCanAffordBoundaryTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    TestTrue(TEXT("afford exact 500"), Shop.CanAfford(TEXT("pistol"), 500));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopCanAffordBelowTest,
    "GTC.Systems.Economy.ShopModel.CanAffordFalseBelowPrice",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopCanAffordBelowTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    TestFalse(TEXT("not afford 499"), Shop.CanAfford(TEXT("pistol"), 499));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopCanAffordUnknownTest,
    "GTC.Systems.Economy.ShopModel.CanAffordUnknownIsFalse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopCanAffordUnknownTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    TestFalse(TEXT("unknown not affordable"), Shop.CanAfford(TEXT("ghost"), 999999));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopPurchaseDeductsTest,
    "GTC.Systems.Economy.ShopModel.PurchaseDeductsExactly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopPurchaseDeductsTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    const FShopPurchase Result = Shop.Purchase(TEXT("smg"), 3000);
    TestTrue(TEXT("success"), Result.bSuccess);
    TestEqual(TEXT("cost == 2500"), Result.Cost, 2500);
    TestEqual(TEXT("new_balance == 500"), Result.NewBalance, 500);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopPurchaseExactTest,
    "GTC.Systems.Economy.ShopModel.PurchaseExactBalanceToZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopPurchaseExactTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    const FShopPurchase Result = Shop.Purchase(TEXT("body_armor"), 1000);
    TestTrue(TEXT("success"), Result.bSuccess);
    TestEqual(TEXT("new_balance == 0"), Result.NewBalance, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopPurchaseInsufficientTest,
    "GTC.Systems.Economy.ShopModel.PurchaseInsufficientFundsFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopPurchaseInsufficientTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    const FShopPurchase Result = Shop.Purchase(TEXT("rifle"), 100);
    TestFalse(TEXT("not success"), Result.bSuccess);
    TestEqual(TEXT("new_balance unchanged"), Result.NewBalance, 100);
    TestEqual(TEXT("cost == 0"), Result.Cost, 0);
    TestFalse(TEXT("reason set"), Result.Reason.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopPurchaseUnknownTest,
    "GTC.Systems.Economy.ShopModel.PurchaseUnknownIdFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopPurchaseUnknownTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    const FShopPurchase Result = Shop.Purchase(TEXT("flamethrower"), 50000);
    TestFalse(TEXT("not success"), Result.bSuccess);
    TestEqual(TEXT("new_balance unchanged"), Result.NewBalance, 50000);
    TestFalse(TEXT("reason set"), Result.Reason.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopCategoryWeaponsTest,
    "GTC.Systems.Economy.ShopModel.ItemsInCategoryFiltersWeapons",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopCategoryWeaponsTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    TestEqual(TEXT("3 weapons"), Shop.ItemsInCategory(TEXT("weapon")).Num(), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopCategoryVehiclesTest,
    "GTC.Systems.Economy.ShopModel.ItemsInCategoryFiltersVehicles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopCategoryVehiclesTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    TestEqual(TEXT("2 vehicles"), Shop.ItemsInCategory(TEXT("vehicle")).Num(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopCategoryUnknownTest,
    "GTC.Systems.Economy.ShopModel.ItemsInCategoryUnknownIsEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopCategoryUnknownTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    TestEqual(TEXT("empty aircraft"), Shop.ItemsInCategory(TEXT("aircraft")).Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopSellValueHalfTest,
    "GTC.Systems.Economy.ShopModel.SellValueDefaultHalf",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopSellValueHalfTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    TestEqual(TEXT("smg half == 1250"), Shop.SellValue(TEXT("smg")), 1250);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopSellValueCustomTest,
    "GTC.Systems.Economy.ShopModel.SellValueCustomFraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopSellValueCustomTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    TestEqual(TEXT("armor 0.25 == 250"), Shop.SellValue(TEXT("body_armor"), 0.25), 250);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopSellValueClampTest,
    "GTC.Systems.Economy.ShopModel.SellValueFractionClamped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopSellValueClampTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    TestEqual(TEXT("pistol 2.0 -> 500"), Shop.SellValue(TEXT("pistol"), 2.0), 500);
    TestEqual(TEXT("pistol -1.0 -> 0"), Shop.SellValue(TEXT("pistol"), -1.0), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopSellValueUnknownTest,
    "GTC.Systems.Economy.ShopModel.SellValueUnknownIsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopSellValueUnknownTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    TestEqual(TEXT("nope -> 0"), Shop.SellValue(TEXT("nope")), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopCustomCatalogueTest,
    "GTC.Systems.Economy.ShopModel.CustomCatalogueOverridesDefault",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopCustomCatalogueTest::RunTest(const FString& Parameters)
{
    ShopModel Shop({ FShopCatalogueItem(TEXT("bat"), TEXT("Bat"), 75, TEXT("melee")) });
    TestEqual(TEXT("count == 1"), Shop.ItemCount(), 1);
    TestEqual(TEXT("bat == 75"), Shop.PriceOf(TEXT("bat")), 75);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopGarbageDroppedTest,
    "GTC.Systems.Economy.ShopModel.GarbageEntriesDropped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopGarbageDroppedTest::RunTest(const FString& Parameters)
{
    // Typed-port note: the reference non-int-price ("free") and non-dict ("not_a_dict")
    // rows are unrepresentable with a typed FShopCatalogueItem; the empty-id and negative-price
    // rows that ARE representable cover the garbage-drop path 1:1.
    ShopModel Shop({
        FShopCatalogueItem(TEXT("good"), TEXT("good"), 100, TEXT("weapon")),
        FShopCatalogueItem(TEXT(""), TEXT(""), 50, TEXT("")),
        FShopCatalogueItem(TEXT("negative"), TEXT("negative"), -5, TEXT("")),
    });
    TestEqual(TEXT("count == 1"), Shop.ItemCount(), 1);
    TestTrue(TEXT("has good"), Shop.HasItem(TEXT("good")));
    return true;
}

#endif // WITH_AUTOMATION_TESTS
