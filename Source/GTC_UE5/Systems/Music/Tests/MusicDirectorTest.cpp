// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../MusicDirector.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FMusicDirector — the interactive score. GTC-original (no Godot oracle).
 * Pins the smoothed attack rise, the asymmetric (slower) release, intensity bounds,
 * the layer quantization, the hysteresis (same intensity -> different layer by
 * history), the single-layer and clamp edge cases. Prefix GTC.Systems.Music.
 *
 * Helpers live in a NAMED namespace so the unity build can't collide them with
 * another test's same-named anonymous-namespace helper.
 */
namespace MusicDirectorTest
{
    FMusicDirector::FParams Tuning()
    {
        FMusicDirector::FParams P; // attack 3, release 0.4, 4 layers, hysteresis 0.06
        return P;
    }

    // Hold `Heat` for `Count` ticks of `Dt` so the intensity settles.
    void Settle(FMusicDirector& D, double Heat, double Dt, int32 Count)
    {
        for (int32 I = 0; I < Count; ++I)
        {
            D.Update(Heat, Dt);
        }
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMusicDirectorTest,
    "GTC.Systems.Music",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicDirectorTest::RunTest(const FString& Parameters)
{
    using namespace MusicDirectorTest;

    // ---- Fresh director is silent ---------------------------------------------
    {
        FMusicDirector D;
        D.Configure(Tuning());
        TestTrue(TEXT("fresh intensity 0"), FMath::Abs(D.Intensity()) <= Eps);
        TestEqual(TEXT("fresh layer 0"), D.Layer(), 0);
        TestEqual(TEXT("four layers"), D.LayerCount(), 4);
    }

    // ---- Attack: intensity rushes up toward heat ------------------------------
    {
        FMusicDirector D;
        D.Configure(Tuning());
        D.Update(1.0, 0.1); // frac = 3*0.1 = 0.3 -> intensity 0.3
        TestTrue(TEXT("attack step rises to 0.3"), FMath::IsNearlyEqual(D.Intensity(), 0.3, Eps));
        TestEqual(TEXT("0.3 crosses into layer 1"), D.Layer(), 1);

        D.Update(1.0, 1.0); // frac saturates -> intensity 1
        TestTrue(TEXT("saturates to 1"), FMath::IsNearlyEqual(D.Intensity(), 1.0, Eps));
        TestEqual(TEXT("full intensity is the top layer"), D.Layer(), 3);
    }

    // ---- Release is slower than attack (the tail-out) -------------------------
    {
        FMusicDirector Rise;
        Rise.Configure(Tuning());
        Rise.Update(1.0, 0.1); // from 0 -> 0.3, rose 0.3
        const double AttackRise = Rise.Intensity() - 0.0;

        FMusicDirector Fall;
        Fall.Configure(Tuning());
        Fall.Update(1.0, 1.0); // saturate to 1.0
        Fall.Update(0.0, 0.1); // release: frac 0.4*0.1=0.04 -> 0.96, fell 0.04
        const double ReleaseFall = 1.0 - Fall.Intensity();

        TestTrue(TEXT("release fall (~0.04) is far smaller than attack rise (0.3)"), ReleaseFall < AttackRise);
        TestTrue(TEXT("release step is 0.04"), FMath::IsNearlyEqual(ReleaseFall, 0.04, Eps));
    }

    // ---- Hysteresis: the SAME intensity gives a different layer by history -----
    {
        // Settle at heat 0.25 coming DOWN from a peak -> stays in the hotter layer 1.
        FMusicDirector Falling;
        Falling.Configure(Tuning());
        Falling.Update(1.0, 1.0);          // peak to layer 3
        Settle(Falling, 0.25, 0.1, 400);   // ease down, settle near 0.25 from above
        TestTrue(TEXT("settled intensity is ~0.25"), FMath::IsNearlyEqual(Falling.Intensity(), 0.25, 1e-2));
        TestEqual(TEXT("coming down, 0.25 holds layer 1 (hysteresis)"), Falling.Layer(), 1);

        // Settle at the same heat 0.25 coming UP from silence -> still layer 0.
        FMusicDirector Rising;
        Rising.Configure(Tuning());
        Settle(Rising, 0.25, 0.1, 200);    // ease up, settle near 0.25 from below
        TestTrue(TEXT("settled intensity is ~0.25"), FMath::IsNearlyEqual(Rising.Intensity(), 0.25, 1e-2));
        TestEqual(TEXT("rising, 0.25 stays in layer 0 (hysteresis)"), Rising.Layer(), 0);
    }

    // ---- A single-layer score never changes layer -----------------------------
    {
        FMusicDirector D;
        FMusicDirector::FParams P = Tuning();
        P.LayerCount = 1;
        D.Configure(P);
        D.Update(1.0, 1.0);
        TestEqual(TEXT("one layer is always layer 0"), D.Layer(), 0);
        TestEqual(TEXT("LayerCount reports 1"), D.LayerCount(), 1);
    }

    // ---- Heat and Dt are clamped ---------------------------------------------
    {
        FMusicDirector Over;
        Over.Configure(Tuning());
        Over.Update(5.0, 1.0); // heat clamps to 1
        TestTrue(TEXT("over-range heat clamps to 1"), FMath::IsNearlyEqual(Over.Intensity(), 1.0, Eps));

        FMusicDirector Neg;
        Neg.Configure(Tuning());
        Neg.Update(1.0, 0.1);              // intensity 0.3
        const double Before = Neg.Intensity();
        Neg.Update(1.0, -5.0);             // negative Dt -> no change
        TestTrue(TEXT("negative Dt freezes intensity"), FMath::IsNearlyEqual(Neg.Intensity(), Before, Eps));

        // A negative heat is treated as silence and bleeds intensity down (slowly).
        Neg.Update(-1.0, 0.1);
        TestTrue(TEXT("negative heat clamps to 0 and releases"), Neg.Intensity() < Before);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
