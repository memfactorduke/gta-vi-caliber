// Copyright Epic Games, Inc. All Rights Reserved.

#include "TailObjective.h"

void FTailObjective::Configure(const FParams& InParams)
{
    Params = InParams;
    CurrentState = EState::Tailing;
    SuspicionValue = 0.0;
    TrackedTime = 0.0;
    LostTimer = 0.0;
}

double FTailObjective::TrackProgress() const
{
    const double Required = FMath::Max<double>(KINDA_SMALL_NUMBER, Params.RequiredTrackSeconds);
    return FMath::Clamp(TrackedTime / Required, 0.0, 1.0);
}

void FTailObjective::Update(double Distance, bool bInTargetView, double Dt)
{
    if (CurrentState != EState::Tailing)
    {
        return;
    }
    const double Step = FMath::Max(0.0, Dt);
    if (Step <= 0.0)
    {
        return;
    }

    const double Fall = FMath::Max(0.0, Params.SuspicionFallPerSec);

    if (Distance > Params.MaxDistance)
    {
        // Out of range: the lost clock runs; the target can't see you, so suspicion cools.
        LostTimer += Step;
        SuspicionValue = FMath::Max(0.0, SuspicionValue - Fall * Step);
        if (LostTimer >= FMath::Max(0.0, Params.LostGraceSeconds))
        {
            CurrentState = EState::FailedLost;
        }
        return; // no tracking progress while out of range
    }

    // Back in range: reset the lost clock.
    LostTimer = 0.0;

    const bool bRisk = (Distance < Params.MinDistance) || bInTargetView;
    if (bRisk)
    {
        SuspicionValue += FMath::Max(0.0, Params.SuspicionRisePerSec) * Step;
        if (SuspicionValue >= 1.0)
        {
            SuspicionValue = 1.0;
            CurrentState = EState::FailedSpotted;
            return;
        }
    }
    else
    {
        SuspicionValue = FMath::Max(0.0, SuspicionValue - Fall * Step);
    }

    // Riding the band counts toward the objective.
    TrackedTime += Step;
    if (TrackedTime >= FMath::Max<double>(KINDA_SMALL_NUMBER, Params.RequiredTrackSeconds))
    {
        CurrentState = EState::Succeeded;
    }
}
