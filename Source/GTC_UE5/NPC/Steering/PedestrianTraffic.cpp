// Copyright (c) 2026 GTC contributors

#include "PedestrianTraffic.h"

#include "NpcSteering.h"
#include "Math/UnrealMathUtility.h"

bool FPedestrianTraffic::IsClosing(
    const FVector& PPos, const FVector& PVel, const FVector& CPos, const FVector& CVel)
{
    const FVector R = FNpcSteering::Ground(PPos - CPos);
    const FVector V = FNpcSteering::Ground(PVel - CVel);
    return FVector::DotProduct(R, V) < 0.0;
}

double FPedestrianTraffic::TimeToClosestApproach(
    const FVector& PPos, const FVector& PVel, const FVector& CPos, const FVector& CVel)
{
    const FVector R = FNpcSteering::Ground(PPos - CPos);
    const FVector V = FNpcSteering::Ground(PVel - CVel);
    const double VV = FVector::DotProduct(V, V);
    if (VV < Epsilon)
    {
        return 0.0;
    }
    return FMath::Max(-FVector::DotProduct(R, V) / VV, 0.0);
}

double FPedestrianTraffic::ClosestApproachDistance(
    const FVector& PPos, const FVector& PVel, const FVector& CPos, const FVector& CVel)
{
    const double T = TimeToClosestApproach(PPos, PVel, CPos, CVel);
    const FVector PFuture = FNpcSteering::Ground(PPos) + FNpcSteering::Ground(PVel) * T;
    const FVector CFuture = FNpcSteering::Ground(CPos) + FNpcSteering::Ground(CVel) * T;
    return (PFuture - CFuture).Size();
}

double FPedestrianTraffic::CarThreat(
    const FVector& PPos,
    const FVector& PVel,
    const FVector& CPos,
    const FVector& CVel,
    double ReactRadius,
    double HorizonSec)
{
    if (ReactRadius <= 0.0 || HorizonSec <= 0.0)
    {
        return 0.0;
    }
    if (!IsClosing(PPos, PVel, CPos, CVel))
    {
        return 0.0;
    }
    const double T = TimeToClosestApproach(PPos, PVel, CPos, CVel);
    if (T > HorizonSec)
    {
        return 0.0;
    }
    const double Miss = ClosestApproachDistance(PPos, PVel, CPos, CVel);
    if (Miss >= ReactRadius)
    {
        return 0.0;
    }
    const double Proximity = 1.0 - Miss / ReactRadius;
    const double Urgency = 1.0 - T / HorizonSec;
    return FMath::Clamp(Proximity * Urgency, 0.0, 1.0);
}

FPedestrianTraffic::FThreat FPedestrianTraffic::NearestThreat(
    const FVector& PPos,
    const FVector& PVel,
    const TArray<FCar>& Cars,
    double ReactRadius,
    double HorizonSec)
{
    FThreat Best;
    for (int32 i = 0; i < Cars.Num(); ++i)
    {
        const FVector CPos = Cars[i].Pos;
        const FVector CVel = Cars[i].Vel;
        const double Threat = CarThreat(PPos, PVel, CPos, CVel, ReactRadius, HorizonSec);
        if (Threat > Best.Threat)
        {
            Best.Threat = Threat;
            Best.Index = i;
            Best.Pos = CPos;
            Best.Vel = CVel;
        }
    }
    return Best;
}

FVector FPedestrianTraffic::DodgeVelocity(
    const FVector& PPos, const FVector& CPos, const FVector& CVel, double MaxSpeed)
{
    const FVector Heading = FNpcSteering::Ground(CVel);
    const FVector Offset = FNpcSteering::Ground(PPos - CPos);
    if (Heading.Size() < Epsilon)
    {
        if (Offset.Size() < Epsilon)
        {
            return FVector::ZeroVector;
        }
        return Offset.GetSafeNormal() * MaxSpeed;
    }
    const FVector Perp = FVector(Heading.Z, 0.0, -Heading.X).GetSafeNormal();
    const double Dot = FVector::DotProduct(Offset, Perp);
    // the reference signf(0.0) == 0.0, then the explicit fallback flips it to 1.0.
    double Side = (Dot > 0.0) ? 1.0 : ((Dot < 0.0) ? -1.0 : 0.0);
    if (Side == 0.0)
    {
        Side = 1.0;
    }
    return Perp * Side * MaxSpeed;
}

bool FPedestrianTraffic::SafeToCross(
    const FVector& PPos, const TArray<FCar>& Cars, double DangerRadius, double SafeGapSec)
{
    for (const FCar& Car : Cars)
    {
        const FVector CPos = Car.Pos;
        const FVector CVel = Car.Vel;
        if (!IsClosing(PPos, FVector::ZeroVector, CPos, CVel))
        {
            continue;
        }
        const double T = TimeToClosestApproach(PPos, FVector::ZeroVector, CPos, CVel);
        if (T > SafeGapSec)
        {
            continue;
        }
        if (ClosestApproachDistance(PPos, FVector::ZeroVector, CPos, CVel) <= DangerRadius)
        {
            return false;
        }
    }
    return true;
}
