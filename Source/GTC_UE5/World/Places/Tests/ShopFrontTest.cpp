// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../GTCShop.h"
#include "../GTCPlaceRegistrySubsystem.h"

// Pure-core tests for the shop "storefront" (FShopFront) and the place-registry
// "shop" kind lifecycle. The live actor wiring — BeginPlay POI registration,
// Interact reading the player's wallet — needs a UWorld and is PIE-deferred,
// exactly the adapter boundary AGTCDoor / FInteraction document. What IS asserted
// headless: the front's identity / prompt / purchase decision, and that the
// registry (whose register/query/unregister are world-free) treats a "shop" POI
// as a real, queryable destination.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopFrontKindTest,
    "GTC.World.Places.Shop.FrontKindIsShop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopFrontKindTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("place kind is 'shop'"), FShopFront::PlaceKind() == FName(TEXT("shop")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopFrontPromptTest,
    "GTC.World.Places.Shop.FrontPromptNonEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopFrontPromptTest::RunTest(const FString& Parameters)
{
    FShopFront Front;
    TestFalse(TEXT("prompt is set"), Front.Prompt().IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopFrontBuyDeductsTest,
    "GTC.World.Places.Shop.FrontBuyDeductsExactly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopFrontBuyDeductsTest::RunTest(const FString& Parameters)
{
    FShopFront Front;
    // pistol is 500 in the default catalogue; buying from 1000 leaves 500.
    const FShopPurchase Result = Front.Buy(TEXT("pistol"), 1000);
    TestTrue(TEXT("success"), Result.bSuccess);
    TestEqual(TEXT("cost == 500"), Result.Cost, 500);
    TestEqual(TEXT("new_balance == 500"), Result.NewBalance, 500);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopFrontBuyInsufficientTest,
    "GTC.World.Places.Shop.FrontBuyInsufficientFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopFrontBuyInsufficientTest::RunTest(const FString& Parameters)
{
    FShopFront Front;
    const FShopPurchase Result = Front.Buy(TEXT("pistol"), 100);
    TestFalse(TEXT("not success"), Result.bSuccess);
    TestEqual(TEXT("balance unchanged"), Result.NewBalance, 100);
    TestEqual(TEXT("cost == 0"), Result.Cost, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopFrontAffordBoundaryTest,
    "GTC.World.Places.Shop.FrontCanAffordAtExactPrice",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopFrontAffordBoundaryTest::RunTest(const FString& Parameters)
{
    FShopFront Front;
    TestTrue(TEXT("afford exact 500"), Front.CanAfford(TEXT("pistol"), 500));
    TestFalse(TEXT("not afford 499"), Front.CanAfford(TEXT("pistol"), 499));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopFrontCustomCatalogueTest,
    "GTC.World.Places.Shop.FrontHonoursCustomCatalogue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopFrontCustomCatalogueTest::RunTest(const FString& Parameters)
{
    FShopFront Front({ FShopCatalogueItem(TEXT("soda"), TEXT("Soda"), 3, TEXT("food")) });
    const FShopPurchase Result = Front.Buy(TEXT("soda"), 10);
    TestTrue(TEXT("bought soda"), Result.bSuccess);
    TestEqual(TEXT("cost == 3"), Result.Cost, 3);
    TestEqual(TEXT("new_balance == 7"), Result.NewBalance, 7);
    return true;
}

// The register -> FindNearest -> unregister lifecycle for a "shop" POI. The
// registry's register/query/unregister are world-free (no GetWorld()), so the
// subsystem is exercised headless via NewObject.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShopPlaceLifecycleTest,
    "GTC.World.Places.Shop.RegistersAsQueryablePoi",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopPlaceLifecycleTest::RunTest(const FString& Parameters)
{
    UGTCPlaceRegistrySubsystem* Registry = NewObject<UGTCPlaceRegistrySubsystem>();
    TestNotNull(TEXT("registry constructed"), Registry);
    if (Registry == nullptr)
    {
        return false;
    }

    const FName ShopKind = FShopFront::PlaceKind();
    const FVector ShopLoc(100.0, 200.0, 0.0);

    const int32 Handle = Registry->RegisterPlace(ShopKind, ShopLoc, /*Capacity=*/0);
    TestTrue(TEXT("got a handle"), Handle != 0);
    TestEqual(TEXT("one shop registered"), Registry->CountOfKind(ShopKind), 1);

    const FGTCPlaceQueryResult Found = Registry->FindNearest(ShopKind, FVector::ZeroVector);
    TestTrue(TEXT("query valid"), Found.bValid);
    TestFalse(TEXT("resolved a real POI, not synthesized"), Found.bSynthesized);
    TestTrue(TEXT("nearest shop is the registered one"), Found.Location.Equals(ShopLoc));

    Registry->UnregisterPlace(Handle);
    TestEqual(TEXT("no shops after unregister"), Registry->CountOfKind(ShopKind), 0);

    const FGTCPlaceQueryResult AfterRemoval = Registry->FindNearest(ShopKind, FVector::ZeroVector);
    TestTrue(TEXT("still valid (synthesized fallback)"), AfterRemoval.bValid);
    TestTrue(TEXT("synthesized once no authored shop remains"), AfterRemoval.bSynthesized);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
