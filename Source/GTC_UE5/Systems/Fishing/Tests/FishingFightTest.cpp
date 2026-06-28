// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../FishingFight.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FFishingFight — the reel-vs-line tug of war. GTC-original (no Godot
 * oracle). Pins the fresh state, the snap from yanking a fresh fish, a careful
 * angler landing it, a passive line never landing (nor snapping), the fish tiring,
 * terminal outcomes (no-op after Landed/Lost), and idle/negative-Dt guards. Prefix
 * GTC.Systems.Fishing. All inputs are method calls, so no shared helpers.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFishingFightTest,
    "GTC.Systems.Fishing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFishingFightTest::RunTest(const FString& Parameters)
{
    using EOutcome = FFishingFight::EOutcome;
    const FFishingFight::FParams P; // snap at 1.0, reel 0.25, pull 0.15, tensions .5/.5/.6, drain 0.2/s

    // ---- Fresh fight ----------------------------------------------------------
    {
        FFishingFight F;
        F.Configure(P);
        TestTrue(TEXT("starts fighting"), F.IsFighting());
        TestTrue(TEXT("full distance"), FMath::IsNearlyEqual(F.Distance(), 1.0, Eps));
        TestTrue(TEXT("fish at full strength"), FMath::IsNearlyEqual(F.PullStrength(), 1.0, Eps));
    }

    // ---- Yanking a fresh fish snaps the line ----------------------------------
    {
        FFishingFight F;
        F.Configure(P);
        F.Update(1.0, 0.1); // reel 1 vs pull 1 -> tension 1.6 > 1.0
        TestTrue(TEXT("yanking snaps the line"), F.IsLost());
        TestTrue(TEXT("tension exceeded line strength"), F.Tension() > P.LineStrength);
    }

    // ---- A careful angler (reel inversely to the fight) lands it ---------------
    {
        FFishingFight F;
        F.Configure(P);
        bool bSnapped = false;
        for (int32 I = 0; I < 400 && F.IsFighting(); ++I)
        {
            F.Update(1.0 - F.PullStrength(), 0.1); // ease off when it pulls, crank when it tires
            if (F.IsLost())
            {
                bSnapped = true;
            }
        }
        TestFalse(TEXT("careful play never snaps"), bSnapped);
        TestTrue(TEXT("careful play lands the fish"), F.IsLanded());
        TestTrue(TEXT("landed at the boat"), FMath::Abs(F.Distance()) <= Eps);
    }

    // ---- Never reeling never lands (and never snaps) ---------------------------
    {
        FFishingFight F;
        F.Configure(P);
        for (int32 I = 0; I < 400; ++I)
        {
            F.Update(0.0, 0.1);
        }
        TestTrue(TEXT("passive line stays in the fight"), F.IsFighting());
        TestFalse(TEXT("passive never lands"), F.IsLanded());
        TestTrue(TEXT("the fish took line"), F.Distance() > 1.0);
        TestTrue(TEXT("the fish tired out"), FMath::Abs(F.PullStrength()) <= Eps);
    }

    // ---- A worn-out fish can be reeled hard without snapping -------------------
    {
        FFishingFight F;
        F.Configure(P);
        for (int32 I = 0; I < 60; ++I)
        {
            F.Update(0.0, 0.1); // let it tire for 6s (drain 0.2/s -> stamina 0)
        }
        TestTrue(TEXT("fish is spent"), FMath::Abs(F.PullStrength()) <= Eps);
        F.Update(1.0, 0.1); // full crank, but no pull -> tension 0.5 < 1.0
        TestTrue(TEXT("cranking a spent fish is safe"), F.IsFighting() || F.IsLanded());
        TestTrue(TEXT("tension is just the reel effort"), FMath::IsNearlyEqual(F.Tension(), 0.5, Eps));
    }

    // ---- Outcomes are terminal ------------------------------------------------
    {
        FFishingFight Lost;
        Lost.Configure(P);
        Lost.Update(1.0, 0.1); // snap
        Lost.Update(0.0, 0.1); // no-op
        TestTrue(TEXT("stays Lost"), Lost.IsLost());

        FFishingFight Landed;
        Landed.Configure(P);
        for (int32 I = 0; I < 400 && Landed.IsFighting(); ++I)
        {
            Landed.Update(1.0 - Landed.PullStrength(), 0.1);
        }
        TestTrue(TEXT("landed"), Landed.IsLanded());
        const double D = Landed.Distance();
        Landed.Update(1.0, 0.1); // no-op
        TestTrue(TEXT("stays Landed"), Landed.IsLanded());
        TestTrue(TEXT("distance frozen after landing"), FMath::IsNearlyEqual(Landed.Distance(), D, Eps));
    }

    // ---- Idle / negative Dt doesn't advance or snap the fight ------------------
    {
        FFishingFight F;
        F.Configure(P);
        F.Update(1.0, 0.0);  // zero dt: must NOT snap despite a full yank
        TestTrue(TEXT("zero Dt is a no-op"), F.IsFighting());
        F.Update(1.0, -2.0); // negative dt: same
        TestTrue(TEXT("negative Dt is a no-op"), F.IsFighting());
        TestTrue(TEXT("distance untouched"), FMath::IsNearlyEqual(F.Distance(), 1.0, Eps));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
