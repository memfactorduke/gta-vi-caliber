// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../WeaponInventory.h"
#include "../../../Tests/GtcTestTolerances.h"

// Each test maps 1:1 to an assertion in the the reference reference behavior
// game/tests/unit/test_weapon_inventory.gd. Prefix GTC.Weapons.Core.WeaponInventory.
// Integer state throughout — no float tolerance needed; Eps is included only for
// the shared-tolerance convention.
namespace
{
    using GtcTest::Eps;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvStartsWithFistsTest,
    "GTC.Weapons.Core.WeaponInventory.StartsWithOnlyFistsEquipped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvStartsWithFistsTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    TestEqual(TEXT("count == 1"), Inv.WeaponCount(), 1);
    TestTrue(TEXT("has fists"), Inv.HasWeapon(WeaponInventory::UnarmedId));
    TestEqual(TEXT("current == fists"), Inv.CurrentId(), WeaponInventory::UnarmedId);
    TestFalse(TEXT("cannot fire"), Inv.CanFire());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvAddWeaponTest,
    "GTC.Weapons.Core.WeaponInventory.AddWeaponOwnsEquipsAndCurrents",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvAddWeaponTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    Inv.AddWeapon(TEXT("pistol"), 12, 36);
    TestTrue(TEXT("has pistol"), Inv.HasWeapon(TEXT("pistol")));
    TestEqual(TEXT("count == 2"), Inv.WeaponCount(), 2);
    TestTrue(TEXT("equip pistol"), Inv.Equip(TEXT("pistol")));
    TestEqual(TEXT("current == pistol"), Inv.CurrentId(), FString(TEXT("pistol")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvNewWeaponFullMagTest,
    "GTC.Weapons.Core.WeaponInventory.NewWeaponStartsWithFullMag",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvNewWeaponFullMagTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    Inv.AddWeapon(TEXT("pistol"), 12, 36);
    Inv.Equip(TEXT("pistol"));
    TestEqual(TEXT("mag == 12"), Inv.AmmoInMag(), 12);
    TestEqual(TEXT("reserve == 36"), Inv.ReserveAmmo(), 36);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvDuplicateTopsUpTest,
    "GTC.Weapons.Core.WeaponInventory.AddDuplicateTopsUpReserveNotSecondSlot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvDuplicateTopsUpTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    Inv.AddWeapon(TEXT("pistol"), 12, 36);
    Inv.AddWeapon(TEXT("pistol"), 99, 14);
    Inv.Equip(TEXT("pistol"));
    // Still one pistol slot; reserve summed; original mag_size kept.
    TestEqual(TEXT("count == 2"), Inv.WeaponCount(), 2);
    TestEqual(TEXT("reserve == 50"), Inv.ReserveAmmo(), 50);
    TestEqual(TEXT("mag == 12"), Inv.AmmoInMag(), 12);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvAddAmmoTest,
    "GTC.Weapons.Core.WeaponInventory.AddAmmoIncreasesReserve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvAddAmmoTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    Inv.AddWeapon(TEXT("rifle"), 30, 30);
    Inv.Equip(TEXT("rifle"));
    Inv.AddAmmo(TEXT("rifle"), 60);
    TestEqual(TEXT("reserve == 90"), Inv.ReserveAmmo(), 90);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvAddAmmoUnownedTest,
    "GTC.Weapons.Core.WeaponInventory.AddAmmoUnownedIsNoop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvAddAmmoUnownedTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    Inv.AddAmmo(TEXT("ghost"), 10);
    TestFalse(TEXT("not owned"), Inv.HasWeapon(TEXT("ghost")));
    TestEqual(TEXT("count == 1"), Inv.WeaponCount(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvEquipUnownedTest,
    "GTC.Weapons.Core.WeaponInventory.EquipUnownedFailsAndKeepsCurrent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvEquipUnownedTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    Inv.AddWeapon(TEXT("pistol"), 12, 12);
    Inv.Equip(TEXT("pistol"));
    const bool bOk = Inv.Equip(TEXT("rocket"));
    TestFalse(TEXT("equip unowned fails"), bOk);
    TestEqual(TEXT("current still pistol"), Inv.CurrentId(), FString(TEXT("pistol")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvOwnedIdsOrderTest,
    "GTC.Weapons.Core.WeaponInventory.OwnedIdsInWheelOrder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvOwnedIdsOrderTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    Inv.AddWeapon(TEXT("pistol"), 12, 0);
    Inv.AddWeapon(TEXT("rifle"), 30, 0);
    const TArray<FString> Ids = Inv.OwnedIds();
    TestEqual(TEXT("size == 3"), Ids.Num(), 3);
    TestEqual(TEXT("[0] fists"), Ids[0], WeaponInventory::UnarmedId);
    TestEqual(TEXT("[1] pistol"), Ids[1], FString(TEXT("pistol")));
    TestEqual(TEXT("[2] rifle"), Ids[2], FString(TEXT("rifle")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvOwnedIdsCopyTest,
    "GTC.Weapons.Core.WeaponInventory.OwnedIdsIsACopy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvOwnedIdsCopyTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    Inv.AddWeapon(TEXT("pistol"), 12, 0);
    TArray<FString> Ids = Inv.OwnedIds();
    Ids.Empty();
    TestEqual(TEXT("inventory unchanged"), Inv.WeaponCount(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvNextWeaponWrapsTest,
    "GTC.Weapons.Core.WeaponInventory.NextWeaponCyclesAndWraps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvNextWeaponWrapsTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    Inv.AddWeapon(TEXT("pistol"), 12, 0);
    Inv.AddWeapon(TEXT("rifle"), 30, 0);
    // fists -> pistol -> rifle -> fists
    const FString A = Inv.NextWeapon();
    const FString B = Inv.NextWeapon();
    const FString C = Inv.NextWeapon();
    TestEqual(TEXT("a == pistol"), A, FString(TEXT("pistol")));
    TestEqual(TEXT("b == rifle"), B, FString(TEXT("rifle")));
    TestEqual(TEXT("c == fists"), C, WeaponInventory::UnarmedId);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvPrevWeaponWrapsTest,
    "GTC.Weapons.Core.WeaponInventory.PreviousWeaponCyclesAndWraps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvPrevWeaponWrapsTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    Inv.AddWeapon(TEXT("pistol"), 12, 0);
    Inv.AddWeapon(TEXT("rifle"), 30, 0);
    // from fists, previous wraps to rifle -> pistol -> fists
    const FString A = Inv.PreviousWeapon();
    const FString B = Inv.PreviousWeapon();
    const FString C = Inv.PreviousWeapon();
    TestEqual(TEXT("a == rifle"), A, FString(TEXT("rifle")));
    TestEqual(TEXT("b == pistol"), B, FString(TEXT("pistol")));
    TestEqual(TEXT("c == fists"), C, WeaponInventory::UnarmedId);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvSwitchPreservesStateTest,
    "GTC.Weapons.Core.WeaponInventory.SwitchingPreservesPerWeaponState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvSwitchPreservesStateTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    Inv.AddWeapon(TEXT("pistol"), 12, 12);
    Inv.AddWeapon(TEXT("rifle"), 30, 30);
    Inv.Equip(TEXT("pistol"));
    Inv.Fire();
    Inv.Fire();  // pistol mag now 10
    Inv.Equip(TEXT("rifle"));
    Inv.Fire();  // rifle mag now 29
    const int32 RifleMag = Inv.AmmoInMag();
    Inv.Equip(TEXT("pistol"));
    TestEqual(TEXT("rifle mag == 29"), RifleMag, 29);
    TestEqual(TEXT("pistol mag == 10"), Inv.AmmoInMag(), 10);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvFireDecrementsTest,
    "GTC.Weapons.Core.WeaponInventory.FireDecrementsMagAndReturnsTrue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvFireDecrementsTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    Inv.AddWeapon(TEXT("pistol"), 12, 0);
    Inv.Equip(TEXT("pistol"));
    const bool bShot = Inv.Fire();
    TestTrue(TEXT("shot"), bShot);
    TestEqual(TEXT("mag == 11"), Inv.AmmoInMag(), 11);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvFireEmptyMagTest,
    "GTC.Weapons.Core.WeaponInventory.FireEmptyMagReturnsFalseNoConsume",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvFireEmptyMagTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    Inv.AddWeapon(TEXT("pistol"), 1, 5);
    Inv.Equip(TEXT("pistol"));
    Inv.Fire();  // mag 1 -> 0
    const bool bShot = Inv.Fire();
    TestFalse(TEXT("no shot"), bShot);
    TestEqual(TEXT("mag == 0"), Inv.AmmoInMag(), 0);
    TestEqual(TEXT("reserve == 5"), Inv.ReserveAmmo(), 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvFistsCannotFireTest,
    "GTC.Weapons.Core.WeaponInventory.FistsCannotFire",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvFistsCannotFireTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    TestFalse(TEXT("cannot fire"), Inv.CanFire());
    TestFalse(TEXT("fire"), Inv.Fire());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvCanFireReflectsMagTest,
    "GTC.Weapons.Core.WeaponInventory.CanFireReflectsMag",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvCanFireReflectsMagTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    Inv.AddWeapon(TEXT("pistol"), 1, 0);
    Inv.Equip(TEXT("pistol"));
    const bool bBefore = Inv.CanFire();
    Inv.Fire();
    TestTrue(TEXT("before"), bBefore);
    TestFalse(TEXT("after empty"), Inv.CanFire());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvReloadFillsTest,
    "GTC.Weapons.Core.WeaponInventory.ReloadFillsMagFromReserve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvReloadFillsTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    Inv.AddWeapon(TEXT("pistol"), 12, 30);
    Inv.Equip(TEXT("pistol"));
    for (int32 i = 0; i < 5; ++i)
    {
        Inv.Fire();  // mag 12 -> 7
    }
    const int32 Loaded = Inv.Reload();
    TestEqual(TEXT("loaded == 5"), Loaded, 5);
    TestEqual(TEXT("mag == 12"), Inv.AmmoInMag(), 12);
    TestEqual(TEXT("reserve == 25"), Inv.ReserveAmmo(), 25);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvReloadPartialTest,
    "GTC.Weapons.Core.WeaponInventory.ReloadPartialWhenReserveLow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvReloadPartialTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    Inv.AddWeapon(TEXT("pistol"), 12, 0);
    Inv.Equip(TEXT("pistol"));
    Inv.AddAmmo(TEXT("pistol"), 3);
    for (int32 i = 0; i < 10; ++i)
    {
        Inv.Fire();  // mag 12 -> 2, needs 10
    }
    const int32 Loaded = Inv.Reload();
    TestEqual(TEXT("loaded == 3"), Loaded, 3);
    TestEqual(TEXT("mag == 5"), Inv.AmmoInMag(), 5);
    TestEqual(TEXT("reserve == 0"), Inv.ReserveAmmo(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvReloadEmptyReserveTest,
    "GTC.Weapons.Core.WeaponInventory.ReloadEmptyReserveLoadsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvReloadEmptyReserveTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    Inv.AddWeapon(TEXT("pistol"), 12, 0);
    Inv.Equip(TEXT("pistol"));
    Inv.Fire();  // mag 11, reserve 0
    const int32 Loaded = Inv.Reload();
    TestEqual(TEXT("loaded == 0"), Loaded, 0);
    TestEqual(TEXT("mag == 11"), Inv.AmmoInMag(), 11);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvReloadFullMagNoopTest,
    "GTC.Weapons.Core.WeaponInventory.ReloadFullMagIsNoop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvReloadFullMagNoopTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    Inv.AddWeapon(TEXT("pistol"), 12, 30);
    Inv.Equip(TEXT("pistol"));
    const int32 Loaded = Inv.Reload();
    TestEqual(TEXT("loaded == 0"), Loaded, 0);
    TestEqual(TEXT("mag == 12"), Inv.AmmoInMag(), 12);
    TestEqual(TEXT("reserve == 30"), Inv.ReserveAmmo(), 30);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWInvReserveZeroForFistsTest,
    "GTC.Weapons.Core.WeaponInventory.ReserveAmmoZeroForFists",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWInvReserveZeroForFistsTest::RunTest(const FString& Parameters)
{
    WeaponInventory Inv;
    TestEqual(TEXT("reserve == 0"), Inv.ReserveAmmo(), 0);
    TestEqual(TEXT("reload == 0"), Inv.Reload(), 0);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
