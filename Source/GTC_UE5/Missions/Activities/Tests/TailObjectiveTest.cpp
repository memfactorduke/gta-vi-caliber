// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../TailObjective.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FTailObjective — the follow-without-being-made mission beat. GTC-original
 * (no Godot oracle). Pins a clean tail succeeding, losing the target (the grace
 * clock), getting spotted by view and by being too close, suspicion cooling when
 * safe, the lost clock resetting on return to range, terminal states, and dt guards.
 * Prefix GTC.Missions.Activities.Tail.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTailObjectiveTest,
    "GTC.Missions.Activities.Tail",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTailObjectiveTest::RunTest(const FString& Parameters)
{
    using EState = FTailObjective::EState;
    const FTailObjective::FParams P; // min 400, max 3000, grace 4, rise 0.4, fall 0.3, required 20

    // ---- Fresh tail -----------------------------------------------------------
    {
        FTailObjective T;
        T.Configure(P);
        TestTrue(TEXT("starts tailing"), T.IsTailing());
        TestTrue(TEXT("no suspicion"), FMath::Abs(T.Suspicion()) <= Eps);
        TestTrue(TEXT("no progress"), FMath::Abs(T.TrackProgress()) <= Eps);
    }

    // ---- A clean tail in the band succeeds ------------------------------------
    {
        FTailObjective T;
        T.Configure(P);
        for (int32 I = 0; I < 200 && T.IsTailing(); ++I)
        {
            T.Update(1500.0, false, 0.1); // in the band, out of view
        }
        TestTrue(TEXT("clean tail succeeds"), T.IsSucceeded());
        TestTrue(TEXT("suspicion never rose"), FMath::Abs(T.Suspicion()) <= Eps);
        TestTrue(TEXT("progress full"), FMath::IsNearlyEqual(T.TrackProgress(), 1.0, Eps));
    }

    // ---- Drifting too far loses the target after the grace --------------------
    {
        FTailObjective T;
        T.Configure(P);
        for (int32 I = 0; I < 30; ++I)
        {
            T.Update(5000.0, false, 0.1); // 3s out of range (< 4s grace)
        }
        TestTrue(TEXT("still tailing within the grace"), T.IsTailing());
        for (int32 I = 0; I < 20; ++I)
        {
            T.Update(5000.0, false, 0.1); // push past the grace
        }
        TestTrue(TEXT("lost the target past the grace"), T.IsLost());
    }

    // ---- Sitting in the target's view gets you spotted ------------------------
    {
        FTailObjective T;
        T.Configure(P);
        for (int32 I = 0; I < 30 && T.IsTailing(); ++I)
        {
            T.Update(1500.0, true, 0.1); // in band but in view -> suspicion 0.4/s -> 1.0 at 2.5s
        }
        TestTrue(TEXT("spotted in view"), T.IsSpotted());
        TestTrue(TEXT("suspicion maxed"), FMath::IsNearlyEqual(T.Suspicion(), 1.0, Eps));
    }

    // ---- Tailgating (too close) also gets you spotted, even out of view -------
    {
        FTailObjective T;
        T.Configure(P);
        for (int32 I = 0; I < 30 && T.IsTailing(); ++I)
        {
            T.Update(200.0, false, 0.1); // under MinDistance
        }
        TestTrue(TEXT("spotted for tailgating"), T.IsSpotted());
    }

    // ---- Suspicion cools when you back into the safe band ---------------------
    {
        FTailObjective T;
        T.Configure(P);
        for (int32 I = 0; I < 10; ++I)
        {
            T.Update(1500.0, true, 0.1); // 1s in view -> suspicion ~0.4
        }
        const double Hot = T.Suspicion();
        TestTrue(TEXT("suspicion rose"), Hot > 0.3);
        for (int32 I = 0; I < 10; ++I)
        {
            T.Update(1500.0, false, 0.1); // 1s safe -> cools by ~0.3
        }
        TestTrue(TEXT("suspicion cools off"), T.Suspicion() < Hot);
        TestTrue(TEXT("still tailing"), T.IsTailing());
    }

    // ---- The lost clock resets when you close back into range -----------------
    {
        FTailObjective T;
        T.Configure(P);
        for (int32 I = 0; I < 20; ++I)
        {
            T.Update(5000.0, false, 0.1); // 2s out of range
        }
        T.Update(1500.0, false, 0.1); // back in range -> lost clock resets
        for (int32 I = 0; I < 30; ++I)
        {
            T.Update(5000.0, false, 0.1); // 3s out again; without the reset 2+3 > 4 would fail
        }
        TestTrue(TEXT("the lost clock reset on return"), T.IsTailing());
    }

    // ---- Terminal + dt guards -------------------------------------------------
    {
        FTailObjective Done;
        Done.Configure(P);
        for (int32 I = 0; I < 200 && Done.IsTailing(); ++I)
        {
            Done.Update(1500.0, false, 0.1);
        }
        TestTrue(TEXT("succeeded"), Done.IsSucceeded());
        Done.Update(200.0, true, 0.1); // no-op once finished
        TestTrue(TEXT("stays succeeded"), Done.IsSucceeded());

        FTailObjective Idle;
        Idle.Configure(P);
        Idle.Update(1500.0, true, 0.0);  // zero dt
        Idle.Update(1500.0, true, -3.0); // negative dt
        TestTrue(TEXT("non-positive dt does nothing"),
            FMath::Abs(Idle.Suspicion()) <= Eps && FMath::Abs(Idle.TrackProgress()) <= Eps);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
