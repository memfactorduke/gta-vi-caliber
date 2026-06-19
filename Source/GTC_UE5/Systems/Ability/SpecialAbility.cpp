// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpecialAbility.h"

#include "Math/UnrealMathUtility.h"

void FSpecialAbility::AddCharge(double Amount)
{
    if (Amount <= 0.0)
    {
        return;
    }
    Charge = FMath::Clamp(Charge + Amount, 0.0, 1.0);
}

bool FSpecialAbility::Activate()
{
    if (!CanActivate())
    {
        return false;
    }
    bActive = true;
    return true;
}

void FSpecialAbility::Update(double Dt)
{
    if (!bActive || Dt <= 0.0)
    {
        return;
    }
    Charge -= DrainPerSec * Dt;
    if (Charge <= 0.0)
    {
        Charge = 0.0;
        bActive = false; // ran dry
    }
}

double FSpecialAbility::TimeDilation() const
{
    if (!bActive)
    {
        return 1.0;
    }
    switch (Kind)
    {
    case EAbilityKind::Marksman: return 0.4; // heavy slow-mo for precision shots
    case EAbilityKind::Driver:   return 0.8; // a touch of focus time at the wheel
    default:                     return 1.0;
    }
}

double FSpecialAbility::DamageTakenMultiplier() const
{
    if (!bActive)
    {
        return 1.0;
    }
    return Kind == EAbilityKind::Bruiser ? 0.5 : 1.0;
}

double FSpecialAbility::DamageDealtMultiplier() const
{
    if (!bActive)
    {
        return 1.0;
    }
    return Kind == EAbilityKind::Bruiser ? 1.25 : 1.0;
}

double FSpecialAbility::HandlingBoost() const
{
    if (!bActive)
    {
        return 1.0;
    }
    return Kind == EAbilityKind::Driver ? 1.25 : 1.0;
}
