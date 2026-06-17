// Copyright (c) 2026 GTC contributors

#include "VehicleHealth.h"

#include <limits>

FVehicleHealth::FVehicleHealth(double StartingMaxHealth, double FireThreshold, double FuseSeconds)
{
    MaxHealthValue = FMath::Max(StartingMaxHealth, 0.0);
    // Cap the fire band below the SMOKING band so a fire_threshold > 0.33 can't route a
    // merely-DAMAGED car (fraction in [0.33, 0.66)) straight to ON_FIRE.
    FireThresholdFraction = FMath::Clamp(FireThreshold, 0.0, SmokingFraction);
    FuseDuration = FMath::Max(FuseSeconds, 0.0);
    HealthValue = MaxHealthValue;
    FuseRemaining = std::numeric_limits<double>::infinity();
    RefreshState();
}

void FVehicleHealth::ApplyDamage(double Amount)
{
    if (Amount <= 0.0 || StateValue == EState::Wrecked)
    {
        return;
    }
    HealthValue = FMath::Max(HealthValue - Amount, 0.0);
    RefreshState();
}

void FVehicleHealth::Tick(double Delta)
{
    if (Delta <= 0.0 || StateValue != EState::OnFire)
    {
        return;
    }
    if (BurnRate > 0.0)
    {
        HealthValue = FMath::Max(HealthValue - BurnRate * Delta, 0.0);
    }
    FuseRemaining = FMath::Max(FuseRemaining - Delta, 0.0);
    if (FuseRemaining <= 0.0 || HealthValue <= 0.0)
    {
        Explode();
    }
    else
    {
        RefreshState();
    }
}

double FVehicleHealth::Health() const
{
    return HealthValue;
}

double FVehicleHealth::HealthFraction() const
{
    if (MaxHealthValue <= 0.0)
    {
        return 0.0;
    }
    return FMath::Clamp(HealthValue / MaxHealthValue, 0.0, 1.0);
}

FVehicleHealth::EState FVehicleHealth::State() const
{
    return StateValue;
}

bool FVehicleHealth::IsOnFire() const
{
    return StateValue == EState::OnFire;
}

bool FVehicleHealth::IsWrecked() const
{
    return StateValue == EState::Wrecked;
}

double FVehicleHealth::TimeToExplosion() const
{
    if (StateValue == EState::Wrecked)
    {
        return 0.0;
    }
    if (StateValue != EState::OnFire)
    {
        return std::numeric_limits<double>::infinity();
    }
    return FuseRemaining;
}

bool FVehicleHealth::JustExploded()
{
    const bool bFired = bJustExploded;
    bJustExploded = false;
    return bFired;
}

void FVehicleHealth::Repair()
{
    Reset();
}

void FVehicleHealth::Reset()
{
    HealthValue = MaxHealthValue;
    FuseRemaining = std::numeric_limits<double>::infinity();
    bJustExploded = false;
    RefreshState();
}

void FVehicleHealth::Explode()
{
    HealthValue = 0.0;
    FuseRemaining = 0.0;
    StateValue = EState::Wrecked;
    bJustExploded = true;
}

void FVehicleHealth::RefreshState()
{
    if (HealthValue <= 0.0)
    {
        Explode();
        return;
    }
    const double Fraction = HealthFraction();
    if (Fraction < FireThresholdFraction)
    {
        EnterFire();
    }
    else if (Fraction < SmokingFraction)
    {
        StateValue = EState::Smoking;
    }
    else if (Fraction < DamagedFraction)
    {
        StateValue = EState::Damaged;
    }
    else
    {
        StateValue = EState::Pristine;
    }
}

void FVehicleHealth::EnterFire()
{
    if (StateValue != EState::OnFire)
    {
        FuseRemaining = FuseDuration;
    }
    StateValue = EState::OnFire;
}
