// Copyright (c) 2026 GTC contributors

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../BuildingUse.h"

// Parity oracle: game/tests/unit/test_building_use.gd. Each It(...) maps 1:1 to
// one Godot test function, asserting the SAME conditions as the oracle (pure
// exact string/int/bool comparisons, no added tolerances).
BEGIN_DEFINE_SPEC(FBuildingUseSpec, "GTC.World.Buildings.BuildingUse",
    EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)
END_DEFINE_SPEC(FBuildingUseSpec)

void FBuildingUseSpec::Define()
{
    // test_shop_kinds_are_shops
    It("treats shop kinds as shops", [this]()
    {
        TestTrue(TEXT("retail is a shop"), FBuildingUse::IsShop(TEXT("retail")));
        TestTrue(TEXT("commercial is a shop"), FBuildingUse::IsShop(TEXT("commercial")));
        TestTrue(TEXT("supermarket is a shop"), FBuildingUse::IsShop(TEXT("supermarket")));
    });

    // test_non_shop_public_kinds_are_not_shops
    It("does not treat non-shop public kinds as shops", [this]()
    {
        TestFalse(TEXT("office is not a shop"), FBuildingUse::IsShop(TEXT("office")));
        TestFalse(TEXT("hotel is not a shop"), FBuildingUse::IsShop(TEXT("hotel")));
        TestFalse(TEXT("church is not a shop"), FBuildingUse::IsShop(TEXT("church")));
    });

    // test_unknown_and_empty_kind_is_not_a_shop
    It("does not treat unknown or empty kinds as shops", [this]()
    {
        TestFalse(TEXT("empty is not a shop"), FBuildingUse::IsShop(TEXT("")));
        TestFalse(TEXT("apartments is not a shop"), FBuildingUse::IsShop(TEXT("apartments")));
    });

    // test_catalogue_for_shop_is_non_empty
    It("returns a non-empty catalogue for a shop", [this]()
    {
        TestTrue(TEXT("catalogue has entries"), FBuildingUse::CatalogueFor(TEXT("retail")).Num() > 0);
    });

    // test_catalogue_entries_are_priced_dicts
    It("returns priced catalogue entries", [this]()
    {
        const TArray<FShopItem> Catalogue = FBuildingUse::CatalogueFor(TEXT("retail"));
        const FShopItem& First = Catalogue[0];
        // Godot checks first.has("id") and first.has("price"); the typed FShopItem
        // always carries both, so we assert the entry is well-formed: a non-empty
        // id and a valid (non-negative) price.
        TestTrue(TEXT("first entry has an id and a price"),
            !First.Id.IsEmpty() && First.Price >= 0);
    });
}

#endif // WITH_AUTOMATION_TESTS
