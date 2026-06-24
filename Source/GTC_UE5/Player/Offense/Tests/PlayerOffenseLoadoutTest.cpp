// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../PlayerOffenseLoadout.h"

// ============================================================================
// FPlayerOffenseLoadout — ammo + cooldown pacing for the player's throw/melee.
// Pure-core, no engine deps; these assert the resource + rate limits that turn the
// throw/melee verbs into finite, rate-limited abilities (closing the chain-stun and
// takedown-spam exploits).
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOffenseLoadoutStartingAmmoTest,
    "GTC.Player.Offense.Loadout.StartingAmmo",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOffenseLoadoutStartingAmmoTest::RunTest(const FString& Parameters)
{
    FPlayerOffenseLoadout L;
    TestEqual(TEXT("flashbang start"), L.GetAmmo(EThrowableKind::Flashbang), 2);
    TestEqual(TEXT("grenade start"), L.GetAmmo(EThrowableKind::Grenade), 3);
    TestEqual(TEXT("molotov start"), L.GetAmmo(EThrowableKind::Molotov), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOffenseLoadoutThrowConsumesAndCoolsTest,
    "GTC.Player.Offense.Loadout.ThrowConsumesAndCools",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOffenseLoadoutThrowConsumesAndCoolsTest::RunTest(const FString& Parameters)
{
    FPlayerOffenseLoadout L;
    // First throw at t=0 succeeds and consumes one.
    TestTrue(TEXT("throw at 0 ok"), L.TryThrow(EThrowableKind::Flashbang, 0.0));
    TestEqual(TEXT("flashbang now 1"), L.GetAmmo(EThrowableKind::Flashbang), 1);
    // A second throw inside the cooldown (1.2s) is refused — the chain-stun fix.
    TestFalse(TEXT("throw at 0.5 blocked by cooldown"), L.TryThrow(EThrowableKind::Flashbang, 0.5));
    TestEqual(TEXT("ammo unchanged after blocked throw"), L.GetAmmo(EThrowableKind::Flashbang), 1);
    // After the cooldown elapses it works again and empties the stock.
    TestTrue(TEXT("throw at 1.3 ok"), L.TryThrow(EThrowableKind::Flashbang, 1.3));
    TestEqual(TEXT("flashbang now 0"), L.GetAmmo(EThrowableKind::Flashbang), 0);
    // Out of ammo: refused even though the cooldown is clear.
    TestFalse(TEXT("throw at 10 blocked by empty"), L.TryThrow(EThrowableKind::Flashbang, 10.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOffenseLoadoutAddAmmoClampsTest,
    "GTC.Player.Offense.Loadout.AddAmmoClamps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOffenseLoadoutAddAmmoClampsTest::RunTest(const FString& Parameters)
{
    FPlayerOffenseLoadout L;
    // Grenade max is 5; start 3, add 10 → clamps to 5.
    TestEqual(TEXT("grenade clamps to max"), L.AddAmmo(EThrowableKind::Grenade, 10), 5);
    TestEqual(TEXT("grenade is 5"), L.GetAmmo(EThrowableKind::Grenade), 5);
    // Negative add is ignored (no underflow).
    TestEqual(TEXT("negative add ignored"), L.AddAmmo(EThrowableKind::Grenade, -3), 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOffenseLoadoutMeleeCooldownTest,
    "GTC.Player.Offense.Loadout.MeleeCooldown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOffenseLoadoutMeleeCooldownTest::RunTest(const FString& Parameters)
{
    FPlayerOffenseLoadout L;
    TestTrue(TEXT("melee at 0 ok"), L.TryMelee(0.0));
    // Inside the 0.8s cooldown → refused (no walking-down-the-line back-stab spam).
    TestFalse(TEXT("melee at 0.5 blocked"), L.TryMelee(0.5));
    TestTrue(TEXT("melee at 0.9 ok"), L.TryMelee(0.9));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOffenseLoadoutSaveRoundTripTest,
    "GTC.Player.Offense.Loadout.SaveRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOffenseLoadoutSaveRoundTripTest::RunTest(const FString& Parameters)
{
    FPlayerOffenseLoadout L;
    L.TryThrow(EThrowableKind::Grenade, 0.0); // 3 -> 2
    int F = 0, G = 0, M = 0;
    L.SerializeAmmo(F, G, M);
    TestEqual(TEXT("serialize flashbang"), F, 2);
    TestEqual(TEXT("serialize grenade"), G, 2);
    TestEqual(TEXT("serialize molotov"), M, 2);

    // Restore into a fresh loadout round-trips, and out-of-range values clamp to [0, cap].
    FPlayerOffenseLoadout L2;
    L2.RestoreAmmo(F, G, M);
    TestEqual(TEXT("restore grenade"), L2.GetAmmo(EThrowableKind::Grenade), 2);
    L2.RestoreAmmo(-5, 99, 3);
    TestEqual(TEXT("clamp negative to 0"), L2.GetAmmo(EThrowableKind::Flashbang), 0);
    TestEqual(TEXT("clamp over-cap to 5"), L2.GetAmmo(EThrowableKind::Grenade), 5);
    TestEqual(TEXT("at-cap kept"), L2.GetAmmo(EThrowableKind::Molotov), 3);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
