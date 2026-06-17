// Copyright (c) 2026 GTC contributors

#include "Ballistics.h"

#include "Math/UnrealMathUtility.h"

FVector FBallistics::SpreadDirection(
    const FVector& Forward, const FVector& Right, const FVector& Up, const FVector2D& Sample, double Spread)
{
    if (Spread <= 0.0 || Sample.IsNearlyZero(Eps))
    {
        return Forward.GetSafeNormal();
    }
    const FVector Offset = (Right * Sample.X + Up * Sample.Y) * FMath::Tan(Spread);
    return (Forward + Offset).GetSafeNormal();
}

double FBallistics::DamageAtRange(
    double BaseDamage, double Distance, double FalloffStart, double FalloffEnd, double MinFraction)
{
    // Clamp the floor to [0, 1]: an out-of-range MinFraction > 1 would otherwise make
    // a far shot deal MORE than point-blank, and a negative one would heal the target.
    const double Mf = FMath::Clamp(MinFraction, 0.0, 1.0);
    if (Distance <= FalloffStart)
    {
        return BaseDamage;
    }
    if (Distance >= FalloffEnd || FalloffEnd <= FalloffStart)
    {
        return BaseDamage * Mf;
    }
    const double T = (Distance - FalloffStart) / (FalloffEnd - FalloffStart);
    return BaseDamage * FMath::Lerp(1.0, Mf, T);
}

double FBallistics::ZoneMultiplier(double LocalHeight, double HeadHeight, double HeadMult)
{
    return LocalHeight >= HeadHeight ? HeadMult : 1.0;
}

FVector2D FBallistics::DiskSample(double URadius, double UAngle)
{
    const double R = FMath::Sqrt(FMath::Clamp(URadius, 0.0, 1.0));
    const double A = UAngle * UE_DOUBLE_TWO_PI;
    return FVector2D(FMath::Cos(A), FMath::Sin(A)) * R;
}
