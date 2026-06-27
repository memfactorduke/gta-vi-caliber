// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleSpawnSettle.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FVehicleSpawnSettle::Evaluate — the deterministic core that stops a car
 * launching when a World-Partition road streams in under it. The engine glue (the
 * downward trace + teleport + velocity reset on the live car) needs a UWorld and so is
 * DEFERRED-TO-PIE, exactly like the seat component's possess/attach glue. Pins: while
 * ground is missing the car is held a clearance ABOVE its spawn; the frame ground
 * appears it settles to just above the surface and is DONE; a real hit beats the
 * timeout; and only the timeout (never a normal wait) gives up.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleSpawnSettleTest,
    "GTC.Vehicles.Spawn.Settle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleSpawnSettleTest::RunTest(const FString& Parameters)
{
    using FSettle = FVehicleSpawnSettle;

    FSettle::FParams P;
    P.HoldClearanceCm = 300.0;
    P.SettleAboveGroundCm = 25.0;
    P.MaxWaitSeconds = 5.0;

    // ---- Ground not streamed in yet, still within the wait window -> HOLD ---------
    {
        const FSettle::FResult R = FSettle::Evaluate(/*hit*/ false, /*groundZ*/ 0.0,
            /*spawnZ*/ 120.0, /*elapsed*/ 0.5, P);

        TestTrue(TEXT("no ground -> Hold"), R.Action == FSettle::EAction::Hold);
        TestTrue(TEXT("held a clearance above spawn (120 + 300)"),
            FMath::IsNearlyEqual(R.TargetZ, 420.0, Eps));
        TestFalse(TEXT("a hold is never done"), R.bDone);
    }

    // ---- Ground streams in -> SETTLE just above it and finish ---------------------
    {
        const FSettle::FResult R = FSettle::Evaluate(/*hit*/ true, /*groundZ*/ 55.0,
            /*spawnZ*/ 120.0, /*elapsed*/ 0.7, P);

        TestTrue(TEXT("ground found -> Settle"), R.Action == FSettle::EAction::Settle);
        TestTrue(TEXT("settled the gap above the surface (55 + 25)"),
            FMath::IsNearlyEqual(R.TargetZ, 80.0, Eps));
        TestTrue(TEXT("a settle is done"), R.bDone);
    }

    // ---- Ground never appears -> after the timeout, GIVE UP to physics ------------
    {
        const FSettle::FResult R = FSettle::Evaluate(/*hit*/ false, /*groundZ*/ 0.0,
            /*spawnZ*/ 120.0, /*elapsed*/ 5.0, P);

        TestTrue(TEXT("timeout with no ground -> GiveUp"), R.Action == FSettle::EAction::GiveUp);
        TestTrue(TEXT("a give-up is done"), R.bDone);
    }

    // ---- A real hit wins even past the timeout (settle beats give-up) -------------
    {
        const FSettle::FResult R = FSettle::Evaluate(/*hit*/ true, /*groundZ*/ 10.0,
            /*spawnZ*/ 0.0, /*elapsed*/ 99.0, P);

        TestTrue(TEXT("hit wins over timeout"), R.Action == FSettle::EAction::Settle);
        TestTrue(TEXT("settle Z is hit + gap (10 + 25)"),
            FMath::IsNearlyEqual(R.TargetZ, 35.0, Eps));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
