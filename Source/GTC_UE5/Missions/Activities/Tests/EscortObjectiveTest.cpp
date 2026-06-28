// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../EscortObjective.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FEscortObjective — the protect-the-convoy mission beat. GTC-original (no
 * Godot oracle). Pins delivering an escortee under no threat, losing it to sustained
 * fire, integrity recovering once the threat is suppressed, drain scaling with threat,
 * loss taking priority over arrival on the same tick, terminal states, and dt guards.
 * Prefix GTC.Missions.Activities.Escort.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEscortObjectiveTest,
    "GTC.Missions.Activities.Escort",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEscortObjectiveTest::RunTest(const FString& Parameters)
{
    using EState = FEscortObjective::EState;
    const FEscortObjective::FParams P; // drain 0.25/s, regen 0.1/s, threshold 0.05

    // ---- Fresh escort ---------------------------------------------------------
    {
        FEscortObjective E;
        E.Configure(P);
        TestTrue(TEXT("escorting"), E.IsEscorting());
        TestTrue(TEXT("full integrity"), FMath::IsNearlyEqual(E.Integrity(), 1.0, Eps));
        TestTrue(TEXT("no progress"), FMath::Abs(E.Progress()) <= Eps);
    }

    // ---- A calm escort is delivered intact ------------------------------------
    {
        FEscortObjective E;
        E.Configure(P);
        for (int32 I = 0; I < 20 && E.IsEscorting(); ++I)
        {
            E.Update(0.1, 0.0, 0.1); // steady progress, no threat
        }
        TestTrue(TEXT("delivered"), E.IsDelivered());
        TestTrue(TEXT("escortee intact"), FMath::IsNearlyEqual(E.Integrity(), 1.0, Eps));
    }

    // ---- Sustained fire destroys the escortee ---------------------------------
    {
        FEscortObjective E;
        E.Configure(P);
        for (int32 I = 0; I < 30; ++I)
        {
            E.Update(0.0, 1.0, 0.1); // 3s of full threat -> integrity 0.25
        }
        TestTrue(TEXT("still alive at 3s"), E.IsEscorting());
        for (int32 I = 0; I < 20; ++I)
        {
            E.Update(0.0, 1.0, 0.1); // push past 4s total
        }
        TestTrue(TEXT("lost to sustained fire"), E.IsLost());
    }

    // ---- Suppress the threat and the escortee recovers ------------------------
    {
        FEscortObjective E;
        E.Configure(P);
        for (int32 I = 0; I < 20; ++I)
        {
            E.Update(0.0, 1.0, 0.1); // 2s fire -> integrity ~0.5
        }
        const double Hurt = E.Integrity();
        TestTrue(TEXT("took damage"), Hurt < 0.6);
        for (int32 I = 0; I < 20; ++I)
        {
            E.Update(0.0, 0.0, 0.1); // 2s suppressed -> recovers
        }
        TestTrue(TEXT("recovers when suppressed"), E.Integrity() > Hurt);
        TestTrue(TEXT("still escorting"), E.IsEscorting());
    }

    // ---- Drain scales with threat level ---------------------------------------
    {
        FEscortObjective Light;
        Light.Configure(P);
        Light.Update(0.0, 0.5, 1.0); // half threat for 1s -> 1 - 0.5*0.25 = 0.875
        FEscortObjective Heavy;
        Heavy.Configure(P);
        Heavy.Update(0.0, 1.0, 1.0); // full threat for 1s -> 0.75
        TestTrue(TEXT("lighter threat does less damage"), Light.Integrity() > Heavy.Integrity());
        TestTrue(TEXT("half threat -> 0.875"), FMath::IsNearlyEqual(Light.Integrity(), 0.875, Eps));
    }

    // ---- Loss takes priority over arrival on the same tick --------------------
    {
        FEscortObjective E;
        E.Configure(P);
        for (int32 I = 0; I < 36; ++I)
        {
            E.Update(0.0, 1.0, 0.1); // bleed integrity down to ~0.1
        }
        TestTrue(TEXT("clinging on"), E.IsEscorting() && E.Integrity() < 0.2);
        E.Update(1.0, 1.0, 0.5); // would arrive, but the hit this tick is fatal
        TestTrue(TEXT("destroyed despite reaching the drop"), E.IsLost());
    }

    // ---- Terminal + dt guards -------------------------------------------------
    {
        FEscortObjective Done;
        Done.Configure(P);
        for (int32 I = 0; I < 20 && Done.IsEscorting(); ++I)
        {
            Done.Update(0.1, 0.0, 0.1);
        }
        TestTrue(TEXT("delivered"), Done.IsDelivered());
        Done.Update(0.0, 1.0, 1.0); // no-op once finished
        TestTrue(TEXT("stays delivered"), Done.IsDelivered());

        FEscortObjective Idle;
        Idle.Configure(P);
        Idle.Update(1.0, 1.0, 0.0);  // zero dt
        Idle.Update(1.0, 1.0, -4.0); // negative dt
        TestTrue(TEXT("non-positive dt does nothing"),
            Idle.IsEscorting() && FMath::IsNearlyEqual(Idle.Integrity(), 1.0, Eps)
            && FMath::Abs(Idle.Progress()) <= Eps);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
