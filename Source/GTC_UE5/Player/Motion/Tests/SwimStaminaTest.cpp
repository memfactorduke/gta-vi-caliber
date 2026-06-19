// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../SwimStamina.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Parity tests for FSwimStamina, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_swim_stamina.gd. The oracle uses is_equal_approx
 * throughout (Eps=1e-4 here); ordering/threshold checks (>, <=) are exact.
 */

using GtcTest::Eps;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSwimStaminaTest,
    "GTC.Player.Motion.SwimStamina",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSwimStaminaTest::RunTest(const FString& Parameters)
{
    // test_starts_full
    {
        FSwimStamina S(20.0, 100.0);
        TestEqual(TEXT("starts oxygen"), S.Oxygen(), 20.0, Eps);
        TestEqual(TEXT("starts stamina"), S.Stamina(), 100.0, Eps);
        TestEqual(TEXT("starts oxygen frac"), S.OxygenFraction(), 1.0, Eps);
        TestEqual(TEXT("starts stamina frac"), S.StaminaFraction(), 1.0, Eps);
    }

    // test_oxygen_drains_underwater: 1 atm at surface -> 19.0
    {
        FSwimStamina S(20.0, 100.0);
        S.Update(true, false, 0.0, 1.0);
        TestEqual(TEXT("drains underwater"), S.Oxygen(), 19.0, Eps);
    }

    // test_oxygen_drains_faster_at_depth: 10m = 2 atm -> loses 2.0
    {
        FSwimStamina Shallow(20.0, 100.0);
        FSwimStamina Deep(20.0, 100.0);
        Shallow.Update(true, false, 0.0, 1.0);
        Deep.Update(true, false, 10.0, 1.0);
        const double ShallowLost = 20.0 - Shallow.Oxygen();
        const double DeepLost = 20.0 - Deep.Oxygen();
        TestTrue(TEXT("deep loses more"), DeepLost > ShallowLost);
        TestEqual(TEXT("deep loses 2.0"), DeepLost, 2.0, Eps);
    }

    // test_oxygen_refills_at_surface
    {
        FSwimStamina S(20.0, 100.0);
        S.Update(true, false, 0.0, 10.0);
        const double Drained = S.Oxygen();
        S.Update(false, false, 0.0, 1.0);
        TestTrue(TEXT("refill increases"), S.Oxygen() > Drained);
        TestEqual(TEXT("refill +5"), S.Oxygen(), Drained + 5.0, Eps);
    }

    // test_oxygen_clamped_to_zero
    {
        FSwimStamina S(20.0, 100.0);
        S.Update(true, false, 0.0, 1000.0);
        TestEqual(TEXT("oxygen clamp zero"), S.Oxygen(), 0.0, Eps);
    }

    // test_oxygen_clamped_to_max
    {
        FSwimStamina S(20.0, 100.0);
        S.Update(false, false, 0.0, 1000.0);
        TestEqual(TEXT("oxygen clamp max"), S.Oxygen(), 20.0, Eps);
    }

    // test_stamina_drains_swimming
    {
        FSwimStamina S(20.0, 100.0);
        S.Update(true, false, 0.0, 1.0);
        TestEqual(TEXT("stamina drains"), S.Stamina(), 96.0, Eps);
    }

    // test_stamina_drains_faster_sprinting
    {
        FSwimStamina Cruise(20.0, 100.0);
        FSwimStamina Sprint(20.0, 100.0);
        Cruise.Update(true, false, 0.0, 1.0);
        Sprint.Update(true, true, 0.0, 1.0);
        TestEqual(TEXT("cruise 96"), Cruise.Stamina(), 96.0, Eps);
        TestEqual(TEXT("sprint 88"), Sprint.Stamina(), 88.0, Eps);
    }

    // test_stamina_recovers_idle
    {
        FSwimStamina S(20.0, 100.0);
        S.Update(true, false, 0.0, 5.0);
        const double Tired = S.Stamina();
        S.Update(false, false, 0.0, 1.0);
        TestTrue(TEXT("recover increases"), S.Stamina() > Tired);
        TestEqual(TEXT("recover +3"), S.Stamina(), Tired + 3.0, Eps);
    }

    // test_stamina_clamped_to_zero
    {
        FSwimStamina S(20.0, 100.0);
        S.Update(true, true, 0.0, 1000.0);
        TestEqual(TEXT("stamina clamp zero"), S.Stamina(), 0.0, Eps);
    }

    // test_stamina_clamped_to_max
    {
        FSwimStamina S(20.0, 100.0);
        S.Update(false, false, 0.0, 1000.0);
        TestEqual(TEXT("stamina clamp max"), S.Stamina(), 100.0, Eps);
    }

    // test_is_drowning_only_at_zero_oxygen_underwater
    {
        FSwimStamina S(20.0, 100.0);
        TestFalse(TEXT("not drowning fresh"), S.IsDrowning(true));
        S.Update(true, false, 0.0, 1000.0);
        TestTrue(TEXT("drowning out of air under"), S.IsDrowning(true));
        TestFalse(TEXT("not drowning at surface"), S.IsDrowning(false));
    }

    // test_is_exhausted_at_zero_stamina
    {
        FSwimStamina S(20.0, 100.0);
        TestFalse(TEXT("not exhausted fresh"), S.IsExhausted());
        S.Update(true, true, 0.0, 1000.0);
        TestTrue(TEXT("exhausted"), S.IsExhausted());
    }

    // test_drown_damage_zero_with_air
    {
        FSwimStamina S(20.0, 100.0);
        TestEqual(TEXT("no damage with air"), S.DrownDamage(true, 1.0, 10.0), 0.0, Eps);
    }

    // test_drown_damage_positive_without_air
    {
        FSwimStamina S(20.0, 100.0);
        S.Update(true, false, 0.0, 1000.0);
        TestEqual(TEXT("damage without air"), S.DrownDamage(true, 0.5, 10.0), 5.0, Eps);
    }

    // test_drown_damage_zero_at_surface_even_without_air
    {
        FSwimStamina S(20.0, 100.0);
        S.Update(true, false, 0.0, 1000.0);
        TestEqual(TEXT("no damage at surface"), S.DrownDamage(false, 1.0, 10.0), 0.0, Eps);
    }

    // test_drown_damage_guards_negative
    {
        FSwimStamina S(20.0, 100.0);
        S.Update(true, false, 0.0, 1000.0);
        TestEqual(TEXT("neg delta guarded"), S.DrownDamage(true, -1.0, 10.0), 0.0, Eps);
        TestEqual(TEXT("neg dps guarded"), S.DrownDamage(true, 1.0, -10.0), 0.0, Eps);
    }

    // test_swim_speed_base_when_fresh
    {
        FSwimStamina S(20.0, 100.0);
        TestEqual(TEXT("base speed fresh"), S.SwimSpeed(5.0, false), 5.0, Eps);
    }

    // test_swim_speed_faster_sprinting
    {
        FSwimStamina S(20.0, 100.0);
        TestEqual(TEXT("sprint speed"), S.SwimSpeed(5.0, true), 8.0, Eps);
    }

    // test_swim_speed_slower_exhausted
    {
        FSwimStamina S(20.0, 100.0);
        S.Update(true, true, 0.0, 1000.0);
        TestEqual(TEXT("exhausted no sprint flag"), S.SwimSpeed(5.0, false), 2.5, Eps);
        TestEqual(TEXT("exhausted with sprint flag"), S.SwimSpeed(5.0, true), 2.5, Eps);
    }

    // test_pressure_rises_with_depth_and_guards_negative
    {
        FSwimStamina S(20.0, 100.0);
        TestEqual(TEXT("pressure surface"), S.PressureAt(0.0), 1.0, Eps);
        TestEqual(TEXT("pressure 10m"), S.PressureAt(10.0), 2.0, Eps);
        TestTrue(TEXT("pressure rises"), S.PressureAt(20.0) > S.PressureAt(10.0));
        TestEqual(TEXT("pressure neg clamps"), S.PressureAt(-50.0), 1.0, Eps);
    }

    // test_update_guards_negative_delta
    {
        FSwimStamina S(20.0, 100.0);
        S.Update(true, true, 10.0, -5.0);
        TestEqual(TEXT("neg delta oxygen"), S.Oxygen(), 20.0, Eps);
        TestEqual(TEXT("neg delta stamina"), S.Stamina(), 100.0, Eps);
    }

    // test_surface_refills_breath_only
    {
        FSwimStamina S(20.0, 100.0);
        S.Update(true, true, 0.0, 3.0);
        const double Tired = S.Stamina();
        S.Surface();
        TestEqual(TEXT("surface refills oxygen"), S.Oxygen(), 20.0, Eps);
        TestEqual(TEXT("surface keeps stamina"), S.Stamina(), Tired, Eps);
    }

    // test_reset_restores_both
    {
        FSwimStamina S(20.0, 100.0);
        S.Update(true, true, 5.0, 100.0);
        S.Reset();
        TestEqual(TEXT("reset oxygen"), S.Oxygen(), 20.0, Eps);
        TestEqual(TEXT("reset stamina"), S.Stamina(), 100.0, Eps);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
