// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../WeaponLoadout.h"
#include "../../Ballistics/WeaponBallistics.h"
#include "../../../Tests/GtcTestTolerances.h"

// Each test maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_weapon_loadout.gd. Multiplier products use Eps, mirroring
// is_equal_approx. Prefix GTC.Weapons.Core.WeaponLoadout. The composition test
// reuses the merged FWeaponBallistics neighbor (Weapons/Ballistics).
namespace
{
    using GtcTest::Eps;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWLoadDefaultCatalogueTest,
    "GTC.Weapons.Core.WeaponLoadout.DefaultCatalogueLoaded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWLoadDefaultCatalogueTest::RunTest(const FString& Parameters)
{
    WeaponLoadout L;
    TestEqual(TEXT("count == 6"), L.AttachmentCount(), 6);
    TestTrue(TEXT("has scope"), L.HasAttachment(TEXT("scope")));
    TestTrue(TEXT("has suppressor"), L.HasAttachment(TEXT("suppressor")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWLoadMalformedDroppedTest,
    "GTC.Weapons.Core.WeaponLoadout.MalformedAttachmentsDropped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWLoadMalformedDroppedTest::RunTest(const FString& Parameters)
{
    TArray<WeaponLoadout::FAttachment> Entries;

    WeaponLoadout::FAttachment Ok;
    Ok.Id = TEXT("ok");
    Ok.Slot = TEXT("optic");
    Entries.Add(Ok);

    WeaponLoadout::FAttachment NoSlot;  // missing slot
    NoSlot.Id = TEXT("no_slot");
    Entries.Add(NoSlot);

    WeaponLoadout::FAttachment NoId;  // missing id
    NoId.Slot = TEXT("optic");
    Entries.Add(NoId);

    WeaponLoadout::FAttachment BadSlot;  // unknown slot
    BadSlot.Id = TEXT("bad_slot");
    BadSlot.Slot = TEXT("frame");
    Entries.Add(BadSlot);

    WeaponLoadout::FAttachment DupId;  // duplicate id
    DupId.Id = TEXT("ok");
    DupId.Slot = TEXT("grip");
    Entries.Add(DupId);

    WeaponLoadout L(Entries);
    TestEqual(TEXT("count == 1"), L.AttachmentCount(), 1);
    TestTrue(TEXT("has ok"), L.HasAttachment(TEXT("ok")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWLoadSlotLookupTest,
    "GTC.Weapons.Core.WeaponLoadout.SlotLookup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWLoadSlotLookupTest::RunTest(const FString& Parameters)
{
    WeaponLoadout L;
    TestEqual(TEXT("suppressor -> muzzle"), L.SlotOf(TEXT("suppressor")), FString(TEXT("muzzle")));
    TestEqual(TEXT("nope -> empty"), L.SlotOf(TEXT("nope")), FString());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWLoadEquipSetsSlotTest,
    "GTC.Weapons.Core.WeaponLoadout.EquipSetsSlot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWLoadEquipSetsSlotTest::RunTest(const FString& Parameters)
{
    WeaponLoadout L;
    TestTrue(TEXT("equip scope"), L.Equip(TEXT("scope")));
    TestEqual(TEXT("optic == scope"), L.EquippedIn(TEXT("optic")), FString(TEXT("scope")));
    TestTrue(TEXT("scope is equipped"), L.IsEquipped(TEXT("scope")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWLoadEquipUnknownTest,
    "GTC.Weapons.Core.WeaponLoadout.EquipUnknownFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWLoadEquipUnknownTest::RunTest(const FString& Parameters)
{
    WeaponLoadout L;
    TestFalse(TEXT("equip unknown fails"), L.Equip(TEXT("nope")));
    TestEqual(TEXT("equipped count == 0"), L.EquippedCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWLoadEquipReplacesTest,
    "GTC.Weapons.Core.WeaponLoadout.EquipReplacesSameSlot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWLoadEquipReplacesTest::RunTest(const FString& Parameters)
{
    WeaponLoadout L;
    L.Equip(TEXT("scope"));
    L.Equip(TEXT("red_dot"));  // same slot 'optic'
    TestEqual(TEXT("optic == red_dot"), L.EquippedIn(TEXT("optic")), FString(TEXT("red_dot")));
    TestFalse(TEXT("scope no longer equipped"), L.IsEquipped(TEXT("scope")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWLoadUnequipClearsTest,
    "GTC.Weapons.Core.WeaponLoadout.UnequipClearsSlot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWLoadUnequipClearsTest::RunTest(const FString& Parameters)
{
    WeaponLoadout L;
    L.Equip(TEXT("scope"));
    L.Unequip(TEXT("optic"));
    TestEqual(TEXT("optic empty"), L.EquippedIn(TEXT("optic")), FString());
    TestEqual(TEXT("equipped count == 0"), L.EquippedCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWLoadMultNeutralTest,
    "GTC.Weapons.Core.WeaponLoadout.MultNeutralWhenUnaffected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWLoadMultNeutralTest::RunTest(const FString& Parameters)
{
    WeaponLoadout L;
    L.Equip(TEXT("extended_mag"));  // only adds mag, no mults
    TestEqual(TEXT("spread mult == 1.0"), L.MultFor(TEXT("spread")), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWLoadMultProductTest,
    "GTC.Weapons.Core.WeaponLoadout.MultCombinesByProduct",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWLoadMultProductTest::RunTest(const FString& Parameters)
{
    WeaponLoadout L;
    L.Equip(TEXT("scope"));     // spread 0.6
    L.Equip(TEXT("foregrip"));  // spread 0.9
    // 0.6 * 0.9 = 0.54
    TestEqual(TEXT("spread mult == 0.54"), L.MultFor(TEXT("spread")), 0.54, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWLoadApplyMultTest,
    "GTC.Weapons.Core.WeaponLoadout.ApplyMult",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWLoadApplyMultTest::RunTest(const FString& Parameters)
{
    WeaponLoadout L;
    L.Equip(TEXT("scope"));  // range 1.25
    TestEqual(TEXT("60 * 1.25 == 75"), L.ApplyMult(60.0, TEXT("range")), 75.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWLoadMagSizeAdditiveTest,
    "GTC.Weapons.Core.WeaponLoadout.MagSizeAdditive",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWLoadMagSizeAdditiveTest::RunTest(const FString& Parameters)
{
    WeaponLoadout L;
    L.Equip(TEXT("extended_mag"));  // +12
    TestEqual(TEXT("30 + 12 == 42"), L.MagSize(30), 42);
    TestEqual(TEXT("0 + 12 == 12"), L.MagSize(0), 12);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWLoadSuppressedFlagTest,
    "GTC.Weapons.Core.WeaponLoadout.SuppressedFlag",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWLoadSuppressedFlagTest::RunTest(const FString& Parameters)
{
    WeaponLoadout L;
    const bool bBefore = L.IsSuppressed();
    L.Equip(TEXT("suppressor"));
    TestFalse(TEXT("not suppressed before"), bBefore);
    TestTrue(TEXT("suppressed after"), L.IsSuppressed());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWLoadClearStripsTest,
    "GTC.Weapons.Core.WeaponLoadout.ClearStripsAll",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWLoadClearStripsTest::RunTest(const FString& Parameters)
{
    WeaponLoadout L;
    L.Equip(TEXT("scope"));
    L.Equip(TEXT("suppressor"));
    L.Clear();
    TestEqual(TEXT("equipped count == 0"), L.EquippedCount(), 0);
    TestFalse(TEXT("not suppressed"), L.IsSuppressed());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWLoadScopeExtendsRangeTest,
    "GTC.Weapons.Core.WeaponLoadout.ScopeExtendsEffectiveRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWLoadScopeExtendsRangeTest::RunTest(const FString& Parameters)
{
    // Composition: a scope's range multiplier widens FWeaponBallistics' falloff
    // window, so the same shot at distance lands harder.
    WeaponLoadout L;
    L.Equip(TEXT("scope"));  // range 1.25
    const double Dist = 50.0;
    const double Bare = FWeaponBallistics::EffectiveDamage(30.0, Dist, TEXT("torso"), 20.0, 60.0, 0.2);
    const double Scoped = FWeaponBallistics::EffectiveDamage(
        30.0, Dist, TEXT("torso"), L.ApplyMult(20.0, TEXT("range")), L.ApplyMult(60.0, TEXT("range")), 0.2);
    TestTrue(TEXT("scoped > bare"), Scoped > Bare);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
