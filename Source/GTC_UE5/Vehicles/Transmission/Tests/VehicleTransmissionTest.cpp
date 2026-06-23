// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleTransmission.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FVehicleTransmission — the player-input -> gear/RPM -> actuator bridge.
 * GTC-original (no Godot oracle). Pins input clamping, the standstill direction
 * pick (Drive / Reverse / Neutral / Parked), the forward auto-shift on RPM, the
 * derived engine RPM bounds, engine braking on coast, and the actuator outputs the
 * ChaosVehicle adapter feeds the solver. Prefix GTC.Vehicles.Transmission.
 */

namespace
{
    using FInputs = FVehicleTransmission::FInputs;

    FInputs In(double Throttle = 0.0, double Brake = 0.0, double Handbrake = 0.0, double Steer = 0.0)
    {
        FInputs I;
        I.Throttle = Throttle;
        I.Brake = Brake;
        I.Handbrake = Handbrake;
        I.Steer = Steer;
        return I;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleTransmissionTest,
    "GTC.Vehicles.Transmission",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleTransmissionTest::RunTest(const FString& Parameters)
{
    using EGear = FVehicleTransmission::EGear;
    const FVehicleTransmission::FParams P; // Idle 800, Redline 7000, ShiftUp 6200, ShiftDown 2200,
                                           // ratios {12,7,4.5}, ReverseRatio 12, standstill 5 rpm

    // ---- A fresh box idles in Neutral with no demand --------------------------
    {
        FVehicleTransmission T;
        T.Update(In(), 0.0, 0.016, P);
        TestTrue(TEXT("fresh box is Neutral"), T.Gear() == EGear::Neutral);
        TestFalse(TEXT("Neutral is not a driving gear"), T.IsInDrivingGear());
        TestTrue(TEXT("Neutral idles at IdleRpm"), FMath::IsNearlyEqual(T.Rpm(), P.IdleRpm, Eps));
        TestTrue(TEXT("no throttle out in Neutral"), FMath::Abs(T.ThrottleOut()) <= Eps);
        TestTrue(TEXT("no brake out with no input"), FMath::Abs(T.BrakeOut()) <= Eps);
    }

    // ---- Input clamping (out-of-range axes are pinned to their domains) --------
    {
        FVehicleTransmission T;
        // Throttle 2.0 -> 1.0 forward demand; steer 3.0 -> 1.0; brake/handbrake -2 -> 0.
        T.Update(In(/*throttle*/ 2.0, /*brake*/ -2.0, /*handbrake*/ -2.0, /*steer*/ 3.0), 0.0, 0.016, P);
        TestTrue(TEXT("over-range throttle clamps to full"), FMath::IsNearlyEqual(T.ThrottleOut(), 1.0, Eps));
        TestTrue(TEXT("over-range steer clamps to +1"), FMath::IsNearlyEqual(T.SteerOut(), 1.0, Eps));
        TestTrue(TEXT("negative brake clamps to 0"), FMath::Abs(T.BrakeOut()) <= Eps);
        TestTrue(TEXT("negative handbrake clamps to 0"), FMath::Abs(T.HandbrakeOut()) <= Eps);

        T.Update(In(/*throttle*/ -5.0, 0.0, 0.0, /*steer*/ -9.0), 0.0, 0.016, P);
        TestTrue(TEXT("under-range steer clamps to -1"), FMath::IsNearlyEqual(T.SteerOut(), -1.0, Eps));
    }

    // ---- Standstill direction pick --------------------------------------------
    {
        // Throttle from a stop engages Drive in first gear.
        FVehicleTransmission T;
        T.Update(In(/*throttle*/ 1.0), 0.0, 0.016, P);
        TestTrue(TEXT("throttle from stop engages Drive"), T.Gear() == EGear::Drive);
        TestEqual(TEXT("pulls away in first gear"), T.ForwardGearNumber(), 1);
        TestTrue(TEXT("Drive is a driving gear"), T.IsInDrivingGear());
        TestTrue(TEXT("full throttle out in Drive"), FMath::IsNearlyEqual(T.ThrottleOut(), 1.0, Eps));
    }
    {
        // Negative throttle axis from a stop engages Reverse and drives it.
        FVehicleTransmission T;
        T.Update(In(/*throttle*/ -1.0), 0.0, 0.016, P);
        TestTrue(TEXT("back-throttle from stop engages Reverse"), T.Gear() == EGear::Reverse);
        TestTrue(TEXT("reverse drive comes from the back axis"), FMath::IsNearlyEqual(T.ThrottleOut(), 1.0, Eps));
        TestTrue(TEXT("no foot brake applied in a clean reverse"), FMath::Abs(T.BrakeOut()) <= Eps);
    }
    {
        // Foot brake from a stop selects Reverse but must NOT drive backward (throttle 0).
        FVehicleTransmission T;
        T.Update(In(/*throttle*/ 0.0, /*brake*/ 1.0), 0.0, 0.016, P);
        TestTrue(TEXT("brake from stop selects Reverse"), T.Gear() == EGear::Reverse);
        TestTrue(TEXT("brake alone never accelerates a reverse"), FMath::Abs(T.ThrottleOut()) <= Eps);
        TestTrue(TEXT("the brake still brakes"), FMath::IsNearlyEqual(T.BrakeOut(), 1.0, Eps));
    }
    {
        // A feather touch inside the select deadzone leaves it in Neutral.
        FVehicleTransmission T;
        T.Update(In(/*throttle*/ P.SelectDeadzone * 0.5), 0.0, 0.016, P);
        TestTrue(TEXT("sub-deadzone throttle stays Neutral"), T.Gear() == EGear::Neutral);
    }

    // ---- Parking and releasing the park ---------------------------------------
    {
        FVehicleTransmission T;
        // Handbrake at a standstill with no go-input parks the car.
        T.Update(In(/*throttle*/ 0.0, /*brake*/ 0.0, /*handbrake*/ 1.0), 0.0, 0.016, P);
        TestTrue(TEXT("handbrake at rest parks"), T.Gear() == EGear::Parked);
        TestTrue(TEXT("Parked forces full brake hold"), FMath::IsNearlyEqual(T.BrakeOut(), 1.0, Eps));
        TestTrue(TEXT("Parked forces the handbrake on"), FMath::IsNearlyEqual(T.HandbrakeOut(), 1.0, Eps));
        TestTrue(TEXT("Parked delivers no drive"), FMath::Abs(T.ThrottleOut()) <= Eps);
        TestFalse(TEXT("Parked is not a driving gear"), T.IsInDrivingGear());

        // Holding the handbrake another frame keeps it parked (latched).
        T.Update(In(/*throttle*/ 0.0, /*brake*/ 0.0, /*handbrake*/ 1.0), 0.0, 0.016, P);
        TestTrue(TEXT("park is latched while held"), T.Gear() == EGear::Parked);

        // Throttle releases the park and drives away.
        T.Update(In(/*throttle*/ 1.0, /*brake*/ 0.0, /*handbrake*/ 0.0), 0.0, 0.016, P);
        TestTrue(TEXT("throttle releases the park into Drive"), T.Gear() == EGear::Drive);
    }

    // ---- Derived engine RPM + forward auto-shift ------------------------------
    {
        // First gear (ratio 12) at a wheel speed under the up-shift point stays in first.
        // 12 * 400 = 4800 rpm, between ShiftDown(2200) and ShiftUp(6200).
        FVehicleTransmission T;
        T.Update(In(/*throttle*/ 1.0), /*wheelRpm*/ 400.0, 0.016, P);
        TestTrue(TEXT("rolling forward is Drive"), T.Gear() == EGear::Drive);
        TestEqual(TEXT("stays in first below the up-shift"), T.ForwardGearNumber(), 1);
        TestTrue(TEXT("engine rpm = wheelRpm * ratio"), FMath::IsNearlyEqual(T.Rpm(), 400.0 * 12.0, 1.0));
    }
    {
        // Revving first gear past ShiftUpRpm up-shifts to second.
        // 12 * 540 = 6480 >= 6200 -> shift up; second gear 7 * 540 = 3780.
        FVehicleTransmission T;
        T.Update(In(/*throttle*/ 1.0), /*wheelRpm*/ 540.0, 0.016, P);
        TestEqual(TEXT("crossing ShiftUpRpm up-shifts to second"), T.ForwardGearNumber(), 2);
        TestTrue(TEXT("rpm recomputed in the new gear"), FMath::IsNearlyEqual(T.Rpm(), 540.0 * 7.0, 1.0));
    }
    {
        // From second gear, bogging below ShiftDownRpm drops back to first.
        // Step 1: 540 rpm -> second (as above). Step 2: 250 rpm -> 7*250=1750 <= 2200 -> first.
        FVehicleTransmission T;
        T.Update(In(/*throttle*/ 1.0), 540.0, 0.016, P);
        TestEqual(TEXT("now in second"), T.ForwardGearNumber(), 2);
        T.Update(In(/*throttle*/ 1.0), 250.0, 0.016, P);
        TestEqual(TEXT("bogging down drops back to first"), T.ForwardGearNumber(), 1);
    }
    {
        // Engine RPM is clamped to [Idle, Redline] no matter the wheel speed.
        FVehicleTransmission T;
        T.Update(In(/*throttle*/ 1.0), /*wheelRpm*/ 100000.0, 0.016, P);
        TestTrue(TEXT("engine rpm never exceeds redline"), T.Rpm() <= P.RedlineRpm + Eps);
        TestEqual(TEXT("a screaming wheel ends in top gear"), T.ForwardGearNumber(), P.NumForwardGears);

        FVehicleTransmission T2;
        T2.Update(In(/*throttle*/ 1.0), /*wheelRpm*/ 0.0, 0.016, P);
        TestTrue(TEXT("engine rpm never drops below idle"), T2.Rpm() >= P.IdleRpm - Eps);
    }

    // ---- Engine braking on coast ----------------------------------------------
    {
        // Rolling forward in Drive, off the throttle, no foot brake: engine braking shows.
        FVehicleTransmission T;
        T.Update(In(/*throttle*/ 1.0), /*wheelRpm*/ 400.0, 0.016, P); // establish Drive while rolling
        T.Update(In(/*throttle*/ 0.0, /*brake*/ 0.0), /*wheelRpm*/ 400.0, 0.016, P);
        TestTrue(TEXT("coasting still in Drive"), T.Gear() == EGear::Drive);
        TestTrue(TEXT("no throttle out while coasting"), FMath::Abs(T.ThrottleOut()) <= Eps);
        TestTrue(TEXT("engine braking applies a partial brake"),
            FMath::IsNearlyEqual(T.BrakeOut(), P.EngineBrakeStrength, Eps));

        // A real foot brake overrides engine braking when stronger.
        T.Update(In(/*throttle*/ 0.0, /*brake*/ 0.8), /*wheelRpm*/ 400.0, 0.016, P);
        TestTrue(TEXT("foot brake dominates the weaker engine brake"),
            FMath::IsNearlyEqual(T.BrakeOut(), 0.8, Eps));
    }

    // ---- Foot brake while rolling does NOT flip to Reverse --------------------
    {
        // Established in Drive and still rolling: stamping the brake brakes, stays in Drive
        // (only a true standstill may flip direction).
        FVehicleTransmission T;
        T.Update(In(/*throttle*/ 1.0), /*wheelRpm*/ 400.0, 0.016, P);
        T.Update(In(/*throttle*/ 0.0, /*brake*/ 1.0), /*wheelRpm*/ 400.0, 0.016, P);
        TestTrue(TEXT("braking a moving car keeps it in Drive"), T.Gear() == EGear::Drive);
        TestTrue(TEXT("the brake is applied"), FMath::IsNearlyEqual(T.BrakeOut(), 1.0, Eps));
    }

    // ---- Dt is ignored (rate-free): same inputs -> same outputs ---------------
    {
        FVehicleTransmission A;
        FVehicleTransmission B;
        A.Update(In(/*throttle*/ 1.0), 540.0, /*dt*/ 0.001, P);
        B.Update(In(/*throttle*/ 1.0), 540.0, /*dt*/ 10.0, P);
        TestEqual(TEXT("gear independent of Dt"), A.ForwardGearNumber(), B.ForwardGearNumber());
        TestTrue(TEXT("rpm independent of Dt"), FMath::IsNearlyEqual(A.Rpm(), B.Rpm(), Eps));
        // A negative Dt is harmless.
        FVehicleTransmission C;
        C.Update(In(/*throttle*/ 1.0), 540.0, /*dt*/ -5.0, P);
        TestEqual(TEXT("negative Dt still resolves the gear"), C.ForwardGearNumber(), B.ForwardGearNumber());
    }

    // ---- A single-gear (degenerate) box never shifts --------------------------
    {
        FVehicleTransmission::FParams One = P;
        One.NumForwardGears = 1;
        FVehicleTransmission T;
        T.Update(In(/*throttle*/ 1.0), /*wheelRpm*/ 100000.0, 0.016, One);
        TestEqual(TEXT("a one-speed box stays in gear 1"), T.ForwardGearNumber(), 1);
        TestTrue(TEXT("one-speed box is in Drive"), T.Gear() == EGear::Drive);
    }

    // ---- Coasting to a halt with no input returns to Neutral ------------------
    {
        FVehicleTransmission T;
        T.Update(In(/*throttle*/ 1.0), 400.0, 0.016, P); // Drive while rolling
        T.Update(In(/*throttle*/ 0.0), /*wheelRpm*/ 0.0, 0.016, P); // stopped, no input
        TestTrue(TEXT("stopping with no input returns to Neutral"), T.Gear() == EGear::Neutral);
        TestTrue(TEXT("Neutral idles after the stop"), FMath::IsNearlyEqual(T.Rpm(), P.IdleRpm, Eps));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
