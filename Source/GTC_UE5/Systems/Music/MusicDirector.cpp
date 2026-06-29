// Copyright Epic Games, Inc. All Rights Reserved.

#include "MusicDirector.h"

void FMusicDirector::Configure(const FParams& InParams)
{
    Params = InParams;
    CurrentIntensity = 0.0;
    CurrentLayer = 0;
}

int32 FMusicDirector::LayerCount() const
{
    return FMath::Max(1, Params.LayerCount);
}

void FMusicDirector::Update(double Heat, double Dt)
{
    const double Target = FMath::Clamp(Heat, 0.0, 1.0);
    const double Step = FMath::Max(0.0, Dt);

    // Asymmetric one-pole smoothing: rush up at the attack rate, bleed down at the
    // (slower) release rate, so the score hits hard then tails out.
    const double Rate = (Target > CurrentIntensity) ? Params.AttackPerSec : Params.ReleasePerSec;
    const double Frac = FMath::Clamp(FMath::Max(0.0, Rate) * Step, 0.0, 1.0);
    CurrentIntensity = FMath::Clamp(CurrentIntensity + (Target - CurrentIntensity) * Frac, 0.0, 1.0);

    // Re-evaluate the discrete stem layer with hysteresis: rise into a hotter stem at
    // a higher threshold than the one we'd fall back down through.
    const int32 N = LayerCount();
    const double Half = 0.5 * FMath::Max(0.0, Params.Hysteresis);

    // Climb while we're clear of the next boundary up.
    while (CurrentLayer < N - 1
        && CurrentIntensity >= static_cast<double>(CurrentLayer + 1) / N + Half)
    {
        ++CurrentLayer;
    }
    // Drop while we're clear below the boundary into the layer beneath.
    while (CurrentLayer > 0
        && CurrentIntensity < static_cast<double>(CurrentLayer) / N - Half)
    {
        --CurrentLayer;
    }
}
