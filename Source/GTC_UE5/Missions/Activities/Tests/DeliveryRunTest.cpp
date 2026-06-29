// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../DeliveryRun.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FDeliveryRun — the timed courier mission. GTC-original (no Godot oracle).
 * Pins a flawless run paying ~full, a slow delivery paying ~half, a battered delivery
 * docked proportionally, running out of time (too slow), wrecking the cargo, arrival
 * on the buzzer counting, terminal states, and dt guards. Prefix
 * GTC.Missions.Activities.Delivery.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDeliveryRunTest,
    "GTC.Missions.Activities.Delivery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDeliveryRunTest::RunTest(const FString& Parameters)
{
    FDeliveryRun::FParams P;
    P.TimeLimitSeconds = 120.0;

    // ---- Fresh run ------------------------------------------------------------
    {
        FDeliveryRun D;
        D.Configure(P);
        TestTrue(TEXT("in progress"), D.IsInProgress());
        TestTrue(TEXT("full clock"), FMath::IsNearlyEqual(D.TimeLeft(), 120.0, Eps));
        TestTrue(TEXT("no progress"), FMath::Abs(D.Progress()) <= Eps);
        TestTrue(TEXT("cargo intact"), FMath::IsNearlyEqual(D.CargoCondition(), 1.0, Eps));
        TestTrue(TEXT("no payout yet"), FMath::Abs(D.PayoutFactor()) <= Eps);
    }

    // ---- Flawless run pays nearly full ----------------------------------------
    {
        FDeliveryRun D;
        D.Configure(P);
        D.Update(1.0, 0.0, 1.0); // arrive immediately, intact, 119s to spare
        TestTrue(TEXT("delivered"), D.IsDelivered());
        TestTrue(TEXT("flawless pays ~full"), D.PayoutFactor() > 0.99);
    }

    // ---- Slow but careful pays about half -------------------------------------
    {
        FDeliveryRun D;
        D.Configure(P);
        for (int32 I = 0; I < 119; ++I)
        {
            D.Update(0.0, 0.0, 1.0); // burn the clock down to 1s, no progress
        }
        D.Update(1.0, 0.0, 1.0); // arrive as the clock hits 0
        TestTrue(TEXT("delivered on the buzzer"), D.IsDelivered());
        TestTrue(TEXT("slow run pays ~half"), FMath::IsNearlyEqual(D.PayoutFactor(), 0.5, 1e-2));
    }

    // ---- Battered cargo is docked in proportion -------------------------------
    {
        FDeliveryRun D;
        D.Configure(P);
        D.Update(0.0, 0.5, 1.0); // lose half the cargo
        TestTrue(TEXT("cargo halved"), FMath::IsNearlyEqual(D.CargoCondition(), 0.5, Eps));
        D.Update(1.0, 0.0, 1.0); // deliver fast with half cargo
        TestTrue(TEXT("delivered battered"), D.IsDelivered());
        // ~half of a flawless run's payout.
        TestTrue(TEXT("battered payout is ~0.5x"), D.PayoutFactor() < 0.55 && D.PayoutFactor() > 0.45);
    }

    // ---- Out of time before arriving = too slow -------------------------------
    {
        FDeliveryRun D;
        D.Configure(P);
        for (int32 I = 0; I < 130 && D.IsInProgress(); ++I)
        {
            D.Update(0.001, 0.0, 1.0); // dribble of progress, never arrives
        }
        TestTrue(TEXT("ran out of time"), D.IsTooSlow());
        TestTrue(TEXT("no payout for a failure"), FMath::Abs(D.PayoutFactor()) <= Eps);
    }

    // ---- Wrecking the cargo fails immediately, before progress/time -----------
    {
        FDeliveryRun D;
        D.Configure(P);
        D.Update(0.5, 1.0, 1.0); // would-be progress, but cargo destroyed this tick
        TestTrue(TEXT("wrecked"), D.IsWrecked());
        TestTrue(TEXT("no payout when wrecked"), FMath::Abs(D.PayoutFactor()) <= Eps);
    }

    // ---- Terminal + dt guards -------------------------------------------------
    {
        FDeliveryRun Done;
        Done.Configure(P);
        Done.Update(1.0, 0.0, 1.0); // delivered
        const double Pay = Done.PayoutFactor();
        Done.Update(0.0, 1.0, 1.0); // no-op once finished (can't wreck a delivered run)
        TestTrue(TEXT("stays delivered"), Done.IsDelivered());
        TestTrue(TEXT("payout frozen"), FMath::IsNearlyEqual(Done.PayoutFactor(), Pay, Eps));

        FDeliveryRun Idle;
        Idle.Configure(P);
        Idle.Update(1.0, 1.0, 0.0);  // zero dt
        Idle.Update(1.0, 1.0, -5.0); // negative dt
        TestTrue(TEXT("non-positive dt does nothing"),
            Idle.IsInProgress() && FMath::Abs(Idle.Progress()) <= Eps
            && FMath::IsNearlyEqual(Idle.CargoCondition(), 1.0, Eps));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
