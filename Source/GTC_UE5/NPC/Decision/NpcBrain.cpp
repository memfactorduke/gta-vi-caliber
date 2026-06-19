// Copyright (c) 2026 GTC contributors

#include "NpcBrain.h"

#include "Math/UnrealMathUtility.h"

FVector FNpcBrain::WanderTarget(const FVector& Origin, double Radius, double URadius, double UAngle)
{
    const double R = FMath::Sqrt(FMath::Clamp(URadius, 0.0, 1.0)) * Radius;
    const double A = UAngle * UE_DOUBLE_TWO_PI;
    return Origin + FVector(FMath::Cos(A) * R, 0.0, FMath::Sin(A) * R);
}

double FNpcBrain::PlanarDistance(const FVector& A, const FVector& B)
{
    return FVector2D(A.X - B.X, A.Z - B.Z).Size();
}

bool FNpcBrain::Arrived(const FVector& Pos, const FVector& Target, double Tolerance)
{
    return PlanarDistance(Pos, Target) <= Tolerance;
}

FVector FNpcBrain::PlanarDir(const FVector& A, const FVector& B)
{
    const FVector D(B.X - A.X, 0.0, B.Z - A.Z);
    return D.Size() > 0.0001 ? D.GetSafeNormal() : FVector::ZeroVector;
}

FVector FNpcBrain::FleeDir(const FVector& SelfPos, const FVector& ThreatPos)
{
    return PlanarDir(ThreatPos, SelfPos);
}

FVector FNpcBrain::PursueDir(const FVector& SelfPos, const FVector& TargetPos)
{
    return PlanarDir(SelfPos, TargetPos);
}

FNpcBrain::EState FNpcBrain::NextState(
    EState Current,
    bool bThreatActive,
    double ThreatDistance,
    double FleeRadius,
    double CalmRadius)
{
    if (bThreatActive && ThreatDistance <= FleeRadius)
    {
        return EState::Flee;
    }
    if (Current == EState::Flee && bThreatActive && ThreatDistance < CalmRadius)
    {
        return EState::Flee;
    }
    if (Current == EState::Flee)
    {
        return EState::Wander;
    }
    return Current;
}

double FNpcBrain::SpeedFor(EState State, double WalkSpeed, double RunSpeed)
{
    switch (State)
    {
    case EState::Flee:
        return RunSpeed;
    case EState::Wander:
        return WalkSpeed;
    default:
        return 0.0;
    }
}
