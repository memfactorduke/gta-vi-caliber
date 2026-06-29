// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleAttitude.h"

#include "Math/UnrealMathUtility.h"

double FVehicleAttitude::BankAngle(double YawRateRad, double SpeedCmS, double Gain, double MaxRollRad)
{
    const double Lateral = FMath::Max(0.0, SpeedCmS) * YawRateRad; // v * omega
    const double Roll = FMath::Atan2(Lateral, GravityCmS2) * Gain;
    const double MaxRoll = FMath::Abs(MaxRollRad);
    return FMath::Clamp(Roll, -MaxRoll, MaxRoll);
}

double FVehicleAttitude::PitchFromVelocity(double ClimbRateCmS, double HorizSpeedCmS, double MaxPitchRad)
{
    const double Horiz = FMath::Max(0.0, HorizSpeedCmS);
    const double Pitch = FMath::Atan2(ClimbRateCmS, Horiz);
    const double MaxPitch = FMath::Abs(MaxPitchRad);
    return FMath::Clamp(Pitch, -MaxPitch, MaxPitch);
}
