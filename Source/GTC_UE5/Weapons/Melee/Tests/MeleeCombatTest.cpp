// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../MeleeCombat.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FMeleeCombat — strike/combo/block/parry math and the FMeleeFighter
 * stamina + combo state. Prefix GTC.Weapons.Melee.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMeleeCombatTest,
    "GTC.Weapons.Melee",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMeleeCombatTest::RunTest(const FString& Parameters)
{
    using Strike = EMeleeStrike;

    // --- Base math ---
    TestTrue(TEXT("heavy hits harder than light"),
        FMeleeCombat::BaseDamage(Strike::Heavy) > FMeleeCombat::BaseDamage(Strike::Light));
    TestTrue(TEXT("heavy costs more stamina than light"),
        FMeleeCombat::StaminaCost(Strike::Heavy) > FMeleeCombat::StaminaCost(Strike::Light));

    // --- Combo multiplier ---
    TestTrue(TEXT("first hit has no combo bonus"),
        FMath::IsNearlyEqual(FMeleeCombat::ComboMultiplier(0), 1.0, GtcTest::Eps));
    TestTrue(TEXT("combo multiplier rises with chained hits"),
        FMeleeCombat::ComboMultiplier(3) > FMeleeCombat::ComboMultiplier(1));
    TestTrue(TEXT("combo multiplier is capped"),
        FMeleeCombat::ComboMultiplier(1000) <= FMeleeCombat::MaxComboMultiplier + GtcTest::Eps);

    // --- Exhaustion ---
    TestTrue(TEXT("full stamina = full damage"),
        FMath::IsNearlyEqual(FMeleeCombat::ExhaustionFactor(1.0), 1.0, GtcTest::Eps));
    TestTrue(TEXT("empty stamina drops to the floor"),
        FMath::IsNearlyEqual(FMeleeCombat::ExhaustionFactor(0.0), FMeleeCombat::ExhaustionFloor, GtcTest::Eps));
    TestTrue(TEXT("exhaustion scales between"),
        FMeleeCombat::ExhaustionFactor(0.3) < FMeleeCombat::ExhaustionFactor(0.9));

    // StrikeDamage composes the three.
    TestTrue(TEXT("a combo'd fresh heavy beats a lone tired light"),
        FMeleeCombat::StrikeDamage(Strike::Heavy, 3, 1.0) > FMeleeCombat::StrikeDamage(Strike::Light, 0, 0.0));

    // --- Block / parry ---
    TestTrue(TEXT("last-instant block is a perfect parry"),
        FMeleeCombat::IsPerfectParry(FMeleeCombat::ParryWindow * 0.5));
    TestFalse(TEXT("a block held since well before the hit is not perfect"),
        FMeleeCombat::IsPerfectParry(FMeleeCombat::ParryWindow + 1.0));

    const double Incoming = 20.0;
    TestEqual(TEXT("unblocked damage lands in full"),
        FMeleeCombat::IncomingAfterBlock(Incoming, Strike::Light, /*blocking*/ false, /*perfect*/ false), Incoming);
    TestTrue(TEXT("a normal block soaks most of it"),
        FMeleeCombat::IncomingAfterBlock(Incoming, Strike::Light, true, false) < Incoming);
    TestEqual(TEXT("a perfect parry negates it"),
        FMeleeCombat::IncomingAfterBlock(Incoming, Strike::Heavy, true, true), 0.0);
    TestTrue(TEXT("a heavy bullies through a block more than a light"),
        FMeleeCombat::IncomingAfterBlock(Incoming, Strike::Heavy, true, false)
        > FMeleeCombat::IncomingAfterBlock(Incoming, Strike::Light, true, false));

    TestTrue(TEXT("a perfect parry counters for bonus damage"),
        FMeleeCombat::CounterDamage(Incoming, true) > Incoming);
    TestEqual(TEXT("no counter without a perfect parry"),
        FMeleeCombat::CounterDamage(Incoming, false), 0.0);

    // --- FMeleeFighter: combo chaining ---
    {
        FMeleeCombat::FMeleeFighter F;
        const double First = F.Strike(Strike::Light);
        const double Second = F.Strike(Strike::Light);
        TestTrue(TEXT("a chained second hit does more (combo) at near-equal stamina"), Second > First);
        TestEqual(TEXT("two hits => combo count 2"), F.ComboCount, 2);

        // Letting the window lapse resets the chain.
        F.Tick(F.ComboWindow + 0.01);
        TestEqual(TEXT("combo lapses after the window"), F.ComboCount, 0);
    }

    // --- FMeleeFighter: stamina gating + regen ---
    {
        FMeleeCombat::FMeleeFighter F;
        int32 Thrown = 0;
        while (F.CanStrike(Strike::Heavy))
        {
            F.Strike(Strike::Heavy);
            ++Thrown;
        }
        TestTrue(TEXT("heavies drain the stamina pool"), Thrown >= 1);
        TestEqual(TEXT("a strike with no stamina deals nothing"), F.Strike(Strike::Heavy), 0.0);

        const double Before = F.Stamina;
        F.Tick(1.0);
        TestTrue(TEXT("stamina regenerates over time"), F.Stamina > Before);
        TestTrue(TEXT("stamina never exceeds max"), F.Stamina <= F.MaxStamina + GtcTest::Eps);
    }

    // --- FMeleeFighter: taking a hit breaks the combo ---
    {
        FMeleeCombat::FMeleeFighter F;
        F.Strike(Strike::Light);
        F.Strike(Strike::Light);
        TestTrue(TEXT("combo is running"), F.ComboCount > 0);
        F.OnHitTaken();
        TestEqual(TEXT("eating a blow breaks the combo"), F.ComboCount, 0);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
