// Copyright Epic Games, Inc. All Rights Reserved.

#include "Carjacking.h"

FVector Carjacking::Ground(const FVector& V)
{
    return FVector(V.X, 0.0, V.Z);
}

bool Carjacking::CanReach(const FVector& PlayerPos, const FVector& CarPos, double ReachRadius)
{
    if (ReachRadius <= 0.0)
    {
        return false;
    }
    return Ground(PlayerPos - CarPos).Size() <= ReachRadius;
}

int32 Carjacking::DoorSide(const FVector& CarPos, const FVector& CarForward, const FVector& PlayerPos)
{
    const FVector Fwd = Ground(CarForward);
    if (Fwd.Size() < Eps)
    {
        return 0;
    }
    // Right-hand vector on the XZ plane (forward rotated -90 degrees about up).
    const FVector Right = FVector(Fwd.Z, 0.0, -Fwd.X).GetSafeNormal();
    const double Lateral = FVector::DotProduct(Ground(PlayerPos - CarPos), Right);
    if (FMath::Abs(Lateral) < Eps)
    {
        return 0;
    }
    return Lateral > 0.0 ? 1 : -1;
}

bool Carjacking::IsOccupiedCrime(bool bCarHasDriver)
{
    return bCarHasDriver;
}

double Carjacking::HeatForJack(bool bCarHasDriver, double BaseHeat)
{
    if (!IsOccupiedCrime(bCarHasDriver))
    {
        return 0.0;
    }
    return FMath::Max(BaseHeat, 0.0);
}

double Carjacking::ResistModifier(double DriverToughness)
{
    return 1.0 + FMath::Clamp(DriverToughness, 0.0, 1.0);
}

Carjacking::Carjacking(double Duration)
{
    JackDuration = FMath::Max(Duration, 0.0001);
}

void Carjacking::Begin()
{
    Elapsed = 0.0;
    bActive = true;
    bComplete = false;
}

void Carjacking::Tick(double Delta)
{
    if (!bActive || bComplete || Delta < 0.0)
    {
        return;
    }
    Elapsed += Delta;
    if (Elapsed >= JackDuration)
    {
        Elapsed = JackDuration;
        bActive = false;
        bComplete = true;
    }
}

double Carjacking::Progress() const
{
    return FMath::Clamp(Elapsed / JackDuration, 0.0, 1.0);
}

bool Carjacking::IsComplete() const
{
    return bComplete;
}

void Carjacking::Cancel()
{
    bActive = false;
    bComplete = false;
    Elapsed = 0.0;
}

void Carjacking::Reset()
{
    bActive = false;
    bComplete = false;
    Elapsed = 0.0;
}
