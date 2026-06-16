// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrimeWitness.h"

bool FCrimeWitness::CanWitness(
    const FVector& ObserverPos,
    const FVector& ObserverFacing,
    const FVector& CrimePos,
    double SightRange,
    double FovRadians)
{
    if (SightRange <= 0.0 || FovRadians <= 0.0)
    {
        return false;
    }
    const FVector Facing = Ground(ObserverFacing);
    if (Facing.Size() < 0.0001)
    {
        return false;
    }
    const FVector ToCrime = Ground(CrimePos - ObserverPos);
    const double Dist = ToCrime.Size();
    if (Dist > SightRange)
    {
        return false;
    }
    if (Dist < 0.0001)
    {
        // Crime is right on the observer — no meaningful bearing, count it seen.
        return true;
    }
    // Compare the bearing to facing against the cone's half-angle. FovRadians is the
    // half-angle, so the dot must clear cos(half-angle).
    const double CosAngle = Facing.GetSafeNormal().Dot(ToCrime.GetSafeNormal());
    return CosAngle >= FMath::Cos(FMath::Clamp(FovRadians, 0.0, UE_DOUBLE_PI));
}

int32 FCrimeWitness::CountWitnesses(
    const FVector& CrimePos,
    const TArray<FCrimeObserver>& Observers,
    double SightRange,
    double FovRadians)
{
    int32 Count = 0;
    for (const FCrimeObserver& Entry : Observers)
    {
        if (CanWitness(Entry.Pos, Entry.Facing, CrimePos, SightRange, FovRadians))
        {
            ++Count;
        }
    }
    return Count;
}

FCollectedWitnesses FCrimeWitness::CollectWitnesses(
    const FVector& CrimePos,
    const TArray<FCrimeObserver>& Observers,
    double PedRange,
    double PedFov,
    double PoliceRange,
    double PoliceFov)
{
    FCollectedWitnesses Result;
    for (const FCrimeObserver& Observer : Observers)
    {
        const double Sight = Observer.bIsPolice ? PoliceRange : PedRange;
        const double Fov = Observer.bIsPolice ? PoliceFov : PedFov;
        if (CanWitness(Observer.Pos, Observer.Facing, CrimePos, Sight, Fov))
        {
            Result.Witnesses.Add(Observer);
            Result.bPoliceSaw = Result.bPoliceSaw || Observer.bIsPolice;
        }
    }
    return Result;
}

double FCrimeWitness::HeatForCrime(double BaseHeat, int32 WitnessCount)
{
    if (WitnessCount <= 0 || BaseHeat <= 0.0)
    {
        return 0.0;
    }
    // Saturating curve: 1 - 0.5^n. One witness -> 0.5, two -> 0.75 ... asymptotes to 1.0.
    const double Fraction = 1.0 - FMath::Pow(0.5, static_cast<double>(WitnessCount));
    return BaseHeat * Fraction;
}

void FCrimeWitness::Tick(double Delta)
{
    if (_bSilenced || IsReported())
    {
        return;
    }
    _Elapsed = FMath::Min(_Elapsed + FMath::Max(Delta, 0.0), _ReportDelay);
}

bool FCrimeWitness::IsReported() const
{
    if (_bSilenced)
    {
        return false;
    }
    return _Elapsed >= _ReportDelay;
}

double FCrimeWitness::Progress() const
{
    if (_bSilenced)
    {
        return 0.0;
    }
    if (_ReportDelay <= 0.0)
    {
        return 1.0;
    }
    return FMath::Clamp(_Elapsed / _ReportDelay, 0.0, 1.0);
}
