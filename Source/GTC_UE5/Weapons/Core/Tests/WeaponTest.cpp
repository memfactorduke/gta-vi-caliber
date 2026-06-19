// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../Weapon.h"
#include "../WeaponStats.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

// Each test maps 1:1 to an assertion in the the reference reference behavior
// game/tests/unit/test_weapon.gd. Float compares use Eps, mirroring
// is_equal_approx. Prefix GTC.Weapons.Core.Weapon.
namespace
{
    using GtcTest::Eps;

    // A small, predictable stats block so assertions use round numbers. Mirrors
    // test_weapon.gd::_stats().
    FWeaponStats WeaponTestStats()
    {
        FWeaponStats S;
        S.FireRate = 10.0;  // 0.1s cooldown
        S.MagSize = 5;
        S.ReserveMax = 10;
        S.ReloadTime = 2.0;
        S.BaseSpread = 0.01;
        S.SpreadPerShot = 0.05;
        S.MaxSpread = 0.20;
        S.SpreadRecovery = 0.10;
        return S;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponStartsFullTest,
    "GTC.Weapons.Core.Weapon.StartsFull",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponStartsFullTest::RunTest(const FString& Parameters)
{
    FWeapon W(WeaponTestStats());
    TestEqual(TEXT("ammo == 5"), W.Ammo, 5);
    TestEqual(TEXT("reserve == 10"), W.Reserve, 10);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponStartReserveOverrideTest,
    "GTC.Weapons.Core.Weapon.StartReserveOverride",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponStartReserveOverrideTest::RunTest(const FString& Parameters)
{
    FWeapon W(WeaponTestStats(), 3);
    TestEqual(TEXT("reserve == 3"), W.Reserve, 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponFireConsumesRoundTest,
    "GTC.Weapons.Core.Weapon.FireConsumesARound",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponFireConsumesRoundTest::RunTest(const FString& Parameters)
{
    FWeapon W(WeaponTestStats());
    const bool bFired = W.Fire();
    TestTrue(TEXT("fired"), bFired);
    TestEqual(TEXT("ammo == 4"), W.Ammo, 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponCannotFireTwiceTest,
    "GTC.Weapons.Core.Weapon.CannotFireTwiceWithoutCooldown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponCannotFireTwiceTest::RunTest(const FString& Parameters)
{
    FWeapon W(WeaponTestStats());
    W.Fire();
    TestFalse(TEXT("can_fire"), W.CanFire());
    TestFalse(TEXT("fire"), W.Fire());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponCooldownClearsTest,
    "GTC.Weapons.Core.Weapon.CooldownClearsAfterTick",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponCooldownClearsTest::RunTest(const FString& Parameters)
{
    FWeapon W(WeaponTestStats());
    W.Fire();
    W.Tick(0.1);
    TestTrue(TEXT("can_fire after tick"), W.CanFire());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponEmptyMagCannotFireTest,
    "GTC.Weapons.Core.Weapon.EmptyMagCannotFire",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponEmptyMagCannotFireTest::RunTest(const FString& Parameters)
{
    FWeapon W(WeaponTestStats());
    for (int32 i = 0; i < 5; ++i)
    {
        W.Fire();
        W.Tick(0.1);
    }
    TestEqual(TEXT("ammo == 0"), W.Ammo, 0);
    TestFalse(TEXT("cannot fire"), W.CanFire());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponReloadRefillsTest,
    "GTC.Weapons.Core.Weapon.ReloadRefillsFromReserve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponReloadRefillsTest::RunTest(const FString& Parameters)
{
    FWeapon W(WeaponTestStats());
    for (int32 i = 0; i < 5; ++i)
    {
        W.Fire();
        W.Tick(0.1);
    }
    const bool bStarted = W.StartReload();
    W.Tick(2.0);
    TestTrue(TEXT("started"), bStarted);
    TestEqual(TEXT("ammo == 5"), W.Ammo, 5);
    TestEqual(TEXT("reserve == 5"), W.Reserve, 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponPartialReloadTest,
    "GTC.Weapons.Core.Weapon.PartialReloadOnlyTakesWhatIsNeeded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponPartialReloadTest::RunTest(const FString& Parameters)
{
    FWeapon W(WeaponTestStats());
    W.Fire();  // one round gone
    W.Tick(0.1);
    W.StartReload();
    W.Tick(2.0);
    // Only 1 round needed to top up the mag of 5.
    TestEqual(TEXT("ammo == 5"), W.Ammo, 5);
    TestEqual(TEXT("reserve == 9"), W.Reserve, 9);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponCannotReloadFullMagTest,
    "GTC.Weapons.Core.Weapon.CannotReloadFullMag",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponCannotReloadFullMagTest::RunTest(const FString& Parameters)
{
    FWeapon W(WeaponTestStats());
    TestFalse(TEXT("no reload on full mag"), W.StartReload());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponCannotReloadNoReserveTest,
    "GTC.Weapons.Core.Weapon.CannotReloadWithoutReserve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponCannotReloadNoReserveTest::RunTest(const FString& Parameters)
{
    FWeapon W(WeaponTestStats(), 0);
    W.Fire();
    W.Tick(0.1);
    TestFalse(TEXT("no reload without reserve"), W.StartReload());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponCannotFireWhileReloadingTest,
    "GTC.Weapons.Core.Weapon.CannotFireWhileReloading",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponCannotFireWhileReloadingTest::RunTest(const FString& Parameters)
{
    FWeapon W(WeaponTestStats());
    W.Fire();
    W.Tick(0.1);
    W.StartReload();
    TestFalse(TEXT("cannot fire mid-reload"), W.CanFire());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponReloadClampsReserveTest,
    "GTC.Weapons.Core.Weapon.ReloadClampsToAvailableReserve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponReloadClampsReserveTest::RunTest(const FString& Parameters)
{
    FWeapon W(WeaponTestStats(), 2);
    for (int32 i = 0; i < 5; ++i)
    {
        W.Fire();
        W.Tick(0.1);
    }
    W.StartReload();
    W.Tick(2.0);
    // Only 2 spare rounds exist, so the mag of 5 only gets 2.
    TestEqual(TEXT("ammo == 2"), W.Ammo, 2);
    TestEqual(TEXT("reserve == 0"), W.Reserve, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponSpreadGrowsTest,
    "GTC.Weapons.Core.Weapon.SpreadGrowsOnFire",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponSpreadGrowsTest::RunTest(const FString& Parameters)
{
    FWeapon W(WeaponTestStats());
    const double Before = W.Spread;
    W.Fire();
    TestTrue(TEXT("spread grew"), W.Spread > Before);
    TestEqual(TEXT("spread == 0.06"), W.Spread, 0.06, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponSpreadClampsTest,
    "GTC.Weapons.Core.Weapon.SpreadClampsToMax",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponSpreadClampsTest::RunTest(const FString& Parameters)
{
    FWeapon W(WeaponTestStats());
    for (int32 i = 0; i < 20; ++i)
    {
        W.Fire();
        W.Tick(0.1);
    }
    TestTrue(TEXT("spread <= max"), W.Spread <= 0.20 + 0.0001);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWeaponSpreadRecoversTest,
    "GTC.Weapons.Core.Weapon.SpreadRecoversTowardBase",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponSpreadRecoversTest::RunTest(const FString& Parameters)
{
    FWeapon W(WeaponTestStats());
    W.Fire();  // spread 0.06
    for (int32 i = 0; i < 100; ++i)
    {
        W.Tick(0.1);
    }
    TestEqual(TEXT("spread back to base 0.01"), W.Spread, 0.01, Eps);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
