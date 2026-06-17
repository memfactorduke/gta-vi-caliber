// Copyright (c) 2026 GTC contributors

#include "ExplosionModel.h"

#include "Math/UnrealMathUtility.h"

namespace
{
    // the reference `Vector3.UP` is (0, 1, 0). The model stays in the the reference frame (no
    // Z-up remap), so the upward bias must be along +Y here — NOT UE's
    // FVector::UpVector (0, 0, 1). The oracle asserts k.y > 0 and k.z == 0.
    const FVector GodotUp(0.0, 1.0, 0.0);
}

double FExplosionModel::Falloff(double Distance, double Radius)
{
    if (Radius <= 0.0)
    {
        return 0.0;
    }
    if (Distance <= 0.0)
    {
        return 1.0;
    }
    if (Distance >= Radius)
    {
        return 0.0;
    }
    return 1.0 - Distance / Radius;
}

bool FExplosionModel::IsInBlast(const FVector& Center, const FVector& TargetPos, double Radius)
{
    if (Radius <= 0.0)
    {
        return false;
    }
    return FVector::Distance(Center, TargetPos) < Radius;
}

double FExplosionModel::DamageAt(const FVector& Center, const FVector& TargetPos, double MaxDamage, double Radius)
{
    const double Dist = FVector::Distance(Center, TargetPos);
    return FMath::Max(MaxDamage, 0.0) * Falloff(Dist, Radius);
}

FVector FExplosionModel::Knockback(const FVector& Center, const FVector& TargetPos, double MaxImpulse, double Radius)
{
    const FVector To = TargetPos - Center;
    const double Dist = To.Length();
    // At the dead centre there is no outward direction; spare the NaN and let the
    // vertical bias still throw the body straight up.
    if (Dist < 0.0001)
    {
        const double FCenter = Falloff(0.0, Radius);
        return GodotUp * (FMath::Max(MaxImpulse, 0.0) * FCenter * UpwardBias);
    }
    const double F = Falloff(Dist, Radius);
    if (F <= 0.0)
    {
        return FVector::ZeroVector;
    }
    const double Strength = FMath::Max(MaxImpulse, 0.0) * F;
    const FVector Outward = To / Dist;
    return Outward * Strength + GodotUp * (Strength * UpwardBias);
}

bool FExplosionModel::ShouldChain(const FVector& Center, const FVector& OtherExplosivePos, double TriggerRadius)
{
    if (TriggerRadius <= 0.0)
    {
        return false;
    }
    return FVector::Distance(Center, OtherExplosivePos) <= TriggerRadius;
}

TArray<FExplosionHit> FExplosionModel::ApplyToMany(
    const FVector& Center, const TArray<FVector>& Targets, double MaxDamage, double Radius)
{
    TArray<FExplosionHit> Hits;
    for (int32 i = 0; i < Targets.Num(); ++i)
    {
        const FVector& Pos = Targets[i];
        if (!IsInBlast(Center, Pos, Radius))
        {
            continue;
        }
        FExplosionHit Hit;
        Hit.Index = i;
        Hit.Damage = DamageAt(Center, Pos, MaxDamage, Radius);
        Hits.Add(Hit);
    }
    return Hits;
}
