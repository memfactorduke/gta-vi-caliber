// Copyright Epic Games, Inc. All Rights Reserved.

#include "FishingFight.h"

void FFishingFight::Configure(const FParams& InParams)
{
    Params = InParams;
    Result = EOutcome::Fighting;
    DistanceValue = 1.0;
    Stamina = 1.0;
    TensionValue = 0.0;
}

void FFishingFight::Update(double ReelInput, double Dt)
{
    if (Result != EOutcome::Fighting)
    {
        return; // the fight's already decided
    }

    const double Step = FMath::Max(0.0, Dt);
    if (Step <= 0.0)
    {
        return; // an idle/rewound frame doesn't advance the fight
    }

    const double Reel = FMath::Clamp(ReelInput, 0.0, 1.0);
    const double Pull = FMath::Clamp(Stamina, 0.0, 1.0); // a tired fish pulls weaker

    // Tension is your effort + the fish's pull + a penalty for fighting head-on.
    TensionValue = Reel * Params.TensionPerReel
        + Pull * Params.TensionPerPull
        + Reel * Pull * Params.TensionCombo;

    if (TensionValue > Params.LineStrength)
    {
        Result = EOutcome::Lost; // snap
        return;
    }

    // Reel against the fish: positive net brings it in, negative lets it run.
    DistanceValue += (Pull * Params.PullBack - Reel * Params.ReelRate) * Step;
    DistanceValue = FMath::Max(0.0, DistanceValue);

    // The fish tires whether or not you're reeling.
    Stamina = FMath::Max(0.0, Stamina - FMath::Max(0.0, Params.StaminaDrainPerSec) * Step);

    if (DistanceValue <= 0.0)
    {
        Result = EOutcome::Landed;
    }
}
