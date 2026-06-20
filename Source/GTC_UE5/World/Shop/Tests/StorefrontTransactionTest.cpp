// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../StorefrontTransaction.h"
#include "../../../Systems/Economy/ShopModel.h"

// The pure, new logic under test is the category -> grant-kind classification and the
// ResolveBuy fold over the (separately parity-tested) ShopModel. Deterministic numbers
// keyed to ShopModel::DefaultCatalogue (pistol 500 weapon, body_armor 1000 armor,
// rifle 7500 weapon).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStorefrontGrantWeaponTest,
    "GTC.World.Shop.Transaction.GrantKindWeapon",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStorefrontGrantWeaponTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("weapon -> Weapon"),
        static_cast<int32>(FStorefrontTransaction::GrantKindFromCategory(TEXT("weapon"))),
        static_cast<int32>(EStorefrontGrantKind::Weapon));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStorefrontGrantAmmoTest,
    "GTC.World.Shop.Transaction.GrantKindAmmo",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStorefrontGrantAmmoTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("ammo -> Ammo"),
        static_cast<int32>(FStorefrontTransaction::GrantKindFromCategory(TEXT("ammo"))),
        static_cast<int32>(EStorefrontGrantKind::Ammo));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStorefrontGrantArmorTest,
    "GTC.World.Shop.Transaction.GrantKindArmor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStorefrontGrantArmorTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("armor -> Armor"),
        static_cast<int32>(FStorefrontTransaction::GrantKindFromCategory(TEXT("armor"))),
        static_cast<int32>(EStorefrontGrantKind::Armor));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStorefrontGrantVehicleTest,
    "GTC.World.Shop.Transaction.GrantKindVehicle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStorefrontGrantVehicleTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("vehicle -> Vehicle"),
        static_cast<int32>(FStorefrontTransaction::GrantKindFromCategory(TEXT("vehicle"))),
        static_cast<int32>(EStorefrontGrantKind::Vehicle));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStorefrontGrantUnknownTest,
    "GTC.World.Shop.Transaction.GrantKindUnknownCategory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStorefrontGrantUnknownTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("spaceship -> Unknown"),
        static_cast<int32>(FStorefrontTransaction::GrantKindFromCategory(TEXT("spaceship"))),
        static_cast<int32>(EStorefrontGrantKind::Unknown));
    TestEqual(TEXT("empty -> Unknown"),
        static_cast<int32>(FStorefrontTransaction::GrantKindFromCategory(TEXT(""))),
        static_cast<int32>(EStorefrontGrantKind::Unknown));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStorefrontGrantCaseInsensitiveTest,
    "GTC.World.Shop.Transaction.GrantKindCaseInsensitive",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStorefrontGrantCaseInsensitiveTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("WEAPON -> Weapon"),
        static_cast<int32>(FStorefrontTransaction::GrantKindFromCategory(TEXT("WEAPON"))),
        static_cast<int32>(EStorefrontGrantKind::Weapon));
    TestEqual(TEXT("Vehicle -> Vehicle"),
        static_cast<int32>(FStorefrontTransaction::GrantKindFromCategory(TEXT("Vehicle"))),
        static_cast<int32>(EStorefrontGrantKind::Vehicle));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStorefrontResolveBuySuccessTest,
    "GTC.World.Shop.Transaction.ResolveBuyWeaponSuccess",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStorefrontResolveBuySuccessTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    const FShopCatalogueItem Item(TEXT("pistol"), TEXT("Pistol"), 500, TEXT("weapon"));
    const FStorefrontBuyDecision D = FStorefrontTransaction::ResolveBuy(Shop, Item, 1000);
    TestTrue(TEXT("success"), D.bSuccess);
    TestEqual(TEXT("cost == 500"), D.Cost, 500);
    TestEqual(TEXT("new_balance == 500"), D.NewBalance, 500);
    TestEqual(TEXT("grant == Weapon"), static_cast<int32>(D.Grant), static_cast<int32>(EStorefrontGrantKind::Weapon));
    TestEqual(TEXT("name carried"), D.ItemName, FString(TEXT("Pistol")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStorefrontResolveBuyArmorTest,
    "GTC.World.Shop.Transaction.ResolveBuyArmorToZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStorefrontResolveBuyArmorTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    const FShopCatalogueItem Item(TEXT("body_armor"), TEXT("Body Armor"), 1000, TEXT("armor"));
    const FStorefrontBuyDecision D = FStorefrontTransaction::ResolveBuy(Shop, Item, 1000);
    TestTrue(TEXT("success"), D.bSuccess);
    TestEqual(TEXT("new_balance == 0"), D.NewBalance, 0);
    TestEqual(TEXT("grant == Armor"), static_cast<int32>(D.Grant), static_cast<int32>(EStorefrontGrantKind::Armor));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStorefrontResolveBuyInsufficientTest,
    "GTC.World.Shop.Transaction.ResolveBuyInsufficientKeepsBalance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStorefrontResolveBuyInsufficientTest::RunTest(const FString& Parameters)
{
    ShopModel Shop;
    const FShopCatalogueItem Item(TEXT("rifle"), TEXT("Assault Rifle"), 7500, TEXT("weapon"));
    const FStorefrontBuyDecision D = FStorefrontTransaction::ResolveBuy(Shop, Item, 100);
    TestFalse(TEXT("not success"), D.bSuccess);
    TestEqual(TEXT("balance unchanged"), D.NewBalance, 100);
    TestEqual(TEXT("cost == 0"), D.Cost, 0);
    TestEqual(TEXT("grant Unknown on fail"), static_cast<int32>(D.Grant), static_cast<int32>(EStorefrontGrantKind::Unknown));
    TestFalse(TEXT("reason set"), D.Reason.IsEmpty());
    return true;
}

#endif // WITH_AUTOMATION_TESTS
