// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcPanic.h"

#include "Math/UnrealMathUtility.h"

double FNpcPanic::CaughtFear(double SourceFear, double DistanceM, double Bravery, double RadiusM)
{
    if (RadiusM <= 0.0)
    {
        return 0.0;
    }

    const double Dist = FMath::Max(0.0, DistanceM); // standing on them == closest
    if (Dist >= RadiusM)
    {
        return 0.0; // too far for terror to jump
    }

    const double Source = FMath::Clamp(SourceFear, 0.0, 1.0);
    const double Falloff = 1.0 - (Dist / RadiusM); // 1 at the shoulder, 0 at the edge

    // Nerve resistance: the timid catch all of it, the steady (>= SteadyNerve) none.
    const double NerveFactor = FMath::Clamp((SteadyNerve - Bravery) / SteadyNerve, 0.0, 1.0);

    const double S = Source * Falloff * NerveFactor;
    return (S < MinShare) ? 0.0 : FMath::Clamp(S, 0.0, 1.0);
}

bool FNpcPanic::WouldSpread(double SourceFear, double DistanceM, double Bravery, double RadiusM)
{
    return CaughtFear(SourceFear, DistanceM, Bravery, RadiusM) > 0.0;
}

double FNpcPanic::Reinforce(double CurrentFear, double CaughtFear)
{
    const double Current = FMath::Clamp(CurrentFear, 0.0, 1.0);
    const double Caught = FMath::Clamp(CaughtFear, 0.0, 1.0);

    // Probabilistic OR: adds the headroom the catch can still fill, so repeated nudges
    // approach 1 without ever exceeding it.
    return FMath::Clamp(Current + Caught * (1.0 - Current), 0.0, 1.0);
}
