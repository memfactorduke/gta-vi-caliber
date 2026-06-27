// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleDriveInput.h"

#include "Math/UnrealMathUtility.h"

double FVehicleDriveInput::ApplyDeadzone(double Raw, double Deadzone)
{
    const double Clamped = FMath::Clamp(Raw, -1.0, 1.0);
    const double Dz = FMath::Clamp(Deadzone, 0.0, 0.999);
    const double Mag = FMath::Abs(Clamped);
    if (Mag <= Dz)
    {
        return 0.0;
    }
    // Rescale the live range [Dz, 1] back to [0, 1] so response stays full-throw.
    const double Scaled = (Mag - Dz) / (1.0 - Dz);
    return Clamped >= 0.0 ? Scaled : -Scaled;
}

double FVehicleDriveInput::SteerAuthority(double SpeedCmS)
{
    const double Speed = FMath::Max(0.0, SpeedCmS);
    if (Speed <= SteerFullAuthoritySpeed)
    {
        return 1.0;
    }
    if (Speed >= SteerMinAuthoritySpeed)
    {
        return MinSteerAuthority;
    }
    const double T = (Speed - SteerFullAuthoritySpeed) / (SteerMinAuthoritySpeed - SteerFullAuthoritySpeed);
    return FMath::Lerp(1.0, MinSteerAuthority, T);
}

double FVehicleDriveInput::SteerTarget(double RawSteer, double SpeedCmS)
{
    const double Deadzoned = ApplyDeadzone(RawSteer, SteerDeadzone);
    return FMath::Clamp(Deadzoned * SteerAuthority(SpeedCmS), -1.0, 1.0);
}

double FVehicleDriveInput::SteerInterp(double Current, double Target, double Dt)
{
    if (Dt <= 0.0)
    {
        return Current;
    }
    // Engine FInterpTo semantics: move a clamped fraction of the remaining distance.
    const double Alpha = FMath::Clamp(Dt * SteerInterpRate, 0.0, 1.0);
    return Current + (Target - Current) * Alpha;
}

FVehicleDriveInput::FDriveOutput FVehicleDriveInput::ResolvePedals(
    double RawAccel, double RawBrake, double ForwardSpeedCmS)
{
    const double Accel = FMath::Clamp(RawAccel, 0.0, 1.0);
    const double BrakeReq = FMath::Clamp(RawBrake, 0.0, 1.0);

    FDriveOutput Out;

    // Reverse when the brake trigger is held, the gas is released, and the car is
    // not still rolling forward faster than a crawl — otherwise the brake brakes.
    const bool bWantReverse = BrakeReq > 0.0 && Accel <= 0.0 && ForwardSpeedCmS <= ReverseEngageSpeed;

    if (bWantReverse)
    {
        Out.bReverse = true;
        Out.Throttle = BrakeReq;  // brake trigger now drives backward
        Out.Brake = 0.0;
    }
    else
    {
        Out.bReverse = false;
        Out.Throttle = Accel;
        Out.Brake = BrakeReq;
    }
    return Out;
}
