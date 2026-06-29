// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../BoatHandling.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FBoatHandling — arcade watercraft dynamics. GTC-original (no Godot
 * oracle). Pins the first-tick surge, reverse, getting onto plane, no-steer-at-rest
 * (rudder needs flow), steering once moving, the drift-then-grip slide, a bounded
 * terminal speed, and the input/dt clamps. Prefix GTC.Vehicles.Watercraft.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBoatHandlingTest,
    "GTC.Vehicles.Watercraft",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBoatHandlingTest::RunTest(const FString& Parameters)
{
    const FBoatHandling::FParams P; // engine 800, plow 0.9, plane 0.3, lateral 3.0,
                                    // planingSpeed 600, rudder 1.2, flowSpeed 300

    // ---- Fresh boat at rest ---------------------------------------------------
    {
        FBoatHandling B;
        B.Configure(P);
        TestTrue(TEXT("at rest"), B.Velocity().Size() <= Eps);
        TestTrue(TEXT("no forward speed"), FMath::Abs(B.ForwardSpeed()) <= Eps);
        TestFalse(TEXT("not planing"), B.IsPlaning());
    }

    // ---- Throttle surges forward; reverse goes backward -----------------------
    {
        FBoatHandling Fwd;
        Fwd.Configure(P);
        Fwd.Update(1.0, 0.0, 0.1); // 800 * 0.1 = 80 cm/s
        TestTrue(TEXT("throttle surges to 80"), FMath::IsNearlyEqual(Fwd.ForwardSpeed(), 80.0, Eps));

        FBoatHandling Rev;
        Rev.Configure(P);
        Rev.Update(-1.0, 0.0, 0.1);
        TestTrue(TEXT("reverse goes backward"), FMath::IsNearlyEqual(Rev.ForwardSpeed(), -80.0, Eps));
    }

    // ---- Held throttle climbs onto plane --------------------------------------
    {
        FBoatHandling B;
        B.Configure(P);
        for (int32 I = 0; I < 60 && !B.IsPlaning(); ++I)
        {
            B.Update(1.0, 0.0, 0.1);
        }
        TestTrue(TEXT("gets on plane"), B.IsPlaning());
        TestTrue(TEXT("planing speed is past the threshold"), B.ForwardSpeed() > P.PlaningSpeed);
    }

    // ---- The rudder needs flow: no steering at rest ---------------------------
    {
        FBoatHandling B;
        B.Configure(P);
        B.Update(0.0, 1.0, 0.2); // hard steer, but dead in the water
        TestTrue(TEXT("a stopped boat won't turn"), FMath::Abs(B.Heading()) <= Eps);

        // Build some speed, then it turns.
        for (int32 I = 0; I < 20; ++I)
        {
            B.Update(1.0, 0.0, 0.1);
        }
        B.Update(1.0, 1.0, 0.1);
        TestTrue(TEXT("moving, the rudder bites"), B.Heading() > 0.0);
    }

    // ---- Drift then grip: a hard turn slides, then the hull bites -------------
    {
        FBoatHandling B;
        B.Configure(P);
        for (int32 I = 0; I < 40; ++I) // get up to speed, dead ahead
        {
            B.Update(1.0, 0.0, 0.1);
        }
        TestTrue(TEXT("no slip going straight"), FMath::Abs(B.LateralSpeed()) <= 1.0);

        for (int32 I = 0; I < 5; ++I) // crank the wheel: the stern slides out
        {
            B.Update(1.0, 1.0, 0.1);
        }
        const double Slip = FMath::Abs(B.LateralSpeed());
        TestTrue(TEXT("the hull slides in a hard turn"), Slip > 50.0);

        for (int32 I = 0; I < 8; ++I) // straighten: lateral drag bites the slide back
        {
            B.Update(1.0, 0.0, 0.1);
        }
        TestTrue(TEXT("the slide grips back"), FMath::Abs(B.LateralSpeed()) < Slip);
    }

    // ---- Terminal speed is bounded (drag caps it) -----------------------------
    {
        FBoatHandling B;
        B.Configure(P);
        for (int32 I = 0; I < 400; ++I)
        {
            B.Update(1.0, 0.0, 0.1);
        }
        // On-plane terminal ~ EngineAccel / ForwardDragPlaning = 2667 cm/s.
        TestTrue(TEXT("terminal is bounded"), B.ForwardSpeed() < 2700.0);
        TestTrue(TEXT("terminal is fast (on plane)"), B.ForwardSpeed() > P.PlaningSpeed);
    }

    // ---- Input / dt clamps ----------------------------------------------------
    {
        FBoatHandling Over;
        Over.Configure(P);
        Over.Update(5.0, 0.0, 0.1); // throttle clamps to 1 -> 80
        TestTrue(TEXT("over-range throttle clamps"), FMath::IsNearlyEqual(Over.ForwardSpeed(), 80.0, Eps));

        FBoatHandling Idle;
        Idle.Configure(P);
        Idle.Update(1.0, 1.0, 0.0);  // zero dt
        Idle.Update(1.0, 1.0, -2.0); // negative dt
        TestTrue(TEXT("non-positive dt doesn't move it"), Idle.Velocity().Size() <= Eps);
        TestTrue(TEXT("non-positive dt doesn't turn it"), FMath::Abs(Idle.Heading()) <= Eps);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
