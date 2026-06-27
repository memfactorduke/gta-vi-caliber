// Copyright Epic Games, Inc. All Rights Reserved.

#include "RoadGrip.h"

double FRoadGrip::AquaplaneAmount(double Wetness, double SpeedKmh, const FParams& Params)
{
    const double W = FMath::Clamp(Wetness, 0.0, 1.0);
    if (W <= 0.0)
    {
        return 0.0; // a dry road never aquaplanes
    }

    const double Speed = FMath::Max(0.0, SpeedKmh);
    const double Begin = Params.AquaplaneSpeedKmh;
    const double Full = Params.AquaplaneSpeedFullKmh;

    // Speed ramp between the two thresholds. A degenerate (Full <= Begin) window is a
    // hard step at Begin.
    double SpeedRamp;
    if (Full <= Begin)
    {
        SpeedRamp = Speed >= Begin ? 1.0 : 0.0;
    }
    else
    {
        SpeedRamp = FMath::Clamp((Speed - Begin) / (Full - Begin), 0.0, 1.0);
    }

    // More water -> more aquaplaning; it takes both water and speed.
    return W * SpeedRamp;
}

double FRoadGrip::GripFactor(double Wetness, double SpeedKmh, const FParams& Params)
{
    const double W = FMath::Clamp(Wetness, 0.0, 1.0);
    if (W <= 0.0)
    {
        return 1.0; // dry road, full grip at any speed
    }

    // A wet road is simply less grippy, before any aquaplaning.
    const double BaseWet = 1.0 - W * FMath::Clamp(Params.WetGripLoss, 0.0, 1.0);

    // Aquaplaning then drags grip from that wet baseline toward the floor.
    const double Aqua = AquaplaneAmount(W, SpeedKmh, Params);
    const double Floor = FMath::Clamp(Params.AquaplaneFloor, 0.0, 1.0);
    const double Grip = BaseWet * (1.0 - Aqua) + Floor * Aqua;

    return FMath::Clamp(Grip, 0.0, 1.0);
}

bool FRoadGrip::IsAquaplaning(double Wetness, double SpeedKmh, const FParams& Params)
{
    return AquaplaneAmount(Wetness, SpeedKmh, Params) > 0.0;
}
