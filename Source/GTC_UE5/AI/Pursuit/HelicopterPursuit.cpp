// Copyright (c) 2026 GTC contributors

#include "HelicopterPursuit.h"

#include "Math/UnrealMathUtility.h"

#include "../../World/Police/PoliceResponse.h"

bool FHelicopterPursuit::ShouldDeploy(int32 Stars)
{
    return FPoliceResponse::UsesHelicopter(Stars);
}

FVector FHelicopterPursuit::OrbitPoint(
    const FVector& Center, double Time, double Radius, double Altitude, double AngularSpeed)
{
    const double A = Time * AngularSpeed;
    return Center + FVector(FMath::Cos(A) * Radius, Altitude, FMath::Sin(A) * Radius);
}

double FHelicopterPursuit::ConeHalfRadians(double Degrees)
{
    return FMath::DegreesToRadians(FMath::Clamp(Degrees, 0.0, 89.0));
}

double FHelicopterPursuit::SpotlightGroundRadius(double Altitude, double ConeHalf)
{
    // Clamp to the SAME ceiling ConeHalfRadians() produces (89deg).
    return FMath::Max(Altitude, 0.0) *
           FMath::Tan(FMath::Clamp(ConeHalf, 0.0, FMath::DegreesToRadians(89.0)));
}

bool FHelicopterPursuit::TargetLit(
    const FVector& FocusGround, const FVector& TargetGround, double LitRadius)
{
    const FVector2D D(TargetGround.X - FocusGround.X, TargetGround.Z - FocusGround.Z);
    return D.Size() <= FMath::Max(LitRadius, 0.0);
}
