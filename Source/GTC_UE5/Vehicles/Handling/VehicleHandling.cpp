// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleHandling.h"

#include "Math/UnrealMathUtility.h"

double FVehicleHandling::SlipAngleRad(const FVector& Forward, const FVector& Velocity)
{
    const FVector Fwd = Forward.GetSafeNormal();
    const FVector Vel = Velocity.GetSafeNormal();
    if (Fwd.IsNearlyZero() || Vel.IsNearlyZero())
    {
        return 0.0; // parked or no heading — nothing is slipping
    }
    const double CosA = FMath::Clamp(FVector::DotProduct(Fwd, Vel), -1.0, 1.0);
    return FMath::Acos(CosA);
}

double FVehicleHandling::LateralRetention(bool bHandbrake, double GripFactor)
{
    const double Base = bHandbrake ? HandbrakeRetention : GripRetention;
    const double Grip = FMath::Clamp(GripFactor, 0.0, 1.0);
    // Degraded grip (low GripFactor) lets the tail step out more: lerp the retention
    // partway toward a full slide as grip falls to 0.
    const double Worn = Base + (1.0 - Grip) * (1.0 - Base) * 0.5;
    return FMath::Clamp(Worn, 0.0, 1.0);
}

double FVehicleHandling::DriftFactor(double SlipAngleRadians, double Speed)
{
    if (Speed < MinDriftSpeed || SlipAngleRadians <= SlipDeadzoneRad)
    {
        return 0.0;
    }
    const double Span = FullDriftSlipRad - SlipDeadzoneRad;
    if (Span <= 0.0)
    {
        return 1.0; // degenerate band — treat any slip past the deadzone as full drift
    }
    return FMath::Clamp((SlipAngleRadians - SlipDeadzoneRad) / Span, 0.0, 1.0);
}

FVector FVehicleHandling::ApplyGrip(const FVector& Velocity, const FVector& Forward, const FVector& Right,
    double LateralRetentionFraction)
{
    const FVector Fwd = Forward.GetSafeNormal();
    const FVector Rgt = Right.GetSafeNormal();
    if (Fwd.IsNearlyZero() || Rgt.IsNearlyZero())
    {
        return Velocity; // no valid basis to split against
    }

    const double FwdSpeed = FVector::DotProduct(Velocity, Fwd);
    const double LatSpeed = FVector::DotProduct(Velocity, Rgt);

    // Whatever isn't along the ground basis (e.g. vertical) is left untouched.
    const FVector Planar = Fwd * FwdSpeed + Rgt * LatSpeed;
    const FVector Residual = Velocity - Planar;

    const double Retained = FMath::Clamp(LateralRetentionFraction, 0.0, 1.0);
    return Fwd * FwdSpeed + Rgt * (LatSpeed * Retained) + Residual;
}

double FVehicleHandling::EffectiveTopSpeed(double BaseTopSpeed, double TopSpeedFactor)
{
    return FMath::Max(0.0, BaseTopSpeed) * FMath::Max(0.0, TopSpeedFactor);
}

void FVehicleHandling::FDriftScorer::Update(double DriftFactorValue, double Speed, double Dt)
{
    if (DriftFactorValue < MinDriftFactor || Speed <= 0.0 || Dt <= 0.0)
    {
        return; // not drifting — hold the run, don't grow or lose it
    }
    DriftTime += Dt;
    Multiplier = FMath::Min(MaxMultiplier, 1.0 + DriftTime * MultiplierPerSec);
    Score += DriftFactorValue * Speed * Dt * ScoreRate;
}

double FVehicleHandling::FDriftScorer::Bank()
{
    const double Payout = Score * Multiplier;
    Score = 0.0;
    Multiplier = 1.0;
    DriftTime = 0.0;
    return Payout;
}

double FVehicleHandling::FDriftScorer::Wipe()
{
    const double Lost = Score;
    Score = 0.0;
    Multiplier = 1.0;
    DriftTime = 0.0;
    return Lost;
}
