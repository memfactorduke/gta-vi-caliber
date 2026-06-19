// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../WeaponFireController.h"
#include "../../Core/WeaponStats.h"

// Trigger-cadence tests for the GTA-style fire controller (semi-auto = one shot
// per pull, automatic = repeats at fire rate while held). Prefix
// GTC.Weapons.Firing.FireController.
namespace
{
    // 10 rounds/sec -> 0.1s cooldown, so cadence math uses round numbers.
    FWeaponStats SemiStats()
    {
        FWeaponStats S;
        S.bAutomatic = false;
        S.FireRate = 10.0;
        S.MagSize = 5;
        S.ReserveMax = 10;
        S.ReloadTime = 2.0;
        return S;
    }

    FWeaponStats AutoStats()
    {
        FWeaponStats S = SemiStats();
        S.bAutomatic = true;
        return S;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFireControllerReleasedTriggerNeverFiresTest,
    "GTC.Weapons.Firing.FireController.ReleasedTriggerNeverFires",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFireControllerReleasedTriggerNeverFiresTest::RunTest(const FString& Parameters)
{
    FWeaponFireController C(SemiStats());
    TestFalse(TEXT("no shot while trigger released"), C.TryFire());
    TestEqual(TEXT("ammo untouched"), C.AmmoInMag(), 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFireControllerSemiOneShotPerPullTest,
    "GTC.Weapons.Firing.FireController.SemiOneShotPerPull",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFireControllerSemiOneShotPerPullTest::RunTest(const FString& Parameters)
{
    FWeaponFireController C(SemiStats());
    C.PressTrigger();
    TestTrue(TEXT("first pull fires"), C.TryFire());
    TestEqual(TEXT("one round spent"), C.AmmoInMag(), 4);

    // Even after the cooldown fully elapses, a held semi-auto must NOT auto-repeat.
    C.Tick(1.0);
    TestFalse(TEXT("held semi-auto does not repeat"), C.TryFire());
    TestEqual(TEXT("still one round spent"), C.AmmoInMag(), 4);

    // Releasing and pulling again re-arms the next shot.
    C.ReleaseTrigger();
    C.PressTrigger();
    TestTrue(TEXT("second pull fires"), C.TryFire());
    TestEqual(TEXT("two rounds spent"), C.AmmoInMag(), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFireControllerAutoRepeatsWhileHeldTest,
    "GTC.Weapons.Firing.FireController.AutoRepeatsWhileHeld",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFireControllerAutoRepeatsWhileHeldTest::RunTest(const FString& Parameters)
{
    FWeaponFireController C(AutoStats());
    C.PressTrigger();
    TestTrue(TEXT("first shot fires"), C.TryFire());

    // Immediately after, the weapon is on cooldown (0.1s) — no shot yet.
    TestFalse(TEXT("on cooldown, no shot"), C.TryFire());

    // Advance past the cooldown WITHOUT releasing the trigger — auto fires again.
    C.Tick(0.1);
    TestTrue(TEXT("auto repeats while held"), C.TryFire());
    TestEqual(TEXT("two rounds spent"), C.AmmoInMag(), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFireControllerEmptyMagThenReloadTest,
    "GTC.Weapons.Firing.FireController.EmptyMagThenReload",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFireControllerEmptyMagThenReloadTest::RunTest(const FString& Parameters)
{
    FWeaponFireController C(AutoStats());
    C.PressTrigger();

    // Burn the whole magazine (5 rounds), ticking past cooldown each time.
    for (int32 i = 0; i < 5; ++i)
    {
        TestTrue(TEXT("round fires"), C.TryFire());
        C.Tick(0.1);
    }
    TestEqual(TEXT("mag empty"), C.AmmoInMag(), 0);
    TestFalse(TEXT("empty mag cannot fire"), C.TryFire());

    // Reload from reserve and confirm we can fire again.
    TestTrue(TEXT("reload starts"), C.Reload());
    TestTrue(TEXT("reloading"), C.IsReloading());
    C.Tick(2.0);
    TestFalse(TEXT("reload finished"), C.IsReloading());
    TestEqual(TEXT("mag refilled"), C.AmmoInMag(), 5);
    TestTrue(TEXT("fires after reload"), C.TryFire());
    return true;
}

#endif // WITH_AUTOMATION_TESTS
