// Copyright Epic Games, Inc. All Rights Reserved.

#include "AirSeaEscalation.h"

#include "Math/UnrealMathUtility.h"

int32 FAirSeaEscalation::ClampStars(int32 Stars)
{
    return FMath::Clamp(Stars, 0, MaxStars);
}

bool FAirSeaEscalation::ShouldDeployPoliceAir(int32 Stars, EPlayerMedium Medium)
{
    const int32 S = ClampStars(Stars);
    // Scramble one star earlier to chase a flyer, but never at 0 stars.
    const int32 Threshold = (Medium == EPlayerMedium::Air) ? FMath::Max(1, PoliceAirStars - 1) : PoliceAirStars;
    return S >= Threshold;
}

bool FAirSeaEscalation::ShouldDeployCoastGuard(int32 Stars, EPlayerMedium Medium)
{
    if (Medium != EPlayerMedium::Sea)
    {
        return false;
    }
    return ClampStars(Stars) >= CoastGuardStars;
}

int32 FAirSeaEscalation::PoliceAirCount(int32 Stars, EPlayerMedium Medium)
{
    if (!ShouldDeployPoliceAir(Stars, Medium))
    {
        return 0;
    }
    // One chopper, a second once the heat is serious (5+).
    return ClampStars(Stars) >= 5 ? 2 : 1;
}

int32 FAirSeaEscalation::CoastGuardBoatCount(int32 Stars, EPlayerMedium Medium)
{
    if (!ShouldDeployCoastGuard(Stars, Medium))
    {
        return 0;
    }
    // 1 boat at the launch star, +1 per star above it, capped at 3.
    const int32 Count = ClampStars(Stars) - (CoastGuardStars - 1);
    return FMath::Clamp(Count, 1, 3);
}
