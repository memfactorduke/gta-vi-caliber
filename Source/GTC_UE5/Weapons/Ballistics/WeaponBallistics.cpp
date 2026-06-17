// Copyright (c) 2026 GTC contributors

#include "WeaponBallistics.h"

#include "Math/UnrealMathUtility.h"

#include <limits>

double FWeaponBallistics::DamageAtRange(
    double BaseDamage, double Distance, double FalloffStart, double FalloffEnd, double MinFactor)
{
    const double Base = FMath::Max(BaseDamage, 0.0);
    const double FloorFactor = FMath::Clamp(MinFactor, 0.0, 1.0);
    const double D = FMath::Max(Distance, 0.0);
    if (D <= FalloffStart)
    {
        return Base;
    }
    if (D >= FalloffEnd || FalloffEnd <= FalloffStart)
    {
        return Base * FloorFactor;
    }
    const double T = (D - FalloffStart) / (FalloffEnd - FalloffStart);
    return Base * FMath::Lerp(1.0, FloorFactor, FMath::Clamp(T, 0.0, 1.0));
}

double FWeaponBallistics::HitMultiplier(const FString& BodyPart)
{
    const FString Part = BodyPart.ToLower();
    if (Part == TEXT("head"))
    {
        return HeadMultiplier;
    }
    if (Part == TEXT("torso") || Part == TEXT("chest") || Part == TEXT("body"))
    {
        return TorsoMultiplier;
    }
    if (Part == TEXT("limb") || Part == TEXT("arm") || Part == TEXT("leg"))
    {
        return LimbMultiplier;
    }
    return 1.0;
}

FVector FWeaponBallistics::SpreadDirection(const FVector& AimDir, double SpreadRadians, FRandomStream& Rng)
{
    if (AimDir.Length() < 0.0001)
    {
        return AimDir;
    }
    const FVector Aim = AimDir.GetSafeNormal();
    if (SpreadRadians <= 0.0)
    {
        return Aim;
    }
    // An orthonormal basis spanning the plane perpendicular to Aim (the reference frame).
    const FVector Helper = FMath::Abs(FVector::DotProduct(Aim, FVector::UpVector)) < 0.99
        ? FVector::UpVector
        : FVector::RightVector;
    const FVector RightAxis = FVector::CrossProduct(Aim, Helper).GetSafeNormal();
    const FVector UpAxis = FVector::CrossProduct(RightAxis, Aim).GetSafeNormal();
    // Uniform point in the unit disk (sqrt keeps it area-correct, no centre clump).
    const double Radius = FMath::Sqrt(static_cast<double>(Rng.GetFraction()));
    const double Angle = static_cast<double>(Rng.GetFraction()) * UE_DOUBLE_TWO_PI;
    const FVector Offset =
        (RightAxis * FMath::Cos(Angle) + UpAxis * FMath::Sin(Angle)) * Radius * FMath::Tan(SpreadRadians);
    return (Aim + Offset).GetSafeNormal();
}

double FWeaponBallistics::EffectiveDamage(
    double BaseDamage,
    double Distance,
    const FString& BodyPart,
    double FalloffStart,
    double FalloffEnd,
    double MinFactor)
{
    const double Ranged = DamageAtRange(BaseDamage, Distance, FalloffStart, FalloffEnd, MinFactor);
    return Ranged * HitMultiplier(BodyPart);
}

double FWeaponBallistics::TimeToKill(double EffectiveDamagePerShot, double FireRate, double TargetHealth)
{
    if (EffectiveDamagePerShot <= 0.0 || FireRate <= 0.0)
    {
        return std::numeric_limits<double>::infinity();
    }
    if (TargetHealth <= 0.0)
    {
        return 0.0;
    }
    const int32 Shots = static_cast<int32>(FMath::CeilToDouble(TargetHealth / EffectiveDamagePerShot));
    return static_cast<double>(Shots - 1) / FireRate;
}

FWeaponBallistics::FBloom::FBloom(double InMin, double InMax, double InPerShot, double InRecovery)
{
    MinSpread = FMath::Max(InMin, 0.0);
    MaxSpread = FMath::Max(InMax, MinSpread);
    PerShot = FMath::Max(InPerShot, 0.0);
    Recovery = FMath::Max(InRecovery, 0.0);
    Spread = MinSpread;
}

double FWeaponBallistics::FBloom::CurrentSpread() const
{
    return Spread;
}

void FWeaponBallistics::FBloom::AddShot()
{
    Spread = FMath::Min(Spread + PerShot, MaxSpread);
}

void FWeaponBallistics::FBloom::Recover(double Delta)
{
    if (Delta <= 0.0)
    {
        return;
    }
    Spread = FMath::Max(Spread - Recovery * Delta, MinSpread);
}

void FWeaponBallistics::FBloom::Reset()
{
    Spread = MinSpread;
}
