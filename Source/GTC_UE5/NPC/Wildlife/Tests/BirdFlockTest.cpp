// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../BirdFlock.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FBirdFlock — the collective flock-phase reflex. GTC-original (no Godot
 * oracle). Pins the phase ring (Perched->Startled->Wheeling->Settling->Perched), the
 * startle launch, the burst/wheel/settle timing, the calm-down gate, the altitude
 * ramp, a startle while wheeling resetting only the calm timer, a startle while
 * settling re-spooking the flock, and negative-Dt clamping. Prefix
 * GTC.NPC.Wildlife.Flock.
 *
 * Helpers live in a NAMED namespace so the unity build can't collide them with
 * another test's same-named anonymous-namespace helper.
 */
namespace BirdFlockTest
{
    FBirdFlock::FParams Tuning()
    {
        FBirdFlock::FParams P; // Burst 0.8, MinWheel 4, CalmBeforeSettle 2.5, Settle 3
        return P;
    }

    // Drive a fresh flock up to Wheeling and return it (startle, finish the burst).
    FBirdFlock Wheeling(const FBirdFlock::FParams& P)
    {
        FBirdFlock F;
        F.Update(true, 0.0, P);   // startle -> Startled
        F.Update(false, 0.4, P);  // mid-burst
        F.Update(false, 0.4, P);  // 0.8s -> Wheeling
        return F;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBirdFlockTest,
    "GTC.NPC.Wildlife.Flock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBirdFlockTest::RunTest(const FString& Parameters)
{
    using namespace BirdFlockTest;
    using EPhase = FBirdFlock::EPhase;
    const FBirdFlock::FParams P = Tuning();

    // ---- Fresh flock is perched and stays put without a startle ----------------
    {
        FBirdFlock F;
        TestTrue(TEXT("fresh flock is perched"), F.Phase() == EPhase::Perched);
        TestFalse(TEXT("perched is not aloft"), F.IsAloft());
        TestTrue(TEXT("perched altitude is 0"), FMath::Abs(F.Altitude(P)) <= Eps);
        F.Update(false, 1.0, P);
        TestTrue(TEXT("no startle keeps it perched"), F.Phase() == EPhase::Perched);
    }

    // ---- A startle launches the flock; the burst ramps altitude then wheels ----
    {
        FBirdFlock F;
        F.Update(true, 0.0, P);
        TestTrue(TEXT("startle launches into Startled"), F.Phase() == EPhase::Startled);
        TestTrue(TEXT("now aloft"), F.IsAloft());

        F.Update(false, 0.4, P); // halfway through the 0.8s burst
        TestTrue(TEXT("burst altitude ~0.5"), FMath::IsNearlyEqual(F.Altitude(P), 0.5, Eps));
        TestTrue(TEXT("still bursting"), F.Phase() == EPhase::Startled);

        F.Update(false, 0.4, P); // 0.8s -> Wheeling
        TestTrue(TEXT("burst done -> Wheeling"), F.Phase() == EPhase::Wheeling);
        TestTrue(TEXT("wheeling altitude is 1"), FMath::IsNearlyEqual(F.Altitude(P), 1.0, Eps));
    }

    // ---- Wheeling holds until min-wheel AND calm, then settles, then perches ---
    {
        FBirdFlock F = Wheeling(P); // entered Wheeling, ~0.8s of calm so far
        F.Update(false, 2.0, P);    // wheel 2s (calm ~2.8)
        TestTrue(TEXT("still wheeling before min wheel"), F.Phase() == EPhase::Wheeling);
        F.Update(false, 2.0, P);    // wheel 4s total -> min met, calm met -> Settling
        TestTrue(TEXT("settles after the min wheel + calm"), F.Phase() == EPhase::Settling);

        F.Update(false, 1.5, P);    // halfway down the 3s settle
        TestTrue(TEXT("settling altitude ~0.5"), FMath::IsNearlyEqual(F.Altitude(P), 0.5, Eps));
        F.Update(false, 1.5, P);    // 3s -> Perched
        TestTrue(TEXT("re-perched after settle"), F.Phase() == EPhase::Perched);
        TestTrue(TEXT("perched altitude back to 0"), FMath::Abs(F.Altitude(P)) <= Eps);
    }

    // ---- A startle while wheeling only resets the calm timer (stays up) --------
    {
        FBirdFlock F = Wheeling(P);
        // 4s of wheeling so the min-wheel is met, but startle on the same tick:
        F.Update(true, 4.0, P);
        TestTrue(TEXT("startle while wheeling keeps it wheeling"), F.Phase() == EPhase::Wheeling);
        // Now let it calm: needs CalmBeforeSettle (2.5s) undisturbed.
        F.Update(false, 2.0, P);
        TestTrue(TEXT("still wheeling 2s after the re-startle"), F.Phase() == EPhase::Wheeling);
        F.Update(false, 1.0, P); // calm now 3s >= 2.5, wheel time well past min
        TestTrue(TEXT("settles once calm again"), F.Phase() == EPhase::Settling);
    }

    // ---- A startle while settling re-spooks the flock back into the burst ------
    {
        FBirdFlock F = Wheeling(P);
        F.Update(false, 4.0, P); // -> Settling
        TestTrue(TEXT("settling"), F.Phase() == EPhase::Settling);
        F.Update(true, 0.1, P);  // re-spooked mid-descent
        TestTrue(TEXT("re-spook bursts again"), F.Phase() == EPhase::Startled);
        TestTrue(TEXT("re-burst altitude ramps from low"), F.Altitude(P) < 0.5);
    }

    // ---- Negative Dt never advances the phase or the clocks --------------------
    {
        FBirdFlock F;
        F.Update(true, 0.0, P);     // Startled
        F.Update(false, -5.0, P);   // ignored
        TestTrue(TEXT("negative Dt keeps it Startled"), F.Phase() == EPhase::Startled);
        TestTrue(TEXT("negative Dt added no altitude"), FMath::Abs(F.Altitude(P)) <= Eps);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
