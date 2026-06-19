// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../StoreRobbery.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FStoreRobbery — intimidation, clerk fear/compliance, register emptying,
 * and the silent-alarm countdown. Prefix GTC.Systems.Robbery.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStoreRobberyTest,
    "GTC.Systems.Robbery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoreRobberyTest::RunTest(const FString& Parameters)
{
    // --- IntimidationFrom ---
    TestTrue(TEXT("unarmed is barely intimidating"),
        FStoreRobbery::IntimidationFrom(false, 1.0, true) < 0.2);
    TestTrue(TEXT("a levelled gun beats a holstered one"),
        FStoreRobbery::IntimidationFrom(true, 0.5, true) > FStoreRobbery::IntimidationFrom(true, 0.5, false));
    TestTrue(TEXT("a higher weapon tier is scarier"),
        FStoreRobbery::IntimidationFrom(true, 1.0, true) > FStoreRobbery::IntimidationFrom(true, 0.0, true));
    TestTrue(TEXT("intimidation stays within [0,1]"),
        FStoreRobbery::IntimidationFrom(true, 5.0, true) <= 1.0 + GtcTest::Eps);

    // --- A held-up clerk cooperates and the till empties ---
    {
        FStoreRobbery R;
        R.RegisterTotal = 1000.0;
        const double Intim = FStoreRobbery::IntimidationFrom(true, 1.0, true);

        TestFalse(TEXT("clerk starts defiant"), R.IsCooperating());

        // Hold the gun on them for a couple of seconds: fear crosses the line.
        for (int32 i = 0; i < 30; ++i)
        {
            R.Update(0.1, /*threatening*/ true, Intim);
        }
        TestTrue(TEXT("sustained threat makes the clerk cooperate"), R.IsCooperating());
        TestTrue(TEXT("the register is draining"), R.CashTaken > 0.0);

        // Keep holding until the till is clear.
        for (int32 i = 0; i < 60 && !R.IsEmptied(); ++i)
        {
            R.Update(0.1, true, Intim);
        }
        TestTrue(TEXT("the register empties"), R.IsEmptied());
        TestTrue(TEXT("the take equals the till total"),
            FMath::IsNearlyEqual(R.CashTaken, R.RegisterTotal, GtcTest::Eps));
        TestFalse(TEXT("no alarm while the clerk cooperated"), R.bAlarmRaised);
    }

    // --- A defiant clerk reaches the silent alarm ---
    {
        FStoreRobbery R;
        R.RegisterTotal = 1000.0;

        // Never threaten them: they stay defiant and the alarm countdown runs out.
        bool Tripped = false;
        for (int32 i = 0; i < 100; ++i)
        {
            R.Update(0.1, /*threatening*/ false, 0.0);
            if (R.bAlarmRaised) { Tripped = true; break; }
        }
        TestTrue(TEXT("a defiant clerk trips the silent alarm"), Tripped);
        TestEqual(TEXT("no cash taken from a defiant clerk"), R.CashTaken, 0.0);
    }

    // --- Letting up lets fear ebb (cooperation is not permanent) ---
    {
        FStoreRobbery R;
        R.RegisterTotal = 1000.0;
        const double Intim = FStoreRobbery::IntimidationFrom(true, 1.0, true);
        for (int32 i = 0; i < 30; ++i) { R.Update(0.1, true, Intim); }
        const double Peak = R.Fear;
        TestTrue(TEXT("fear built up under threat"), Peak > FStoreRobbery::ComplianceThreshold);

        // Stop threatening; fear decays.
        for (int32 i = 0; i < 10; ++i) { R.Update(0.1, false, 0.0); }
        TestTrue(TEXT("fear ebbs once the threat lets up"), R.Fear < Peak);
    }

    // --- Progress / Remaining are sane ---
    {
        FStoreRobbery R;
        R.RegisterTotal = 200.0;
        R.CashTaken = 50.0;
        TestTrue(TEXT("progress reflects the take"),
            FMath::IsNearlyEqual(R.Progress(), 0.25, GtcTest::Eps));
        TestTrue(TEXT("remaining reflects the take"),
            FMath::IsNearlyEqual(R.Remaining(), 150.0, GtcTest::Eps));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
