// Copyright Epic Games, Inc. All Rights Reserved.

#include "EscortObjective.h"

void FEscortObjective::Configure(const FParams& InParams)
{
    Params = InParams;
    CurrentState = EState::Escorting;
    IntegrityValue = 1.0;
    ProgressValue = 0.0;
}

void FEscortObjective::Update(double ProgressDelta, double ThreatLevel, double Dt)
{
    if (CurrentState != EState::Escorting)
    {
        return;
    }
    const double Step = FMath::Max(0.0, Dt);
    if (Step <= 0.0)
    {
        return;
    }

    const double Threat = FMath::Clamp(ThreatLevel, 0.0, 1.0);

    // Under fire it bleeds in proportion to the heat; suppressed, it recovers.
    if (Threat > Params.RegenThreatThreshold)
    {
        IntegrityValue -= Threat * FMath::Max(0.0, Params.DrainPerSec) * Step;
    }
    else
    {
        IntegrityValue += FMath::Max(0.0, Params.RegenPerSec) * Step;
    }
    IntegrityValue = FMath::Clamp(IntegrityValue, 0.0, 1.0);

    if (IntegrityValue <= 0.0)
    {
        CurrentState = EState::Lost;
        return;
    }

    ProgressValue = FMath::Clamp(ProgressValue + FMath::Max(0.0, ProgressDelta), 0.0, 1.0);
    if (ProgressValue >= 1.0)
    {
        CurrentState = EState::Delivered;
    }
}
