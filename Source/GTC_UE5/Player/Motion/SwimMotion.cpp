// Copyright (c) 2026 GTC contributors

#include "SwimMotion.h"

#include "Math/UnrealMathUtility.h"

double FSwimMotion::Submersion(double OriginY, double WaterY, double BodyHeight)
{
    if (BodyHeight <= 0.0)
    {
        return 0.0;
    }
    return FMath::Clamp((WaterY - OriginY) / BodyHeight, 0.0, 1.0);
}

bool FSwimMotion::IsSwimming(double SubmersionFraction, bool bCurrently, double EnterFraction, double ExitFraction)
{
    if (bCurrently)
    {
        return SubmersionFraction > ExitFraction;
    }
    return SubmersionFraction >= EnterFraction;
}

double FSwimMotion::VerticalAxis(bool bSurfacePressed, bool bDivePressed)
{
    return (bSurfacePressed ? 1.0 : 0.0) - (bDivePressed ? 1.0 : 0.0);
}

FVector FSwimMotion::TargetVelocity(const FVector& Direction, double SwimSpeed, double Axis, double VerticalSpeed)
{
    return FVector(Direction.X * SwimSpeed, Axis * VerticalSpeed, Direction.Z * SwimSpeed);
}

double FSwimMotion::Buoyancy(double SubmersionFraction, double Neutral, double Strength, double MaxSpeed)
{
    return FMath::Clamp((SubmersionFraction - Neutral) * Strength, -MaxSpeed, MaxSpeed);
}

bool FSwimMotion::HeadUnderwater(double SubmersionFraction, double HeadFraction)
{
    return SubmersionFraction >= HeadFraction;
}

double FSwimMotion::NextOxygen(double Oxygen, bool bUnderwater, double BreathSeconds, double RecoverRate, double Delta)
{
    if (bUnderwater)
    {
        return FMath::Clamp(Oxygen - Delta / FMath::Max(BreathSeconds, 0.0001), 0.0, 1.0);
    }
    return FMath::Clamp(Oxygen + RecoverRate * Delta, 0.0, 1.0);
}
