// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleDriveResolver.h"

#include "VehicleDriveInput.h" // FVehicleDriveInput — same folder (shaping math)
#include "Math/UnrealMathUtility.h"

double FVehicleDriveResolver::WheelRpmFromForwardSpeed(double ForwardSpeedCmS, double WheelRadiusCm)
{
    // circumference (cm) = 2*pi*r; rev/min = (speed_cm_per_s / circumference_cm) * 60.
    const double TwoPi = 6.283185307179586;
    const double Circumference = TwoPi * FMath::Max(1.0, WheelRadiusCm);
    return (ForwardSpeedCmS / Circumference) * 60.0;
}

FVehicleDriveResolver::FOutput FVehicleDriveResolver::Shape(const FInput& In)
{
    FOutput Out;

    // Steering: deadzone + speed-sensitive authority give a target, then smooth toward it so a tap
    // doesn't snap the wheels (frame-rate-independent).
    const double SteerTarget = FVehicleDriveInput::SteerTarget(In.RawSteer, In.SpeedCmS);
    Out.Steer = FVehicleDriveInput::SteerInterp(In.PrevSteer, SteerTarget, In.Dt);

    // Pedals: the two triggers become throttle/brake, with brake flipping to reverse drive at a crawl.
    const FVehicleDriveInput::FDriveOutput Pedals =
        FVehicleDriveInput::ResolvePedals(In.RawThrottle, In.RawBrake, In.ForwardSpeedCmS);
    Out.Throttle = Pedals.Throttle;
    Out.Brake = Pedals.Brake;
    Out.bReverse = Pedals.bReverse;

    // The transmission reads the throttle's SIGN to pick Drive vs Reverse from a standstill.
    Out.GearboxThrottle = Pedals.bReverse ? -Pedals.Throttle : Pedals.Throttle;

    return Out;
}
