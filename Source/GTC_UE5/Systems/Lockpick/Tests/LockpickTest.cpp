// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../Lockpick.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FLockpick — the lock-tumbler minigame. GTC-original (no Godot oracle).
 * Pins the alignment curve, a perfect pick unlocking with no stress, forcing
 * off-spot snapping the pick, forcing only-half-aligned still snapping (you must find
 * the exact spot), stress recovering on release, terminal outcomes, and the dt/clamp
 * guards. Prefix GTC.Systems.Lockpick. All inputs are method calls, so no shared
 * helpers.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLockpickTest,
    "GTC.Systems.Lockpick",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLockpickTest::RunTest(const FString& Parameters)
{
    using EOutcome = FLockpick::EOutcome;
    const FLockpick::FParams P; // sweet 0.5, tol 0.08, turn 0.8, stress 1.5, strength 1.0, relax 2.0

    // ---- Alignment curve ------------------------------------------------------
    {
        FLockpick L;
        L.Configure(P);
        TestTrue(TEXT("dead on the spot -> 1"), FMath::IsNearlyEqual(L.AlignmentAt(0.5), 1.0, Eps));
        TestTrue(TEXT("at the tolerance edge -> 0"), FMath::Abs(L.AlignmentAt(0.58)) <= Eps);
        TestTrue(TEXT("half a tolerance off -> 0.5"), FMath::IsNearlyEqual(L.AlignmentAt(0.54), 0.5, Eps));
        TestTrue(TEXT("way off -> 0"), FMath::Abs(L.AlignmentAt(0.0)) <= Eps);
    }

    // ---- A perfect pick unlocks with no stress --------------------------------
    {
        FLockpick L;
        L.Configure(P);
        for (int32 I = 0; I < 100 && L.IsPicking(); ++I)
        {
            L.Update(0.5, 1.0, 0.1); // dead on the spot, full tension
        }
        TestTrue(TEXT("perfect pick unlocks"), L.IsUnlocked());
        TestTrue(TEXT("no stress on the sweet spot"), FMath::Abs(L.Stress()) <= Eps);
        TestTrue(TEXT("progress maxed"), FMath::IsNearlyEqual(L.Progress(), 1.0, Eps));
    }

    // ---- Forcing off the spot snaps the pick ----------------------------------
    {
        FLockpick L;
        L.Configure(P);
        bool bUnlocked = false;
        for (int32 I = 0; I < 100 && L.IsPicking(); ++I)
        {
            L.Update(0.0, 1.0, 0.1); // way off, cranking hard
            if (L.IsUnlocked())
            {
                bUnlocked = true;
            }
        }
        TestTrue(TEXT("forcing off-spot snaps the pick"), L.IsBroken());
        TestFalse(TEXT("never made progress off-spot"), bUnlocked);
        TestTrue(TEXT("no turn progress off-spot"), FMath::Abs(L.Progress()) <= Eps);
    }

    // ---- Even half-aligned, forcing breaks before it opens --------------------
    {
        FLockpick L;
        L.Configure(P);
        for (int32 I = 0; I < 100 && L.IsPicking(); ++I)
        {
            L.Update(0.54, 1.0, 0.1); // half alignment, full tension
        }
        TestTrue(TEXT("half-aligned forcing still snaps"), L.IsBroken());
    }

    // ---- Releasing tension lets the pick un-stress ----------------------------
    {
        FLockpick L;
        L.Configure(P);
        L.Update(0.0, 1.0, 0.2); // build some stress without unlocking
        L.Update(0.0, 1.0, 0.1);
        const double Stressed = L.Stress();
        TestTrue(TEXT("stress accrued"), Stressed > 0.0);
        L.Update(0.5, 0.0, 0.1); // release tension (angle irrelevant)
        TestTrue(TEXT("releasing recovers stress"), L.Stress() < Stressed);
        TestTrue(TEXT("still picking after a release"), L.IsPicking());
    }

    // ---- Outcomes are terminal ------------------------------------------------
    {
        FLockpick Unlocked;
        Unlocked.Configure(P);
        for (int32 I = 0; I < 100 && Unlocked.IsPicking(); ++I)
        {
            Unlocked.Update(0.5, 1.0, 0.1);
        }
        TestTrue(TEXT("unlocked"), Unlocked.IsUnlocked());
        Unlocked.Update(0.0, 1.0, 0.1); // no-op
        TestTrue(TEXT("stays unlocked"), Unlocked.IsUnlocked());

        FLockpick Broken;
        Broken.Configure(P);
        for (int32 I = 0; I < 100 && Broken.IsPicking(); ++I)
        {
            Broken.Update(0.0, 1.0, 0.1);
        }
        TestTrue(TEXT("broken"), Broken.IsBroken());
        const double S = Broken.Stress();
        Broken.Update(0.5, 1.0, 0.1); // no-op
        TestTrue(TEXT("stays broken"), Broken.IsBroken());
        TestTrue(TEXT("stress frozen after break"), FMath::IsNearlyEqual(Broken.Stress(), S, Eps));
    }

    // ---- dt / input guards ----------------------------------------------------
    {
        FLockpick L;
        L.Configure(P);
        L.Update(0.5, 1.0, 0.0);  // zero dt -> no progress
        TestTrue(TEXT("zero dt makes no progress"), FMath::Abs(L.Progress()) <= Eps);
        L.Update(0.5, 1.0, -2.0); // negative dt -> no progress
        TestTrue(TEXT("negative dt makes no progress"), FMath::Abs(L.Progress()) <= Eps);
        // Over-range angle clamps (2.0 -> 1.0, far from 0.5 -> alignment 0).
        TestTrue(TEXT("over-range angle clamps to a cold alignment"), FMath::Abs(L.AlignmentAt(2.0)) <= Eps);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
