// Copyright Epic Games, Inc. All Rights Reserved.

#include "WildlifeBehavior.h"

void FWildlifeBehavior::Update(double DistanceToThreat, bool bProvoked, double Dt, const FParams& Params)
{
    const double Distance = FMath::Max(0.0, DistanceToThreat);
    const double Notice = FMath::Max(0.0, Params.NoticeRange);
    // Personal space can never exceed the notice ring (you can't react before you notice).
    const double React = FMath::Clamp(Params.ReactRange, 0.0, Notice);
    const double Step = FMath::Max(0.0, Dt);

    const bool bThreatPresent = Distance <= Notice;

    // The calm-down timer only runs while the threat is gone; any presence (or a
    // provoke) resets it, so the animal won't settle until it's truly been left alone.
    if (bThreatPresent || bProvoked)
    {
        CalmTimer = 0.0;
    }
    else
    {
        CalmTimer += Step;
    }

    const bool bCalmedDown = CalmTimer >= FMath::Max(0.0, Params.CalmSeconds);

    // A provoke forces an immediate escalation by temperament, regardless of distance.
    if (bProvoked)
    {
        CurrentState = (Params.Temperament == ETemperament::Aggressive) ? EState::Charge : EState::Flee;
        return;
    }

    switch (CurrentState)
    {
    case EState::Idle:
        if (bThreatPresent)
        {
            CurrentState = EState::Alert;
        }
        break;

    case EState::Alert:
        if (bThreatPresent && Distance <= React)
        {
            // Crowded: skittish bolts, aggressive charges, placid just stays wary.
            if (Params.Temperament == ETemperament::Skittish)
            {
                CurrentState = EState::Flee;
            }
            else if (Params.Temperament == ETemperament::Aggressive)
            {
                CurrentState = EState::Charge;
            }
        }
        else if (!bThreatPresent && bCalmedDown)
        {
            CurrentState = EState::Idle;
        }
        break;

    case EState::Flee:
    case EState::Charge:
        // Keep fleeing / charging until the threat has been gone long enough to settle.
        if (!bThreatPresent && bCalmedDown)
        {
            CurrentState = EState::Idle;
        }
        break;
    }
}

double FWildlifeBehavior::CalmProgress(const FParams& Params) const
{
    if (CurrentState == EState::Idle)
    {
        return 0.0;
    }

    const double Calm = FMath::Max(0.0, Params.CalmSeconds);
    if (Calm <= 0.0)
    {
        return 1.0; // no settling delay -> always "ready to calm"
    }
    return FMath::Clamp(CalmTimer / Calm, 0.0, 1.0);
}
