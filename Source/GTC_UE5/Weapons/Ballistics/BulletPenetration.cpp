// Copyright Epic Games, Inc. All Rights Reserved.

#include "BulletPenetration.h"

#include "Math/UnrealMathUtility.h"

double FBulletPenetration::SurfaceCost(double ThicknessCm, double Density)
{
    return FMath::Max(0.0, ThicknessCm) * FMath::Max(0.0, Density);
}

bool FBulletPenetration::CanPenetrate(double PenetrationPower, double ThicknessCm, double Density)
{
    return PenetrationPower > 0.0 && PenetrationPower >= SurfaceCost(ThicknessCm, Density);
}

double FBulletPenetration::RemainingPower(double PenetrationPower, double ThicknessCm, double Density)
{
    if (!CanPenetrate(PenetrationPower, ThicknessCm, Density))
    {
        return 0.0;
    }
    return FMath::Max(0.0, PenetrationPower - SurfaceCost(ThicknessCm, Density));
}

double FBulletPenetration::ExitDamageFactor(double PenetrationPower, double ThicknessCm, double Density)
{
    if (!CanPenetrate(PenetrationPower, ThicknessCm, Density) || PenetrationPower <= 0.0)
    {
        return 0.0;
    }
    const double Frac = RemainingPower(PenetrationPower, ThicknessCm, Density) / PenetrationPower;
    return FMath::Clamp(Frac, MinExitFactor, 1.0);
}
