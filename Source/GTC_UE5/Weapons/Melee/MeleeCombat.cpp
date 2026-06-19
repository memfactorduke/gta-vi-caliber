// Copyright Epic Games, Inc. All Rights Reserved.

#include "MeleeCombat.h"

#include "Math/UnrealMathUtility.h"

double FMeleeCombat::BaseDamage(EMeleeStrike Strike)
{
    return Strike == EMeleeStrike::Heavy ? HeavyDamage : LightDamage;
}

double FMeleeCombat::StaminaCost(EMeleeStrike Strike)
{
    return Strike == EMeleeStrike::Heavy ? HeavyStaminaCost : LightStaminaCost;
}

double FMeleeCombat::ComboMultiplier(int32 PriorHits)
{
    const int32 Hits = FMath::Max(0, PriorHits);
    return FMath::Min(MaxComboMultiplier, 1.0 + Hits * ComboStep);
}

double FMeleeCombat::ExhaustionFactor(double StaminaFraction)
{
    const double Frac = FMath::Clamp(StaminaFraction, 0.0, 1.0);
    return ExhaustionFloor + (1.0 - ExhaustionFloor) * Frac;
}

double FMeleeCombat::StrikeDamage(EMeleeStrike Strike, int32 PriorHits, double StaminaFraction)
{
    return BaseDamage(Strike) * ComboMultiplier(PriorHits) * ExhaustionFactor(StaminaFraction);
}

bool FMeleeCombat::IsPerfectParry(double BlockHeldSeconds)
{
    // A last-instant raise counts; a block held up since well before the blow does not
    // (you committed early, so it's just a normal block).
    return BlockHeldSeconds >= 0.0 && BlockHeldSeconds <= ParryWindow;
}

double FMeleeCombat::IncomingAfterBlock(double Incoming, EMeleeStrike Strike, bool bBlocking, bool bPerfectParry)
{
    const double Dmg = FMath::Max(0.0, Incoming);
    if (!bBlocking)
    {
        return Dmg;
    }
    if (bPerfectParry)
    {
        return 0.0; // parried clean
    }
    const double Retention = (Strike == EMeleeStrike::Heavy) ? HeavyBlockRetention : BlockRetention;
    return Dmg * Retention;
}

double FMeleeCombat::CounterDamage(double Incoming, bool bPerfectParry)
{
    return bPerfectParry ? FMath::Max(0.0, Incoming) * CounterScale : 0.0;
}

double FMeleeCombat::FMeleeFighter::Strike(EMeleeStrike InStrike)
{
    if (!CanStrike(InStrike))
    {
        return 0.0; // too gassed to throw it
    }

    Stamina = FMath::Max(0.0, Stamina - FMeleeCombat::StaminaCost(InStrike));

    // Damage uses the hits already landed (ComboCount) so the first hit is 1.0x.
    const double Dmg = FMeleeCombat::StrikeDamage(InStrike, ComboCount, StaminaFraction());

    ++ComboCount;
    ComboTimer = ComboWindow;
    return Dmg;
}

void FMeleeCombat::FMeleeFighter::Tick(double Dt)
{
    if (Dt <= 0.0)
    {
        return;
    }

    Stamina = FMath::Min(MaxStamina, Stamina + StaminaRegenPerSec * Dt);

    if (ComboTimer > 0.0)
    {
        ComboTimer -= Dt;
        if (ComboTimer <= 0.0)
        {
            ComboTimer = 0.0;
            ComboCount = 0; // the chain lapsed
        }
    }
}

void FMeleeCombat::FMeleeFighter::OnHitTaken()
{
    ComboCount = 0;
    ComboTimer = 0.0;
}
