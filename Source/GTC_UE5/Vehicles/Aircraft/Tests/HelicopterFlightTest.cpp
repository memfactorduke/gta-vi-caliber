// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../HelicopterFlight.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FHelicopterFlight — arcade rotary-wing dynamics. GTC-original (no Godot
 * oracle). Pins the hover collective (lift cancels gravity), climb/descend on
 * collective, forward/lateral motion from cyclic in the heading frame, yaw from
 * pedal, heading coupling the thrust direction, drag bleeding momentum, and the
 * input/dt clamps. Prefix GTC.Vehicles.Helicopter. First-tick checks start from rest
 * (zero velocity => zero drag) so the numbers are exact.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHelicopterFlightTest,
    "GTC.Vehicles.Helicopter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHelicopterFlightTest::RunTest(const FString& Parameters)
{
    const FHelicopterFlight::FParams P; // g 980, maxLift 2000, tilt 1200, yaw 1.5, drag 0.5

    // ---- Hover collective cancels gravity -------------------------------------
    {
        FHelicopterFlight H;
        H.Configure(P);
        TestTrue(TEXT("hover collective is g/maxLift = 0.49"), FMath::IsNearlyEqual(H.HoverCollective(), 0.49, Eps));
        for (int32 I = 0; I < 50; ++I)
        {
            H.Update(H.HoverCollective(), 0.0, 0.0, 0.0, 0.1);
        }
        TestTrue(TEXT("hovering stays at rest"), H.Velocity().Size() <= 1e-3);
        TestTrue(TEXT("heading unchanged at hover"), FMath::Abs(H.Heading()) <= Eps);
    }

    // ---- Collective climbs and descends ---------------------------------------
    {
        FHelicopterFlight Up;
        Up.Configure(P);
        Up.Update(1.0, 0.0, 0.0, 0.0, 0.1); // vAccel = 2000-980 = 1020 -> +102 cm/s
        TestTrue(TEXT("full collective climbs"), FMath::IsNearlyEqual(Up.Velocity().Z, 102.0, Eps));

        FHelicopterFlight Down;
        Down.Configure(P);
        Down.Update(0.0, 0.0, 0.0, 0.0, 0.1); // vAccel = -980 -> -98 cm/s
        TestTrue(TEXT("no collective sinks"), FMath::IsNearlyEqual(Down.Velocity().Z, -98.0, Eps));
    }

    // ---- Cyclic pitch drives forward (in the heading frame) -------------------
    {
        FHelicopterFlight H;
        H.Configure(P);
        H.Update(H.HoverCollective(), -1.0, 0.0, 0.0, 0.1); // nose down -> +1200 fwd, heading 0 -> +X
        const FVector V = H.Velocity();
        TestTrue(TEXT("accelerates forward (+X)"), FMath::IsNearlyEqual(V.X, 120.0, Eps));
        TestTrue(TEXT("no lateral drift"), FMath::Abs(V.Y) <= Eps);
        TestTrue(TEXT("stays level (hover collective)"), FMath::Abs(V.Z) <= Eps);
    }

    // ---- Cyclic roll drives sideways ------------------------------------------
    {
        FHelicopterFlight H;
        H.Configure(P);
        H.Update(H.HoverCollective(), 0.0, 1.0, 0.0, 0.1); // bank right -> right vector (0,-1,0) at heading 0
        const FVector V = H.Velocity();
        TestTrue(TEXT("accelerates sideways (Y)"), FMath::IsNearlyEqual(V.Y, -120.0, Eps));
        TestTrue(TEXT("no forward drift"), FMath::Abs(V.X) <= Eps);
    }

    // ---- Pedal yaws the heading ----------------------------------------------
    {
        FHelicopterFlight H;
        H.Configure(P);
        H.Update(H.HoverCollective(), 0.0, 0.0, 1.0, 0.1); // 1.5 rad/s * 0.1 = 0.15
        TestTrue(TEXT("pedal turns the heading"), FMath::IsNearlyEqual(H.Heading(), 0.15, Eps));
    }

    // ---- Heading couples the thrust direction ---------------------------------
    {
        FHelicopterFlight H;
        H.Configure(P);
        H.Update(H.HoverCollective(), 0.0, 0.0, 1.0, (PI / 2.0) / 1.5); // yaw exactly +90 deg, still at rest
        TestTrue(TEXT("now facing +Y"), FMath::IsNearlyEqual(H.Heading(), PI / 2.0, Eps));
        H.Update(H.HoverCollective(), -1.0, 0.0, 0.0, 0.1); // forward now points +Y
        const FVector V = H.Velocity();
        TestTrue(TEXT("forward thrust now drives +Y"), FMath::IsNearlyEqual(V.Y, 120.0, Eps));
        TestTrue(TEXT("almost no +X component"), FMath::Abs(V.X) <= Eps);
    }

    // ---- Drag bleeds momentum when you neutralize -----------------------------
    {
        FHelicopterFlight H;
        H.Configure(P);
        H.Update(H.HoverCollective(), -1.0, 0.0, 0.0, 0.1); // gain +120 forward
        const double Fast = H.Velocity().X;
        H.Update(H.HoverCollective(), 0.0, 0.0, 0.0, 0.1);  // neutral -> drag = -vel*0.5
        TestTrue(TEXT("drag slows the chopper"), H.Velocity().X < Fast);
        TestTrue(TEXT("decel is drag (-6 cm/s this tick)"), FMath::IsNearlyEqual(H.Velocity().X, 114.0, Eps));
    }

    // ---- Input / dt clamps ----------------------------------------------------
    {
        FHelicopterFlight Over;
        Over.Configure(P);
        Over.Update(5.0, 0.0, 0.0, 0.0, 0.1); // collective clamps to 1 -> +102

        FHelicopterFlight Full;
        Full.Configure(P);
        Full.Update(1.0, 0.0, 0.0, 0.0, 0.1);
        TestTrue(TEXT("over-range collective clamps to full"),
            FMath::IsNearlyEqual(Over.Velocity().Z, Full.Velocity().Z, Eps));

        FHelicopterFlight Idle;
        Idle.Configure(P);
        Idle.Update(1.0, -1.0, 1.0, 1.0, 0.0);  // zero dt
        Idle.Update(1.0, -1.0, 1.0, 1.0, -3.0); // negative dt
        TestTrue(TEXT("non-positive dt doesn't move it"), Idle.Velocity().Size() <= Eps);
        TestTrue(TEXT("non-positive dt doesn't yaw it"), FMath::Abs(Idle.Heading()) <= Eps);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
