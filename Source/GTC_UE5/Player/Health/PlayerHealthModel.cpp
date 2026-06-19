// Copyright (c) 2026 GTC contributors

#include "PlayerHealthModel.h"

// ============================================================================
// FPlayerHealthModel — pure health/regen/armor (1:1 with player_health_model.gd)
// ============================================================================

FPlayerHealthModel::FPlayerHealthModel(double Maximum, double Regen, double Delay, double ArmorMax)
{
    // Godot _init: max_health = maxf(maximum, 0.0001); health = max_health; ...
    MaxHealth = FMath::Max(Maximum, 0.0001);
    Health = MaxHealth;
    RegenRate = Regen;
    RegenDelay = Delay;
    MaxArmor = FMath::Max(ArmorMax, 0.0);
    Armor = 0.0;
    SinceDamage = 0.0;
}

bool FPlayerHealthModel::Apply(double Amount)
{
    if (IsDead())
    {
        return false;
    }
    // remaining = maxf(amount, 0.0); armor soaks first 1:1, rest hits health.
    double Remaining = FMath::Max(Amount, 0.0);
    const double Absorbed = FMath::Min(Armor, Remaining);
    Armor -= Absorbed;
    Remaining -= Absorbed;
    Health = FMath::Max(Health - Remaining, 0.0);
    SinceDamage = 0.0;
    return IsDead();
}

void FPlayerHealthModel::AddArmor(double Amount)
{
    // clampf(armor + maxf(amount, 0.0), 0.0, max_armor)
    Armor = FMath::Clamp(Armor + FMath::Max(Amount, 0.0), 0.0, MaxArmor);
}

double FPlayerHealthModel::ArmorFraction() const
{
    return MaxArmor > 0.0 ? Armor / MaxArmor : 0.0;
}

void FPlayerHealthModel::Tick(double Delta)
{
    if (IsDead())
    {
        return;
    }
    SinceDamage += Delta;
    if (SinceDamage >= RegenDelay)
    {
        // Only regenerate the slice of `delta` that fell PAST the delay, so the
        // frame that crosses the threshold doesn't grant a full frame's regen for
        // time spent still in the cooldown.
        const double Over = SinceDamage - RegenDelay;
        Health = FMath::Min(Health + RegenRate * FMath::Min(Delta, Over), MaxHealth);
    }
}

void FPlayerHealthModel::Heal(double Amount)
{
    // No silent revive: a heal on a dead model is a no-op.
    if (IsDead())
    {
        return;
    }
    Health = FMath::Min(Health + FMath::Max(Amount, 0.0), MaxHealth);
}

double FPlayerHealthModel::Fraction() const
{
    return Health / MaxHealth;
}

void FPlayerHealthModel::Revive()
{
    Health = MaxHealth;
    Armor = 0.0;
    SinceDamage = RegenDelay;
}
