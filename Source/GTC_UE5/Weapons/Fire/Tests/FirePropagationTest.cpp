// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../FirePropagation.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FFirePropagation — spread falloff, ignition, damage, and the FFireCell
 * grow/burn/die-out lifecycle. Prefix GTC.Weapons.Fire.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFirePropagationTest,
    "GTC.Weapons.Fire",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFirePropagationTest::RunTest(const FString& Parameters)
{
    const double Rad = FFirePropagation::SpreadRadiusM;

    // --- SpreadIntensity ---
    TestTrue(TEXT("a fire imparts heat point-blank"),
        FFirePropagation::SpreadIntensity(1.0, 0.0, Rad) > 0.0);
    TestEqual(TEXT("no spread beyond the radius"),
        FFirePropagation::SpreadIntensity(1.0, Rad + 0.1, Rad), 0.0);
    TestTrue(TEXT("spread falls off with distance"),
        FFirePropagation::SpreadIntensity(1.0, 1.0, Rad) > FFirePropagation::SpreadIntensity(1.0, 3.0, Rad));

    // --- CanIgnite ---
    TestTrue(TEXT("a flammable neighbour close to a big fire ignites"),
        FFirePropagation::CanIgnite(1.0, 0.5, /*flammability*/ 0.9, Rad));
    TestFalse(TEXT("a fire-resistant neighbour does not ignite"),
        FFirePropagation::CanIgnite(1.0, 0.5, /*flammability*/ 0.05, Rad));
    TestFalse(TEXT("a distant neighbour does not ignite"),
        FFirePropagation::CanIgnite(1.0, Rad + 1.0, 1.0, Rad));

    // --- DamagePerSecond ---
    TestTrue(TEXT("a fiercer fire hurts more"),
        FFirePropagation::DamagePerSecond(1.0) > FFirePropagation::DamagePerSecond(0.3));
    TestEqual(TEXT("no fire, no damage"), FFirePropagation::DamagePerSecond(0.0), 0.0);

    // --- FFireCell lifecycle ---
    {
        FFirePropagation::FFireCell Cell;
        TestFalse(TEXT("an unlit cell isn't burning"), Cell.IsBurning());

        Cell.Ignite(0.2, /*fuel*/ 1.0);
        TestTrue(TEXT("a lit cell burns"), Cell.IsBurning());

        // It grows toward a full blaze while fuelled.
        const double I0 = Cell.Intensity;
        Cell.Tick(0.3);
        TestTrue(TEXT("a fuelled fire grows"), Cell.Intensity > I0);
        TestTrue(TEXT("burning consumes fuel"), Cell.Fuel < 1.0);

        // Hot + fuelled -> can spread.
        for (int32 i = 0; i < 4; ++i) { Cell.Tick(0.3); }
        TestTrue(TEXT("an established fire can spread"), Cell.CanSpread());

        // Burn all the fuel away, then it guts out.
        for (int32 i = 0; i < 100 && Cell.Fuel > 0.0; ++i) { Cell.Tick(0.5); }
        TestTrue(TEXT("fuel runs out"), FMath::IsNearlyEqual(Cell.Fuel, 0.0, GtcTest::Eps));
        TestFalse(TEXT("a fuel-less fire can't spread"), Cell.CanSpread());
        for (int32 i = 0; i < 100 && Cell.IsBurning(); ++i) { Cell.Tick(0.5); }
        TestFalse(TEXT("the fire eventually dies out"), Cell.IsBurning());
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
