// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcVitals.h"

// ============================================================================
// FNpcVitals — pure, disposable pedestrian health (no regen, no armor)
// ============================================================================

FNpcVitals::FNpcVitals(double Maximum)
{
    MaxHealth = FMath::Max(Maximum, 0.0001);
    Health = MaxHealth;
}

double FNpcVitals::Fraction() const
{
    return Health / MaxHealth;
}

ENpcHitResult FNpcVitals::Apply(double Amount)
{
    if (IsDead() || Amount <= 0.0)
    {
        return ENpcHitResult::Ignored;
    }
    Health = FMath::Max(Health - Amount, 0.0);
    return IsDead() ? ENpcHitResult::Killed : ENpcHitResult::Wounded;
}

double FNpcVitals::WoundSeverity(double Amount, double MaxHealth)
{
    const double Ceiling = FMath::Max(MaxHealth, 0.0001);
    return FMath::Clamp(Amount / Ceiling, 0.0, 1.0);
}
