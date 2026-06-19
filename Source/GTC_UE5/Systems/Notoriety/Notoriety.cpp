// Copyright Epic Games, Inc. All Rights Reserved.

#include "Notoriety.h"

#include "Math/UnrealMathUtility.h"

void FNotoriety::Add(double Amount)
{
    if (Amount <= 0.0)
    {
        return;
    }
    Notoriety += Amount;
}

void FNotoriety::Decay(double Dt)
{
    if (Dt <= 0.0)
    {
        return;
    }
    Notoriety = FMath::Max(0.0, Notoriety - DecayPerSec * Dt);
    HunterTimer = FMath::Max(0.0, HunterTimer - Dt);
}

ENotorietyTier FNotoriety::Tier() const
{
    if (Notoriety >= LegendaryAt)
    {
        return ENotorietyTier::Legendary;
    }
    if (Notoriety >= NotoriousAt)
    {
        return ENotorietyTier::Notorious;
    }
    if (Notoriety >= KnownAt)
    {
        return ENotorietyTier::Known;
    }
    return ENotorietyTier::Anonymous;
}

double FNotoriety::BountyAmount() const
{
    if (Notoriety < KnownAt)
    {
        return 0.0; // a nobody isn't worth a bounty
    }
    return (Notoriety - KnownAt) * BountyPerPoint;
}

bool FNotoriety::HunterReady() const
{
    return Tier() != ENotorietyTier::Anonymous && HunterTimer <= 0.0;
}

int32 FNotoriety::WaveSize() const
{
    switch (Tier())
    {
    case ENotorietyTier::Legendary: return 4;
    case ENotorietyTier::Notorious: return 2;
    case ENotorietyTier::Known:     return 1;
    default:                        return 0;
    }
}
