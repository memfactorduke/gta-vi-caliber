// Copyright Epic Games, Inc. All Rights Reserved.

#include "HostageStandoff.h"

void FHostageStandoff::Configure(const FParams& InParams)
{
    Params = InParams;
    CurrentState = EState::None;
    Struggle = 0.0;
    IntimidateCount = 0;
}

void FHostageStandoff::Grab()
{
    CurrentState = EState::Holding;
    Struggle = 0.0;
    IntimidateCount = 0;
}

void FHostageStandoff::Update(bool bMoving, double Dt)
{
    if (CurrentState != EState::Holding)
    {
        return;
    }
    const double Step = FMath::Max(0.0, Dt);
    if (Step <= 0.0)
    {
        return;
    }

    const double Rate = FMath::Max(0.0, Params.StrugglePerSec)
        * (bMoving ? FMath::Max(0.0, Params.MoveStruggleMult) : 1.0);
    Struggle += Rate * Step;

    if (Struggle >= 1.0)
    {
        Struggle = 1.0;
        CurrentState = EState::BrokeFree; // shield's gone
    }
}

double FHostageStandoff::Intimidate()
{
    if (CurrentState != EState::Holding)
    {
        return 0.0;
    }

    // Geometric falloff: the Nth shout is Falloff^N as effective as the first.
    const double Falloff = FMath::Clamp(Params.IntimidateFalloff, 0.0, 1.0);
    const double Relief = FMath::Max(0.0, Params.IntimidateRelief) * FMath::Pow(Falloff, static_cast<double>(IntimidateCount));

    const double Before = Struggle;
    Struggle = FMath::Max(0.0, Struggle - Relief);
    ++IntimidateCount;
    return Before - Struggle;
}

void FHostageStandoff::Release()
{
    if (CurrentState == EState::Holding)
    {
        CurrentState = EState::Released;
    }
}

double FHostageStandoff::StruggleFraction() const
{
    return CurrentState == EState::Holding ? FMath::Clamp(Struggle, 0.0, 1.0) : 0.0;
}
