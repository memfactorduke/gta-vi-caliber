// Copyright Epic Games, Inc. All Rights Reserved.

#include "FirePropagation.h"

#include "Math/UnrealMathUtility.h"

double FFirePropagation::SpreadIntensity(double SourceIntensity, double DistanceM, double RadiusM)
{
    if (RadiusM <= 0.0)
    {
        return 0.0;
    }
    const double Dist = FMath::Max(0.0, DistanceM);
    if (Dist >= RadiusM)
    {
        return 0.0;
    }
    const double Src = FMath::Clamp(SourceIntensity, 0.0, 1.0);
    const double Falloff = 1.0 - (Dist / RadiusM);
    const double S = Src * Falloff;
    return (S < MinSpreadIntensity) ? 0.0 : FMath::Clamp(S, 0.0, 1.0);
}

bool FFirePropagation::CanIgnite(double SourceIntensity, double DistanceM, double Flammability, double RadiusM)
{
    const double Imparted = SpreadIntensity(SourceIntensity, DistanceM, RadiusM);
    return Imparted * FMath::Clamp(Flammability, 0.0, 1.0) >= IgnitionThreshold;
}

double FFirePropagation::DamagePerSecond(double Intensity)
{
    return FMath::Clamp(Intensity, 0.0, 1.0) * MaxDamagePerSec;
}

void FFirePropagation::FFireCell::Ignite(double StartIntensity, double FuelLoad)
{
    Intensity = FMath::Clamp(StartIntensity, 0.0, 1.0);
    Fuel = FMath::Max(0.0, FuelLoad);
}

void FFirePropagation::FFireCell::Tick(double Dt)
{
    if (Dt <= 0.0)
    {
        return;
    }

    if (Fuel > 0.0)
    {
        // Grow toward a full blaze and eat fuel (faster the hotter it burns).
        Intensity = FMath::Min(1.0, Intensity + GrowthRate * Dt);
        Fuel = FMath::Max(0.0, Fuel - BurnPerSec * Intensity * Dt);
    }
    else
    {
        // Out of fuel — guttering out.
        Intensity = FMath::Max(0.0, Intensity - DecayRate * Dt);
    }
}
