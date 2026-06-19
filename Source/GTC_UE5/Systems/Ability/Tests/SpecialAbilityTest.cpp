// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../SpecialAbility.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FSpecialAbility — the per-character charge/activate/drain meter and its
 * kind-specific effect factors. Prefix GTC.Systems.Ability.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSpecialAbilityTest,
    "GTC.Systems.Ability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpecialAbilityTest::RunTest(const FString& Parameters)
{
    // --- Charging ---
    {
        FSpecialAbility A;
        TestEqual(TEXT("starts empty"), A.Fraction(), 0.0);
        A.AddCharge(FSpecialAbility::ChargeForKill);
        TestTrue(TEXT("a kill charges the bar"), A.Fraction() > 0.0);
        A.AddCharge(-1.0);
        TestTrue(TEXT("negative charge is ignored"),
            FMath::IsNearlyEqual(A.Fraction(), FSpecialAbility::ChargeForKill, GtcTest::Eps));
        A.AddCharge(5.0);
        TestTrue(TEXT("charge clamps to a full bar"),
            FMath::IsNearlyEqual(A.Fraction(), 1.0, GtcTest::Eps));
    }

    // --- Activation gating ---
    {
        FSpecialAbility A;
        TestFalse(TEXT("can't activate empty"), A.CanActivate());
        TestFalse(TEXT("activate empty is a no-op"), A.Activate());

        A.AddCharge(FSpecialAbility::MinActivateCharge - 0.01);
        TestFalse(TEXT("can't activate below the minimum"), A.CanActivate());

        A.AddCharge(0.5);
        TestTrue(TEXT("can activate with enough charge"), A.CanActivate());
        TestTrue(TEXT("activation succeeds"), A.Activate());
        TestTrue(TEXT("now active"), A.IsActive());
        TestFalse(TEXT("can't re-activate while running"), A.CanActivate());
    }

    // --- Drain + auto-end ---
    {
        FSpecialAbility A;
        A.AddCharge(1.0);
        A.Activate();
        A.Update(1.0);
        TestTrue(TEXT("active use drains the bar"), A.Fraction() < 1.0);

        // Drive it to empty: it auto-deactivates.
        for (int32 i = 0; i < 100 && A.IsActive(); ++i)
        {
            A.Update(0.5);
        }
        TestFalse(TEXT("auto-deactivates when empty"), A.IsActive());
        TestEqual(TEXT("empty bar reads zero"), A.Fraction(), 0.0);
    }

    // --- Charge persists when inactive (no passive bleed) ---
    {
        FSpecialAbility A;
        A.AddCharge(0.5);
        A.Update(10.0); // not active
        TestTrue(TEXT("idle charge does not bleed away"),
            FMath::IsNearlyEqual(A.Fraction(), 0.5, GtcTest::Eps));
    }

    // --- Deactivate keeps the remaining charge ---
    {
        FSpecialAbility A;
        A.AddCharge(1.0);
        A.Activate();
        A.Update(0.5);
        const double Left = A.Fraction();
        A.Deactivate();
        TestFalse(TEXT("deactivate stops it"), A.IsActive());
        A.Update(5.0);
        TestTrue(TEXT("deactivated bar keeps its charge"),
            FMath::IsNearlyEqual(A.Fraction(), Left, GtcTest::Eps));
    }

    // --- Effect factors are neutral when inactive ---
    {
        FSpecialAbility A;
        A.Kind = EAbilityKind::Marksman;
        TestEqual(TEXT("inactive time dilation is neutral"), A.TimeDilation(), 1.0);
        TestEqual(TEXT("inactive damage taken is neutral"), A.DamageTakenMultiplier(), 1.0);
        TestEqual(TEXT("inactive handling is neutral"), A.HandlingBoost(), 1.0);
    }

    // --- Kind-specific effects when active ---
    {
        FSpecialAbility Marks;
        Marks.Kind = EAbilityKind::Marksman;
        Marks.AddCharge(1.0); Marks.Activate();
        TestTrue(TEXT("marksman slows time"), Marks.TimeDilation() < 1.0);

        FSpecialAbility Brute;
        Brute.Kind = EAbilityKind::Bruiser;
        Brute.AddCharge(1.0); Brute.Activate();
        TestTrue(TEXT("bruiser takes less damage"), Brute.DamageTakenMultiplier() < 1.0);
        TestTrue(TEXT("bruiser deals more damage"), Brute.DamageDealtMultiplier() > 1.0);

        FSpecialAbility Wheel;
        Wheel.Kind = EAbilityKind::Driver;
        Wheel.AddCharge(1.0); Wheel.Activate();
        TestTrue(TEXT("driver boosts handling"), Wheel.HandlingBoost() > 1.0);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
