// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleAxisState.h"

#include "Math/UnrealMathUtility.h"

double FVehicleAxisState::Integrate(double Current, double RaiseLower, double RatePerSec, double Dt,
    double Lo, double Hi)
{
    const double Step = FMath::Max(0.0, Dt);
    const double Lower = FMath::Min(Lo, Hi);
    const double Upper = FMath::Max(Lo, Hi);

    if (Step <= 0.0)
    {
        return FMath::Clamp(Current, Lower, Upper);
    }

    const double Dir = FMath::Clamp(RaiseLower, -1.0, 1.0);
    const double Next = Current + Dir * RatePerSec * Step;
    return FMath::Clamp(Next, Lower, Upper);
}
