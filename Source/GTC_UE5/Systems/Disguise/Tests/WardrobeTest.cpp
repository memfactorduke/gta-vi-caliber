// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../Wardrobe.h"
#include "../Disguise.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to a test_* func in the Godot parity oracle
// game/tests/unit/test_wardrobe.gd (12 funcs). Money/counts are exact integer checks; the
// composition recognition check uses Eps. Compound `a and b` returns are split into separate
// Test* calls. The composition test (#12) drives FDisguise (the in-branch sibling), exactly
// as the oracle composes Wardrobe with Disguise.

namespace
{
    // System-prefixed helper (STANDING RULE: prefix every test-helper free function to avoid
    // unity-build ODR collisions). Builds one raw input item for the malformed-drop test.
    FWardrobe::FInputItem MakeWardrobeInput(const FString& Id, const FString& Slot, int32 Price,
                                            bool bHasId = true)
    {
        FWardrobe::FInputItem Item;
        // Godot's "no id" entry has no id key at all; we model that as an empty Id (both the
        // empty-string entry and the missing-key entry hit the same drop in Register()).
        Item.Id = bHasId ? Id : FString();
        Item.Slot = Slot;
        Item.Price = Price;
        return Item;
    }
}

// 1. test_default_catalogue_loaded
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWardrobeDefaultCatalogueLoadedTest,
    "GTC.Systems.Wardrobe.DefaultCatalogueLoaded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWardrobeDefaultCatalogueLoadedTest::RunTest(const FString& Parameters)
{
    FWardrobe W;
    TestEqual(TEXT("item_count == 6"), W.ItemCount(), 6);
    TestTrue(TEXT("has sharp_suit"), W.HasItem(TEXT("sharp_suit")));
    TestTrue(TEXT("has ski_mask"), W.HasItem(TEXT("ski_mask")));
    return true;
}

// 2. test_malformed_items_dropped
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWardrobeMalformedDroppedTest,
    "GTC.Systems.Wardrobe.MalformedItemsDropped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWardrobeMalformedDroppedTest::RunTest(const FString& Parameters)
{
    TArray<FWardrobe::FInputItem> Items;
    Items.Add(MakeWardrobeInput(TEXT("ok"), TEXT("outfit"), 100));
    Items.Add(MakeWardrobeInput(TEXT(""), TEXT("outfit"), 100));         // empty id
    Items.Add(MakeWardrobeInput(TEXT("nope"), TEXT("outfit"), 100, /*bHasId*/ false)); // no id
    Items.Add(MakeWardrobeInput(TEXT("bad_slot"), TEXT("shoes"), 100));  // unknown slot
    Items.Add(MakeWardrobeInput(TEXT("negative"), TEXT("hair"), -5));    // negative price
    Items.Add(MakeWardrobeInput(TEXT("ok"), TEXT("hair"), 200));         // duplicate id
    FWardrobe W(Items);
    TestEqual(TEXT("item_count == 1"), W.ItemCount(), 1);
    TestTrue(TEXT("has ok"), W.HasItem(TEXT("ok")));
    return true;
}

// 3. test_starters_owned_and_worn
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWardrobeStartersOwnedWornTest,
    "GTC.Systems.Wardrobe.StartersOwnedAndWorn",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWardrobeStartersOwnedWornTest::RunTest(const FString& Parameters)
{
    FWardrobe W;
    TestTrue(TEXT("owns street_casual"), W.Owns(TEXT("street_casual")));
    TestTrue(TEXT("owns buzz_cut"), W.Owns(TEXT("buzz_cut")));
    TestEqual(TEXT("worn outfit == street_casual"), W.WornIn(TEXT("outfit")), FString(TEXT("street_casual")));
    TestEqual(TEXT("worn hair == buzz_cut"), W.WornIn(TEXT("hair")), FString(TEXT("buzz_cut")));
    return true;
}

// 4. test_lookups
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWardrobeLookupsTest,
    "GTC.Systems.Wardrobe.Lookups",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWardrobeLookupsTest::RunTest(const FString& Parameters)
{
    FWardrobe W;
    TestEqual(TEXT("price sharp_suit == 1500"), W.PriceOf(TEXT("sharp_suit")), 1500);
    TestEqual(TEXT("slot sharp_suit == outfit"), W.SlotOf(TEXT("sharp_suit")), FString(TEXT("outfit")));
    TestEqual(TEXT("look sharp_suit == suit"), W.LookOf(TEXT("sharp_suit")), FString(TEXT("suit")));
    return true;
}

// 5. test_items_in_slot
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWardrobeItemsInSlotTest,
    "GTC.Systems.Wardrobe.ItemsInSlot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWardrobeItemsInSlotTest::RunTest(const FString& Parameters)
{
    FWardrobe W;
    const TArray<FString> Outfits = W.ItemsInSlot(TEXT("outfit"));
    TestTrue(TEXT("has street_casual"), Outfits.Contains(TEXT("street_casual")));
    TestTrue(TEXT("has sharp_suit"), Outfits.Contains(TEXT("sharp_suit")));
    TestFalse(TEXT("no ski_mask"), Outfits.Contains(TEXT("ski_mask")));
    return true;
}

// 6. test_buy_grants_ownership
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWardrobeBuyGrantsOwnershipTest,
    "GTC.Systems.Wardrobe.BuyGrantsOwnership",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWardrobeBuyGrantsOwnershipTest::RunTest(const FString& Parameters)
{
    FWardrobe W;
    const FWardrobeBuyResult R = W.Buy(TEXT("sharp_suit"), 5000);
    TestTrue(TEXT("success"), R.bSuccess);
    TestEqual(TEXT("cost == 1500"), R.Cost, 1500);
    TestEqual(TEXT("new_balance == 3500"), R.NewBalance, 3500);
    TestTrue(TEXT("owns sharp_suit"), W.Owns(TEXT("sharp_suit")));
    return true;
}

// 7. test_buy_already_owned_fails
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWardrobeBuyAlreadyOwnedFailsTest,
    "GTC.Systems.Wardrobe.BuyAlreadyOwnedFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWardrobeBuyAlreadyOwnedFailsTest::RunTest(const FString& Parameters)
{
    FWardrobe W;
    TestFalse(TEXT("buy owned fails"), W.Buy(TEXT("street_casual"), 5000).bSuccess);
    return true;
}

// 8. test_buy_insufficient_funds
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWardrobeBuyInsufficientFundsTest,
    "GTC.Systems.Wardrobe.BuyInsufficientFunds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWardrobeBuyInsufficientFundsTest::RunTest(const FString& Parameters)
{
    FWardrobe W;
    const FWardrobeBuyResult R = W.Buy(TEXT("sharp_suit"), 100);
    TestFalse(TEXT("not success"), R.bSuccess);
    TestEqual(TEXT("new_balance == 100"), R.NewBalance, 100);
    TestFalse(TEXT("does not own"), W.Owns(TEXT("sharp_suit")));
    return true;
}

// 9. test_wear_requires_ownership
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWardrobeWearRequiresOwnershipTest,
    "GTC.Systems.Wardrobe.WearRequiresOwnership",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWardrobeWearRequiresOwnershipTest::RunTest(const FString& Parameters)
{
    FWardrobe W;
    const bool bUnowned = W.Wear(TEXT("sharp_suit"));  // not bought yet
    W.Buy(TEXT("sharp_suit"), 5000);
    const bool bOwned = W.Wear(TEXT("sharp_suit"));
    TestFalse(TEXT("cannot wear unowned"), bUnowned);
    TestTrue(TEXT("can wear owned"), bOwned);
    TestEqual(TEXT("worn outfit == sharp_suit"), W.WornIn(TEXT("outfit")), FString(TEXT("sharp_suit")));
    return true;
}

// 10. test_take_off_clears_slot
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWardrobeTakeOffClearsSlotTest,
    "GTC.Systems.Wardrobe.TakeOffClearsSlot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWardrobeTakeOffClearsSlotTest::RunTest(const FString& Parameters)
{
    FWardrobe W;
    W.Buy(TEXT("ski_mask"), 5000);
    W.Wear(TEXT("ski_mask"));
    W.TakeOff(TEXT("mask"));
    TestEqual(TEXT("worn mask == \"\""), W.WornIn(TEXT("mask")), FString());
    TestEqual(TEXT("worn_look mask == \"\""), W.WornLook(TEXT("mask")), FString());
    return true;
}

// 11. test_worn_looks_map
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWardrobeWornLooksMapTest,
    "GTC.Systems.Wardrobe.WornLooksMap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWardrobeWornLooksMapTest::RunTest(const FString& Parameters)
{
    FWardrobe W;
    const TMap<FString, FString> Looks = W.WornLooks();
    TestEqual(TEXT("outfit look == casual"), Looks.FindRef(TEXT("outfit")), FString(TEXT("casual")));
    TestEqual(TEXT("hair look == buzz"), Looks.FindRef(TEXT("hair")), FString(TEXT("buzz")));
    return true;
}

// 12. test_changing_clothes_lowers_recognition (composition with FDisguise sibling)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWardrobeChangingClothesLowersRecognitionTest,
    "GTC.Systems.Wardrobe.ChangingClothesLowersRecognition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWardrobeChangingClothesLowersRecognitionTest::RunTest(const FString& Parameters)
{
    FWardrobe W;
    FDisguise D;
    const TMap<FString, FString> WornLooks = W.WornLooks();
    for (const TPair<FString, FString>& Pair : WornLooks)
    {
        D.SetAppearance(Pair.Key, Pair.Value);
    }
    D.LogSighting();  // cops log the starter look -> fully recognized
    const double Before = D.Recognition();
    W.Buy(TEXT("sharp_suit"), 5000);
    W.Wear(TEXT("sharp_suit"));
    D.SetAppearance(TEXT("outfit"), W.WornLook(TEXT("outfit")));  // now wearing the suit
    TestEqual(TEXT("before == 1.0"), Before, 1.0, Eps);
    TestTrue(TEXT("recognition dropped"), D.Recognition() < Before);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
