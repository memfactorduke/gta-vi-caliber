// Copyright (c) 2026 GTC contributors

#include "NpcSteering.h"

#include "Math/UnrealMathUtility.h"

FVector FNpcSteering::Ground(const FVector& V)
{
    return FVector(V.X, 0.0, V.Z);
}

FVector FNpcSteering::Seek(const FVector& Pos, const FVector& Target, double MaxSpeed)
{
    const FVector To = Ground(Target - Pos);
    if (To.Size() < Eps)
    {
        return FVector::ZeroVector;
    }
    return To.GetSafeNormal() * MaxSpeed;
}

FVector FNpcSteering::Arrive(
    const FVector& Pos,
    const FVector& Target,
    double MaxSpeed,
    double SlowRadius,
    double ArriveRadius)
{
    const FVector To = Ground(Target - Pos);
    const double Dist = To.Size();
    if (Dist <= ArriveRadius)
    {
        return FVector::ZeroVector;
    }
    double Speed = MaxSpeed;
    if (Dist < SlowRadius && SlowRadius > 0.0)
    {
        Speed = MaxSpeed * (Dist / SlowRadius);
    }
    return To.GetSafeNormal() * Speed;
}

FVector FNpcSteering::Separation(
    const FVector& Pos, const TArray<FVector>& Neighbors, double Radius, double MaxSpeed)
{
    if (Radius <= 0.0)
    {
        return FVector::ZeroVector;
    }
    FVector Push = FVector::ZeroVector;
    for (const FVector& Other : Neighbors)
    {
        const FVector Away = Ground(Pos - Other);
        const double D = Away.Size();
        if (D > Eps && D < Radius)
        {
            // Inverse falloff: a crowder at the elbow shoves far harder than one
            // at arm's length.
            Push += Away.GetSafeNormal() * (1.0 - D / Radius);
        }
    }
    if (Push.Size() < Eps)
    {
        return FVector::ZeroVector;
    }
    // Preserve the inverse-falloff magnitude accumulated above; only clamp when a
    // dense crowd's combined push would exceed MaxSpeed (the reference limit_length).
    return Push.GetClampedToMaxSize(MaxSpeed);
}

FVector FNpcSteering::Combine(
    const TArray<FVector>& Vectors, const TArray<double>& Weights, double MaxSpeed)
{
    FVector Sum = FVector::ZeroVector;
    const int32 Count = FMath::Min(Vectors.Num(), Weights.Num());
    for (int32 i = 0; i < Count; ++i)
    {
        Sum += Vectors[i] * Weights[i];
    }
    if (Sum.Size() > MaxSpeed)
    {
        Sum = Sum.GetSafeNormal() * MaxSpeed;
    }
    return Sum;
}

int32 FNpcSteering::AdvanceWaypoint(
    const FVector& Pos, const TArray<FVector>& Waypoints, int32 Index, double AcceptRadius)
{
    int32 i = Index;
    while (i < Waypoints.Num() - 1)
    {
        const FVector Wp = Waypoints[i];
        if (Ground(Wp - Pos).Size() <= AcceptRadius)
        {
            i += 1;
        }
        else
        {
            break;
        }
    }
    return FMath::Clamp(i, 0, FMath::Max(Waypoints.Num() - 1, 0));
}
