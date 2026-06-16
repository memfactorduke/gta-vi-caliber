// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../GtcDamageRouting.h"
#include "../../Stats/PlayerStats.h"
#include "../../Health/PlayerHealthModel.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// ============================================================================
// GTC.Player.DamageRouting — behavior tests for the W3 armor-ownership
// resolution (docs/W3_WIRING_NOTES.md, commit f0c0c36): the SINGLE damage
// entry point GtcDamage::ApplyToPlayer drains the SOLE stats armor pool first,
// then routes the overflow into a HEALTH-ONLY model (FPlayerHealthModel built
// with ArmorMax=0). Confirms damage soaks armor exactly once (no double-soak,
// no second pool).
// ============================================================================

namespace
{
    // A health-only model: ArmorMax=0 neutralizes its armor pool per the
    // resolution. Health 100 by construction.
    FPlayerHealthModel MakeHealthOnly()
    {
        return FPlayerHealthModel(/*Maximum=*/100.0, /*Regen=*/10.0, /*Delay=*/5.0, /*ArmorMax=*/0.0);
    }

    FPlayerStats MakeStats(double ArmorValue)
    {
        FPlayerStats S;
        S.Armor = ArmorValue;
        return S;
    }
}

// --- (a) Stats armor soaks first --------------------------------------------
// stats armor 50, health 100; apply 30 -> stats armor 20, health 100.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcDamageRoutingArmorSoaksFirstTest,
    "GTC.Player.DamageRouting.ArmorSoaksFirst",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcDamageRoutingArmorSoaksFirstTest::RunTest(const FString& Parameters)
{
    FPlayerStats Stats = MakeStats(50.0);
    FPlayerHealthModel Health = MakeHealthOnly();

    const double Overflow = GtcDamage::ApplyToPlayer(Stats, Health, 30.0);

    TestTrue(TEXT("overflow == 0 (armor fully soaked 30)"), FMath::Abs(Overflow) < Eps);
    TestTrue(TEXT("stats armor == 20"), FMath::Abs(Stats.Armor - 20.0) < Eps);
    TestTrue(TEXT("health == 100 (untouched)"), FMath::Abs(Health.Health - 100.0) < Eps);
    return true;
}

// --- (b) Overflow reaches health --------------------------------------------
// stats armor 50, health 100; apply 80 -> stats armor 0, health 70.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcDamageRoutingOverflowReachesHealthTest,
    "GTC.Player.DamageRouting.OverflowReachesHealth",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcDamageRoutingOverflowReachesHealthTest::RunTest(const FString& Parameters)
{
    FPlayerStats Stats = MakeStats(50.0);
    FPlayerHealthModel Health = MakeHealthOnly();

    const double Overflow = GtcDamage::ApplyToPlayer(Stats, Health, 80.0);

    TestTrue(TEXT("overflow == 30 (80 - 50 armor)"), FMath::Abs(Overflow - 30.0) < Eps);
    TestTrue(TEXT("stats armor == 0 (drained)"), FMath::Abs(Stats.Armor) < Eps);
    TestTrue(TEXT("health == 70 (100 - 30 overflow)"), FMath::Abs(Health.Health - 70.0) < Eps);
    return true;
}

// --- (c) Never a second pool ------------------------------------------------
// stats armor 0, health 100; apply 25 -> health drops by exactly 25, and the
// health model's own Armor stays 0 throughout (no second soak).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcDamageRoutingNeverSecondPoolTest,
    "GTC.Player.DamageRouting.NeverSecondPool",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcDamageRoutingNeverSecondPoolTest::RunTest(const FString& Parameters)
{
    FPlayerStats Stats = MakeStats(0.0);
    FPlayerHealthModel Health = MakeHealthOnly();

    TestTrue(TEXT("health-model armor starts 0"), FMath::Abs(Health.Armor) < Eps);

    const double Overflow = GtcDamage::ApplyToPlayer(Stats, Health, 25.0);

    TestTrue(TEXT("overflow == 25 (no stats armor to soak)"), FMath::Abs(Overflow - 25.0) < Eps);
    TestTrue(TEXT("health == 75 (dropped by exactly 25)"), FMath::Abs(Health.Health - 75.0) < Eps);
    // The health model's armor pool never absorbed anything (no second soak).
    TestTrue(TEXT("health-model armor still 0 (no second pool)"), FMath::Abs(Health.Armor) < Eps);
    return true;
}

// --- (d) ArmorMax=0 neutralizes the health-model pool -----------------------
// FPlayerHealthModel ctor with ArmorMax=0 -> AddArmor(50) leaves Armor==0;
// Apply(40) soaks 0 -> health -40.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcDamageRoutingArmorMaxZeroNeutralizesTest,
    "GTC.Player.DamageRouting.ArmorMaxZeroNeutralizes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcDamageRoutingArmorMaxZeroNeutralizesTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel Health = MakeHealthOnly();

    TestTrue(TEXT("MaxArmor floored to 0"), FMath::Abs(Health.MaxArmor) < Eps);

    Health.AddArmor(50.0);
    TestTrue(TEXT("AddArmor(50) leaves Armor == 0 (clamped to MaxArmor=0)"), FMath::Abs(Health.Armor) < Eps);

    Health.Apply(40.0);
    TestTrue(TEXT("Apply(40) soaks 0 armor -> health 60"), FMath::Abs(Health.Health - 60.0) < Eps);
    TestTrue(TEXT("Armor still 0 after Apply"), FMath::Abs(Health.Armor) < Eps);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
