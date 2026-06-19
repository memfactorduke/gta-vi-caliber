// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcWitness.h"

#include "Math/UnrealMathUtility.h"

double FNpcWitness::Share(double BaseIntensity, double DistanceM, double RadiusM)
{
    if (RadiusM <= 0.0)
    {
        return 0.0;
    }
    const double Dist = FMath::Max(0.0, DistanceM); // on top of it == full
    if (Dist >= RadiusM)
    {
        return 0.0; // out of sight, out of mind
    }

    const double Base = FMath::Clamp(BaseIntensity, 0.0, 1.0);
    const double Falloff = 1.0 - (Dist / RadiusM); // 1 at the elbow, 0 at the edge
    const double S = Base * Falloff;

    return (S < MinShare) ? 0.0 : FMath::Clamp(S, 0.0, 1.0);
}
