// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleDriveResolver.h"
#include "../../Transmission/VehicleTransmission.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Tests for FVehicleDriveResolver — the input-shaping half of the drive-command pipeline (steering
 * deadzone/authority/smoothing + pedal auto-reverse), and the full pipeline driven end-to-end
 * through the real FVehicleTransmission. Prefix GTC.Vehicles.DriveResolver.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleDriveResolverTest,
    "GTC.Vehicles.DriveResolver",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

static FVehicleDriveResolver::FOutput Shape(double rawSteer, double rawThrottle, double rawBrake,
    double speed, double fwdSpeed, double prevSteer)
{
    FVehicleDriveResolver::FInput In;
    In.RawSteer = rawSteer;
    In.RawThrottle = rawThrottle;
    In.RawBrake = rawBrake;
    In.SpeedCmS = speed;
    In.ForwardSpeedCmS = fwdSpeed;
    In.PrevSteer = prevSteer;
    In.Dt = 0.1;
    return FVehicleDriveResolver::Shape(In);
}

static double ConvergeSteer(double rawSteer, double speed)
{
    double prev = 0.0;
    for (int32 i = 0; i < 60; ++i)
    {
        prev = Shape(rawSteer, 0.0, 0.0, speed, 0.0, prev).Steer;
    }
    return prev;
}

static void DriveTick(FVehicleTransmission& Tx, double& PrevSteer, double rawThrottle, double rawBrake,
    double fwdSpeed, double wheelRpm, const FVehicleTransmission::FParams& P)
{
    const FVehicleDriveResolver::FOutput S =
        Shape(0.0, rawThrottle, rawBrake, (fwdSpeed < 0 ? -fwdSpeed : fwdSpeed), fwdSpeed, PrevSteer);
    PrevSteer = S.Steer;
    FVehicleTransmission::FInputs In;
    In.Throttle = S.GearboxThrottle;
    In.Brake = S.Brake;
    In.Handbrake = 0.0;
    In.Steer = S.Steer;
    Tx.Update(In, wheelRpm, 0.1, P);
}

bool FVehicleDriveResolverTest::RunTest(const FString& Parameters)
{
    // --- Steering shaping. ---
    TestEqual(TEXT("no steer input -> no steer"), Shape(0.0, 0.0, 0.0, 0.0, 0.0, 0.0).Steer, 0.0);
    const double Slow = ConvergeSteer(1.0, 0.0);
    const double Fast = ConvergeSteer(1.0, 6000.0);
    TestTrue(TEXT("full steer at low speed -> near full lock"), Slow > 0.9);
    TestTrue(TEXT("steering authority falls with speed"), Fast < Slow - 0.05);
    TestTrue(TEXT("authority floor keeps the car steering flat out"), Fast > 0.0);

    // --- Pedals + auto-reverse. ---
    {
        const FVehicleDriveResolver::FOutput Fwd = Shape(0.0, 1.0, 0.0, 0.0, 0.0, 0.0);
        TestTrue(TEXT("throttle -> forward drive"), !Fwd.bReverse && Fwd.GearboxThrottle > 0.0);
        const FVehicleDriveResolver::FOutput Rev = Shape(0.0, 0.0, 1.0, 0.0, 0.0, 0.0);
        TestTrue(TEXT("brake at a standstill -> reverse (signed throttle)"), Rev.bReverse && Rev.GearboxThrottle < 0.0);
        const FVehicleDriveResolver::FOutput Braking = Shape(0.0, 0.0, 1.0, 2000.0, 2000.0, 0.0);
        TestTrue(TEXT("brake while rolling forward -> service brake"), !Braking.bReverse && Braking.Brake > 0.0);
    }

    FVehicleTransmission::FParams P;

    // --- Standstill -> Drive / Reverse. ---
    {
        FVehicleTransmission Tx;
        double Prev = 0.0;
        DriveTick(Tx, Prev, 1.0, 0.0, 0.0, 0.0, P);
        TestTrue(TEXT("throttle from a standstill selects Drive"), Tx.Gear() == FVehicleTransmission::EGear::Drive);
        TestTrue(TEXT("Drive with throttle delivers torque"), Tx.ThrottleOut() > 0.0);
    }
    {
        FVehicleTransmission Tx;
        double Prev = 0.0;
        DriveTick(Tx, Prev, 0.0, 1.0, 0.0, 0.0, P);
        TestTrue(TEXT("brake from a standstill selects Reverse"), Tx.Gear() == FVehicleTransmission::EGear::Reverse);
    }

    // --- Auto-shift up. ---
    {
        FVehicleTransmission Tx;
        double Prev = 0.0;
        DriveTick(Tx, Prev, 1.0, 0.0, 200.0, 100.0, P);
        const int32 LowGear = Tx.ForwardGearNumber();
        for (int32 i = 0; i < 4; ++i)
        {
            DriveTick(Tx, Prev, 1.0, 0.0, 4000.0, 700.0, P);
        }
        TestEqual(TEXT("starts in first gear"), LowGear, 1);
        TestTrue(TEXT("revving out first gear auto-shifts up"), Tx.ForwardGearNumber() > 1);
    }

    // --- Engine braking on coast. ---
    {
        FVehicleTransmission Tx;
        double Prev = 0.0;
        DriveTick(Tx, Prev, 1.0, 0.0, 3000.0, 300.0, P);
        DriveTick(Tx, Prev, 0.0, 0.0, 3000.0, 300.0, P);
        TestTrue(TEXT("coasting off-throttle in gear applies engine braking"), Tx.BrakeOut() > 0.0);
    }

    // --- Wheel-RPM estimate (the shell's speed -> RPM math). ---
    TestTrue(TEXT("wheel RPM from forward speed matches 2*pi*r circumference"),
        FMath::IsNearlyEqual(FVehicleDriveResolver::WheelRpmFromForwardSpeed(50.0, 35.0),
            50.0 / (6.283185307179586 * 35.0) * 60.0, 1e-6));
    TestTrue(TEXT("reversing gives negative wheel RPM"),
        FVehicleDriveResolver::WheelRpmFromForwardSpeed(-1000.0, 35.0) < 0.0);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
