// Copyright (c) 2026 GTC contributors

#include "CrowdSteering.h"

#include <cmath>

namespace
{
    // The core's normalize: returns zero below a 1e-9 length threshold, otherwise
    // the exact unit vector. Kept local (function-static-free) to preserve the
    // oracle's literal threshold rather than relying on FVector2D::GetSafeNormal.
    FVector2D CoreNormalize(const FVector2D& A)
    {
        const double L = A.Size();
        return L > 1e-9 ? FVector2D(A.X / L, A.Y / L) : FVector2D(0.0, 0.0);
    }
}

FVector2D FCrowdSteering::ClampLen(const FVector2D& A, double MaxLen)
{
    if (MaxLen <= 0.0)
    {
        return FVector2D(0.0, 0.0); // no force budget — never flip direction
    }
    const double L = A.Size();
    if (L > MaxLen && L > 1e-9)
    {
        return FVector2D(A.X * MaxLen / L, A.Y * MaxLen / L);
    }
    return A;
}

FVector2D FCrowdSteering::Separation(const FVector2D& SelfPos, const TArray<FVector2D>& Neighbors, double Radius)
{
    FVector2D Force(0.0, 0.0);
    int32 N = 0;
    for (const FVector2D& P : Neighbors)
    {
        const FVector2D D = SelfPos - P;
        const double Dist = D.Size();
        if (Dist > 1e-6 && Dist < Radius)
        {
            Force += CoreNormalize(D) * (1.0 / Dist);
            ++N;
        }
    }
    if (N > 0)
    {
        Force *= 1.0 / N;
    }
    return Force;
}

FVector2D FCrowdSteering::Cohesion(const FVector2D& SelfPos, const TArray<FVector2D>& Neighbors)
{
    if (Neighbors.Num() == 0)
    {
        return FVector2D(0.0, 0.0);
    }
    FVector2D Center(0.0, 0.0);
    for (const FVector2D& P : Neighbors)
    {
        Center += P;
    }
    Center *= 1.0 / static_cast<double>(Neighbors.Num());
    return Center - SelfPos;
}

FVector2D FCrowdSteering::Alignment(const TArray<FVector2D>& NeighborVels)
{
    if (NeighborVels.Num() == 0)
    {
        return FVector2D(0.0, 0.0);
    }
    FVector2D Avg(0.0, 0.0);
    for (const FVector2D& V : NeighborVels)
    {
        Avg += V;
    }
    return Avg * (1.0 / static_cast<double>(NeighborVels.Num()));
}

FVector2D FCrowdSteering::Combine(const FVector2D& Sep, const FVector2D& Ali, const FVector2D& Coh,
    double WSep, double WAli, double WCoh, double MaxForce)
{
    const FVector2D F = Sep * WSep + Ali * WAli + Coh * WCoh;
    return ClampLen(F, MaxForce);
}

FVector2D FCrowdSteering::Seek(const FVector2D& SelfPos, const FVector2D& SelfVel, const FVector2D& Target, double MaxSpeed)
{
    const FVector2D Desired = CoreNormalize(Target - SelfPos) * MaxSpeed;
    return Desired - SelfVel;
}

FVector2D FCrowdSteering::Arrive(const FVector2D& SelfPos, const FVector2D& SelfVel, const FVector2D& Target,
    double SlowRadius, double MaxSpeed)
{
    const FVector2D ToTarget = Target - SelfPos;
    const double Dist = ToTarget.Size();
    if (Dist < 1e-6)
    {
        return SelfVel * -1.0; // already there — kill momentum
    }
    double Speed = MaxSpeed;
    if (SlowRadius > 1e-6 && Dist < SlowRadius)
    {
        Speed = MaxSpeed * (Dist / SlowRadius);
    }
    const FVector2D Desired = CoreNormalize(ToTarget) * Speed;
    return Desired - SelfVel;
}

FVector2D FCrowdSteering::AvoidObstacles(const FVector2D& SelfPos, const TArray<FVector2D>& Positions,
    const TArray<double>& Radii, double Margin)
{
    FVector2D Force(0.0, 0.0);
    const int32 N = Positions.Num() < Radii.Num() ? Positions.Num() : Radii.Num();
    for (int32 i = 0; i < N; ++i)
    {
        const double Safe = Radii[i] + Margin;
        if (Safe <= 1e-6)
        {
            continue; // obstacle has no positive avoidance range — ignore it
        }
        const FVector2D D = SelfPos - Positions[i];
        const double Dist = D.Size();
        if (Dist <= 1e-6)
        {
            Force += FVector2D(1.0, 0.0); // dead centre — push a fixed way out
        }
        else if (Dist < Safe)
        {
            const double Penetration = (Safe - Dist) / Safe; // 0..1, deeper = stronger
            Force += CoreNormalize(D) * Penetration;
        }
    }
    return Force;
}
