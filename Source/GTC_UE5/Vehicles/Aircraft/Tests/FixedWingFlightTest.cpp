// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../FixedWingFlight.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FFixedWingFlight — arcade fixed-wing dynamics. GTC-original (no Godot
 * oracle). Pins the stall sink with washed-out controls, throttle building airspeed
 * past the stall, climbing costing airspeed (and diving adding it), turn authority
 * scaling with airspeed, the velocity composition, and the input/dt clamps. Prefix
 * GTC.Vehicles.FixedWing.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFixedWingFlightTest,
    "GTC.Vehicles.FixedWing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFixedWingFlightTest::RunTest(const FString& Parameters)
{
    const FFixedWingFlight::FParams P; // thrust 500, drag 0.2, stall 800, sink 600,
                                       // cruise 1500, climb 700, climbCost 300, turn 0.8

    // Fly a plane up to cruising speed (throttle, wings level).
    auto UpToSpeed = [&](FFixedWingFlight& Plane)
    {
        Plane.Configure(P);
        for (int32 I = 0; I < 60; ++I)
        {
            Plane.Update(1.0, 0.0, 0.0, 0.1);
        }
    };

    // ---- A fresh plane is stalled and sinking ---------------------------------
    {
        FFixedWingFlight A;
        A.Configure(P);
        TestTrue(TEXT("starts stalled"), A.IsStalled());
        A.Update(0.0, 1.0, 0.0, 0.1); // full back-stick, but no airspeed
        TestTrue(TEXT("stalled plane sinks regardless of elevator"),
            FMath::IsNearlyEqual(A.ClimbRate(), -600.0, Eps));
        TestTrue(TEXT("still no climb from a stall"), A.ClimbRate() < 0.0);
    }

    // ---- Throttle builds airspeed out of the stall ----------------------------
    {
        FFixedWingFlight A;
        UpToSpeed(A);
        TestFalse(TEXT("flying now, not stalled"), A.IsStalled());
        TestTrue(TEXT("airspeed past the stall"), A.Airspeed() > P.StallSpeed);
    }

    // ---- Climbing costs airspeed; diving adds it ------------------------------
    {
        FFixedWingFlight Level;
        UpToSpeed(Level);
        FFixedWingFlight Climb;
        UpToSpeed(Climb);
        FFixedWingFlight Dive;
        UpToSpeed(Dive);
        // Same airspeed going in (identical build).
        TestTrue(TEXT("identical build airspeed"), FMath::IsNearlyEqual(Level.Airspeed(), Climb.Airspeed(), Eps));

        Level.Update(1.0, 0.0, 0.0, 0.1);
        Climb.Update(1.0, 1.0, 0.0, 0.1);
        Dive.Update(1.0, -1.0, 0.0, 0.1);

        TestTrue(TEXT("pulling up climbs"), Climb.ClimbRate() > 0.0);
        TestTrue(TEXT("level doesn't climb"), FMath::Abs(Level.ClimbRate()) <= Eps);
        TestTrue(TEXT("diving descends"), Dive.ClimbRate() < 0.0);
        TestTrue(TEXT("climbing bleeds airspeed vs level"), Climb.Airspeed() < Level.Airspeed());
        TestTrue(TEXT("diving gains airspeed vs level"), Dive.Airspeed() > Level.Airspeed());
    }

    // ---- Turn authority needs airspeed ----------------------------------------
    {
        FFixedWingFlight Slow;
        Slow.Configure(P); // stalled
        Slow.Update(0.0, 0.0, 1.0, 0.1); // full aileron, but stalled
        TestTrue(TEXT("a stalled plane barely turns"), FMath::Abs(Slow.Heading()) <= Eps);

        FFixedWingFlight Fast;
        UpToSpeed(Fast);
        const double H0 = Fast.Heading();
        Fast.Update(1.0, 0.0, 1.0, 0.1);
        TestTrue(TEXT("at speed it banks around"), Fast.Heading() > H0);
    }

    // ---- Velocity composition: level flight is horizontal along heading -------
    {
        FFixedWingFlight A;
        UpToSpeed(A); // heading 0, level
        const FVector V = A.Velocity();
        TestTrue(TEXT("velocity is forward (+X)"), FMath::IsNearlyEqual(V.X, A.Airspeed(), Eps));
        TestTrue(TEXT("no sideways velocity"), FMath::Abs(V.Y) <= Eps);
        TestTrue(TEXT("level flight has ~0 climb"), FMath::Abs(V.Z) <= Eps);
    }

    // ---- Input / dt clamps ----------------------------------------------------
    {
        FFixedWingFlight Over;
        Over.Configure(P);
        Over.Update(5.0, 0.0, 0.0, 0.1); // throttle clamps to 1
        FFixedWingFlight One;
        One.Configure(P);
        One.Update(1.0, 0.0, 0.0, 0.1);
        TestTrue(TEXT("over-range throttle clamps"), FMath::IsNearlyEqual(Over.Airspeed(), One.Airspeed(), Eps));

        FFixedWingFlight Idle;
        UpToSpeed(Idle);
        const double A0 = Idle.Airspeed();
        const double H0 = Idle.Heading();
        Idle.Update(1.0, 1.0, 1.0, 0.0);  // zero dt
        Idle.Update(1.0, 1.0, 1.0, -3.0); // negative dt
        TestTrue(TEXT("non-positive dt doesn't change airspeed"), FMath::IsNearlyEqual(Idle.Airspeed(), A0, Eps));
        TestTrue(TEXT("non-positive dt doesn't turn"), FMath::IsNearlyEqual(Idle.Heading(), H0, Eps));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
