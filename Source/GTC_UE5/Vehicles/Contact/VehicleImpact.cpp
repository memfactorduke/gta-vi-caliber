// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleImpact.h"

#include "Math/UnrealMathUtility.h"

double FVehicleImpact::DamageForSpeed(double ImpactSpeedCm, double SafeSpeedCm, double Scale)
{
    const double Safe = FMath::Max(0.0, SafeSpeedCm);
    const double Sc = FMath::Max(0.0, Scale);
    const double Over = ImpactSpeedCm - Safe;
    if (Over <= 0.0 || Sc <= 0.0)
    {
        return 0.0;
    }
    return Sc * Over * Over;
}

double FVehicleImpact::Restitution(double IntoSurfaceSpeedCm, double GrazeDot)
{
    if (IntoSurfaceSpeedCm <= 0.0)
    {
        return 0.0; // moving away / no penetration — nothing to retain
    }
    const double Dot = FMath::Clamp(GrazeDot, 0.0, 1.0);
    return FMath::Clamp(1.0 - Dot, 0.0, 1.0);
}
