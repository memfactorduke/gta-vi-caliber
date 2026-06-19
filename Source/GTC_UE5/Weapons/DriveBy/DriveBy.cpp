// Copyright Epic Games, Inc. All Rights Reserved.

#include "DriveBy.h"

#include "Math/UnrealMathUtility.h"

double FDriveBy::AimAngleFromForward(const FVector& VehicleForward, const FVector& AimDir)
{
    const FVector Fwd = VehicleForward.GetSafeNormal();
    const FVector Aim = AimDir.GetSafeNormal();
    if (Fwd.IsNearlyZero() || Aim.IsNearlyZero())
    {
        return 0.0; // degenerate -> "ahead", which CanFire blocks
    }
    const double CosA = FMath::Clamp(FVector::DotProduct(Fwd, Aim), -1.0, 1.0);
    return FMath::Acos(CosA);
}

bool FDriveBy::CanFire(const FVector& VehicleForward, const FVector& AimDir, double ForwardBlockHalfAngle)
{
    return AimAngleFromForward(VehicleForward, AimDir) >= ForwardBlockHalfAngle;
}

double FDriveBy::FiringSide(const FVector& VehicleRight, const FVector& AimDir)
{
    const FVector Rgt = VehicleRight.GetSafeNormal();
    const FVector Aim = AimDir.GetSafeNormal();
    if (Rgt.IsNearlyZero() || Aim.IsNearlyZero())
    {
        return 0.0;
    }
    const double Dot = FVector::DotProduct(Rgt, Aim);
    constexpr double SideEps = 1e-4;
    if (Dot > SideEps)
    {
        return 1.0;
    }
    if (Dot < -SideEps)
    {
        return -1.0;
    }
    return 0.0;
}

double FDriveBy::SpeedSpread(double SpeedFraction)
{
    return FMath::Clamp(SpeedFraction, 0.0, 1.0) * MaxSpeedSpreadRad;
}

double FDriveBy::EffectiveSpread(double BaseSpreadRad, double SpeedFraction)
{
    return FMath::Max(0.0, BaseSpreadRad) + SpeedSpread(SpeedFraction);
}
