// Copyright Epic Games, Inc. All Rights Reserved.

#include "Airspace.h"

#include "Math/UnrealMathUtility.h"

double FAircraftAirspace::CeilingAuthority(double CraftZCm, const FParams& Params)
{
    const double Ceiling = Params.CeilingZCm;
    const double Soft = FMath::Min(Params.SoftCeilingZCm, Ceiling);

    // Hard ceiling first: at/above it there is no climb authority, even when the band
    // has collapsed (soft == hard) — then this is a clean step at the ceiling.
    if (CraftZCm >= Ceiling)
    {
        return 0.0;
    }
    if (CraftZCm <= Soft)
    {
        return 1.0;
    }
    // Soft < CraftZ < Ceiling here, so Ceiling > Soft is guaranteed (no divide-by-zero).
    return FMath::Clamp((Ceiling - CraftZCm) / (Ceiling - Soft), 0.0, 1.0);
}

double FAircraftAirspace::ClampClimb(double CraftZCm, double ClimbRateCmS, const FParams& Params)
{
    if (ClimbRateCmS <= 0.0)
    {
        return ClimbRateCmS; // descent (and level) is never restricted
    }
    return ClimbRateCmS * CeilingAuthority(CraftZCm, Params);
}

double FAircraftAirspace::RestrictedFloorZ(double GroundZCm, double MinAltAboveGroundCm)
{
    return GroundZCm + FMath::Max(0.0, MinAltAboveGroundCm);
}
