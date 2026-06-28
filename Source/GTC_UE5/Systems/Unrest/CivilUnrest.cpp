// Copyright Epic Games, Inc. All Rights Reserved.

#include "CivilUnrest.h"

void FCivilUnrest::Configure(const FParams& InParams)
{
    Params = InParams;
    UnrestValue = 0.0;
}

void FCivilUnrest::Provoke(double Amount)
{
    UnrestValue = FMath::Clamp(UnrestValue + FMath::Max(0.0, Amount), 0.0, 1.0);
}

void FCivilUnrest::Suppress(double Amount)
{
    UnrestValue = FMath::Clamp(UnrestValue - FMath::Max(0.0, Amount), 0.0, 1.0);
}

void FCivilUnrest::Advance(double Dt)
{
    const double Step = FMath::Max(0.0, Dt);
    if (Step <= 0.0)
    {
        return;
    }

    // The one branch that makes it bistable: self-feed past the flashpoint, calm below it.
    if (UnrestValue >= Params.Flashpoint)
    {
        UnrestValue += FMath::Max(0.0, Params.GrowthPerSec) * Step;
    }
    else
    {
        UnrestValue -= FMath::Max(0.0, Params.DecayPerSec) * Step;
    }
    UnrestValue = FMath::Clamp(UnrestValue, 0.0, 1.0);
}

FCivilUnrest::EStage FCivilUnrest::Stage() const
{
    if (UnrestValue >= Params.Flashpoint)
    {
        return EStage::Rioting;
    }
    if (UnrestValue >= Params.TenseAt)
    {
        return EStage::Tense;
    }
    return EStage::Calm;
}

bool FCivilUnrest::IsRioting() const
{
    return UnrestValue >= Params.Flashpoint;
}
