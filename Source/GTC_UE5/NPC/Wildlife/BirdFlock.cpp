// Copyright Epic Games, Inc. All Rights Reserved.

#include "BirdFlock.h"

void FBirdFlock::Update(bool bStartle, double Dt, const FParams& Params)
{
    const double Step = FMath::Max(0.0, Dt);

    // A startle keeps resetting the calm clock; otherwise it ticks up while undisturbed.
    if (bStartle)
    {
        CalmTime = 0.0;
    }
    else
    {
        CalmTime += Step;
    }

    switch (CurrentPhase)
    {
    case EPhase::Perched:
        if (bStartle)
        {
            CurrentPhase = EPhase::Startled;
            PhaseTime = 0.0;
        }
        break;

    case EPhase::Startled:
        PhaseTime += Step;
        if (PhaseTime >= FMath::Max(0.0, Params.BurstSeconds))
        {
            CurrentPhase = EPhase::Wheeling;
            PhaseTime = 0.0;
        }
        break;

    case EPhase::Wheeling:
        PhaseTime += Step;
        // Come down only once they've circled long enough AND nothing has spooked them recently.
        if (PhaseTime >= FMath::Max(0.0, Params.MinWheelSeconds)
            && CalmTime >= FMath::Max(0.0, Params.CalmBeforeSettle))
        {
            CurrentPhase = EPhase::Settling;
            PhaseTime = 0.0;
        }
        break;

    case EPhase::Settling:
        if (bStartle)
        {
            // Re-spooked mid-descent: abandon the landing and burst again.
            CurrentPhase = EPhase::Startled;
            PhaseTime = 0.0;
        }
        else
        {
            PhaseTime += Step;
            if (PhaseTime >= FMath::Max(0.0, Params.SettleSeconds))
            {
                CurrentPhase = EPhase::Perched;
                PhaseTime = 0.0;
            }
        }
        break;
    }
}

double FBirdFlock::Altitude(const FParams& Params) const
{
    switch (CurrentPhase)
    {
    case EPhase::Perched:
        return 0.0;

    case EPhase::Startled:
    {
        const double Burst = FMath::Max(0.0, Params.BurstSeconds);
        return Burst > 0.0 ? FMath::Clamp(PhaseTime / Burst, 0.0, 1.0) : 1.0;
    }

    case EPhase::Wheeling:
        return 1.0;

    case EPhase::Settling:
    {
        const double Settle = FMath::Max(0.0, Params.SettleSeconds);
        return Settle > 0.0 ? FMath::Clamp(1.0 - PhaseTime / Settle, 0.0, 1.0) : 0.0;
    }
    }

    return 0.0;
}
