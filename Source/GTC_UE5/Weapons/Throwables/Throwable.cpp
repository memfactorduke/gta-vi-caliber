// Copyright Epic Games, Inc. All Rights Reserved.

#include "Throwable.h"

#include "Math/UnrealMathUtility.h"

double FThrowable::FuseAfterCook(double FuseSeconds, double CookSeconds)
{
    const double Fuse = FMath::Max(0.0, FuseSeconds);
    return FMath::Max(0.0, Fuse - FMath::Max(0.0, CookSeconds));
}

bool FThrowable::CookedOff(double FuseSeconds, double CookSeconds)
{
    return FuseAfterCook(FuseSeconds, CookSeconds) <= 0.0;
}

FVector FThrowable::LaunchVelocity(const FVector& AimDir, double ThrowSpeed)
{
    const FVector Dir = AimDir.GetSafeNormal();
    if (Dir.IsNearlyZero())
    {
        return FVector::ZeroVector;
    }
    return Dir * ThrowSpeed;
}

FVector FThrowable::PositionAtTime(const FVector& Origin, const FVector& LaunchVel, double Gravity, double T)
{
    if (T <= 0.0)
    {
        return Origin;
    }
    FVector Pos = Origin + LaunchVel * T;
    Pos.Z -= 0.5 * Gravity * T * T; // gravity pulls down on Z
    return Pos;
}

double FThrowable::TimeToGround(const FVector& Origin, const FVector& LaunchVel, double Gravity, double GroundZ)
{
    const double Vz = LaunchVel.Z;
    const double Dz = Origin.Z - GroundZ; // height above the target plane

    if (Gravity <= 0.0)
    {
        // No arc: only a downward throw ever reaches a lower plane.
        if (Vz < 0.0 && Dz >= 0.0)
        {
            return Dz / -Vz;
        }
        return (Vz < 0.0) ? (Dz / -Vz) : -1.0;
    }

    // Solve 0.5*g*t^2 - Vz*t - Dz = 0 for the later (descending) root.
    const double A = 0.5 * Gravity;
    const double B = -Vz;
    const double C = -Dz;
    const double Disc = B * B - 4.0 * A * C;
    if (Disc < 0.0)
    {
        return -1.0; // never reaches the plane (apex below it)
    }
    const double Root = (-B + FMath::Sqrt(Disc)) / (2.0 * A); // larger root = descending crossing
    return (Root >= 0.0) ? Root : -1.0;
}

FVector FThrowable::PredictLanding(const FVector& Origin, const FVector& LaunchVel, double Gravity, double GroundZ)
{
    const double T = TimeToGround(Origin, LaunchVel, Gravity, GroundZ);
    if (T < 0.0)
    {
        return Origin; // never comes down to the ground plane
    }
    FVector Landing = Origin + LaunchVel * T;
    Landing.Z = GroundZ; // pin exactly to the plane (no float drift)
    return Landing;
}

FVector FThrowable::DetonationPoint(const FVector& Origin, const FVector& LaunchVel, double Gravity,
    double GroundZ, double FuseRemaining)
{
    const double Fuse = FMath::Max(0.0, FuseRemaining);
    const double TGround = TimeToGround(Origin, LaunchVel, Gravity, GroundZ);

    // If it lands before the fuse runs out, it rests on the ground and detonates there.
    if (TGround >= 0.0 && TGround <= Fuse)
    {
        return PredictLanding(Origin, LaunchVel, Gravity, GroundZ);
    }
    // Otherwise it air-bursts wherever it is when the fuse expires.
    return PositionAtTime(Origin, LaunchVel, Gravity, Fuse);
}
