// Out-of-tree oracle for FVehicleDriveResolver — compiles + runs the REAL VehicleDriveResolver.cpp
// against the REAL VehicleDriveInput.cpp + VehicleTransmission.cpp and drives the full player-input
// pipeline the live UGTCVehicleDriveComponent runs: raw Enhanced Input axes -> shaped steering +
// pedals (auto-reverse) -> the stateful 3-speed gearbox -> actuators + gear/RPM.
#include "../../../Source/GTC_UE5/Vehicles/Drive/VehicleDriveResolver.h"
#include "../../../Source/GTC_UE5/Vehicles/Drive/VehicleDriveInput.h"
#include "../../../Source/GTC_UE5/Vehicles/Transmission/VehicleTransmission.h"
#include "harness.h"

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

// Converge the smoothed steer over N ticks of a held raw steer at a fixed speed.
static double ConvergeSteer(double rawSteer, double speed)
{
    double prev = 0.0;
    for (int i = 0; i < 60; ++i)
    {
        prev = Shape(rawSteer, 0.0, 0.0, speed, 0.0, prev).Steer;
    }
    return prev;
}

// One combined pipeline tick: shape raw input, feed the transmission with an explicit wheel RPM.
static void DriveTick(FVehicleTransmission& Tx, double& PrevSteer, double rawThrottle, double rawBrake,
    double handbrake, double fwdSpeed, double wheelRpm, const FVehicleTransmission::FParams& P)
{
    const auto S = Shape(0.0, rawThrottle, rawBrake, (fwdSpeed < 0 ? -fwdSpeed : fwdSpeed), fwdSpeed, PrevSteer);
    PrevSteer = S.Steer;
    FVehicleTransmission::FInputs Tx2;
    Tx2.Throttle = S.GearboxThrottle;
    Tx2.Brake = S.Brake;
    Tx2.Handbrake = handbrake;
    Tx2.Steer = S.Steer;
    Tx.Update(Tx2, wheelRpm, 0.1, P);
}

int main()
{
    // --- Steering shaping: deadzone, smoothing, speed-sensitive authority. ---
    {
        Check(Shape(0.0, 0.0, 0.0, 0.0, 0.0, 0.0).Steer == 0.0, "no steer input -> no steer");
        const double Slow = ConvergeSteer(1.0, 0.0);      // full lock at parking speed
        const double Fast = ConvergeSteer(1.0, 6000.0);   // full lock flat out
        Check(Slow > 0.9, "full steer at low speed converges to near full lock");
        Check(Fast < Slow - 0.05, "steering authority falls with speed (calmer at speed)");
        Check(Fast > 0.0, "even flat out the car still steers (authority floor)");
    }

    // --- Pedals: throttle drives forward; brake at a standstill becomes reverse. ---
    {
        const auto Fwd = Shape(0.0, 1.0, 0.0, 0.0, 0.0, 0.0);
        Check(!Fwd.bReverse && Fwd.GearboxThrottle > 0.0, "throttle -> forward drive");
        const auto Rev = Shape(0.0, 0.0, 1.0, 0.0, 0.0, 0.0); // brake, standstill (fwdSpeed 0)
        Check(Rev.bReverse && Rev.GearboxThrottle < 0.0, "brake at a standstill -> reverse (signed throttle)");
        const auto Braking = Shape(0.0, 0.0, 1.0, 2000.0, 2000.0, 0.0); // brake while rolling forward
        Check(!Braking.bReverse && Braking.Brake > 0.0, "brake while rolling forward -> service brake, not reverse");
    }

    FVehicleTransmission::FParams P; // defaults: idle 800, redline 7000, shift 6200/2200, ratios 12/7/4.5

    // --- Standstill + throttle rolls into Drive with drive torque. ---
    {
        FVehicleTransmission Tx;
        double Prev = 0.0;
        DriveTick(Tx, Prev, 1.0, 0.0, 0.0, 0.0, 0.0, P);
        Check(Tx.Gear() == FVehicleTransmission::EGear::Drive, "throttle from a standstill selects Drive");
        Check(Tx.ThrottleOut() > 0.0, "Drive with throttle delivers drive torque");
        Check(Tx.IsInDrivingGear(), "Drive is a driving gear");
    }

    // --- Standstill + brake rolls into Reverse. ---
    {
        FVehicleTransmission Tx;
        double Prev = 0.0;
        DriveTick(Tx, Prev, 0.0, 1.0, 0.0, 0.0, 0.0, P);
        Check(Tx.Gear() == FVehicleTransmission::EGear::Reverse, "brake from a standstill selects Reverse");
    }

    // --- Auto-shift up: holding throttle as the wheels spin faster steps the forward gear up. ---
    {
        FVehicleTransmission Tx;
        double Prev = 0.0;
        DriveTick(Tx, Prev, 1.0, 0.0, 0.0, 200.0, 100.0, P); // gentle: engine ~1200 rpm in gear 1
        const int32 LowGear = Tx.ForwardGearNumber();
        for (int i = 0; i < 4; ++i)
        {
            DriveTick(Tx, Prev, 1.0, 0.0, 0.0, 4000.0, 700.0, P); // engine 8400 in gear 1 -> shift up
        }
        Check(LowGear == 1, "starts in first gear");
        Check(Tx.ForwardGearNumber() > 1, "revving out first gear auto-shifts up");
    }

    // --- Engine braking: lifting off the throttle at speed bleeds speed via the brake actuator. ---
    {
        FVehicleTransmission Tx;
        double Prev = 0.0;
        DriveTick(Tx, Prev, 1.0, 0.0, 0.0, 3000.0, 300.0, P); // get into Drive, moving
        DriveTick(Tx, Prev, 0.0, 0.0, 0.0, 3000.0, 300.0, P); // lift off, still rolling
        Check(Tx.BrakeOut() > 0.0, "coasting off-throttle in gear applies engine braking");
    }

    // --- Wheel-RPM estimate (the shell's speed -> RPM math). ---
    {
        const double Expect = 50.0 / (6.283185307179586 * 35.0) * 60.0;
        CheckNear(FVehicleDriveResolver::WheelRpmFromForwardSpeed(50.0, 35.0), Expect, "wheel RPM from forward speed");
        Check(FVehicleDriveResolver::WheelRpmFromForwardSpeed(-1000.0, 35.0) < 0.0, "reversing gives negative wheel RPM");
        Check(FVehicleDriveResolver::WheelRpmFromForwardSpeed(1000.0, 0.0) != 0.0, "a zero radius is floored, not a divide-by-zero");
    }

    // --- Brake dead-band fix: aligning the standstill to the reverse-engage speed makes braking
    //     through the low-speed window engage Reverse cleanly instead of coasting on engine braking. ---
    {
        FVehicleTransmission::FParams AP = P;
        AP.StandstillWheelRpm =
            FVehicleDriveResolver::WheelRpmFromForwardSpeed(FVehicleDriveInput::ReverseEngageSpeed, 35.0);
        FVehicleTransmission Tx;
        double Prev = 0.0;
        // ~40 cm/s forward (below the 50 cm/s reverse-engage speed), full brake held.
        DriveTick(Tx, Prev, 0.0, 1.0, 0.0, 40.0, FVehicleDriveResolver::WheelRpmFromForwardSpeed(40.0, 35.0), AP);
        Check(Tx.Gear() == FVehicleTransmission::EGear::Reverse,
            "with the standstill aligned, braking through the low-speed window engages Reverse (no dead-band)");
    }

    return OracleSummary("vehicle_drive_oracle");
}
