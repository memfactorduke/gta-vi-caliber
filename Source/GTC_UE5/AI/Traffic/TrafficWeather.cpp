// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrafficWeather.h"

namespace
{
    // Total speed bleed (0..maxLoss) from rain + fog, both clamped non-negative.
    double SpeedReduction(double Wetness, double Visibility, const FTrafficWeather::FParams& Params)
    {
        const double W = FMath::Clamp(Wetness, 0.0, 1.0);
        const double Fog = 1.0 - FMath::Clamp(Visibility, 0.0, 1.0); // 0 clear .. 1 whiteout
        const double WetLoss = FMath::Max(0.0, Params.WetSpeedLoss);
        const double FogLoss = FMath::Max(0.0, Params.FogSpeedLoss);
        return WetLoss * W + FogLoss * Fog;
    }
}

double FTrafficWeather::SpeedFactor(double Wetness, double Visibility, const FParams& Params)
{
    const double Floor = FMath::Clamp(Params.MinSpeedFactor, 0.0, 1.0);
    const double Reduction = SpeedReduction(Wetness, Visibility, Params);
    return FMath::Clamp(1.0 - Reduction, Floor, 1.0);
}

double FTrafficWeather::HeadwayFactor(double Wetness, double Visibility, const FParams& Params)
{
    // Severity is the speed bleed as a fraction of the worst possible bleed, so the
    // headway gain tracks the slowdown from the same number.
    const double MaxReduction = FMath::Max(0.0, Params.WetSpeedLoss) + FMath::Max(0.0, Params.FogSpeedLoss);
    const double Severity = MaxReduction > 0.0
        ? FMath::Clamp(SpeedReduction(Wetness, Visibility, Params) / MaxReduction, 0.0, 1.0)
        : 0.0;
    const double Gain = FMath::Max(1.0, Params.MaxHeadwayGain);
    return FMath::Lerp(1.0, Gain, Severity);
}

double FTrafficWeather::AdjustedDesiredSpeed(double BaseDesiredSpeed, double Wetness, double Visibility, const FParams& Params)
{
    return BaseDesiredSpeed * SpeedFactor(Wetness, Visibility, Params);
}

double FTrafficWeather::AdjustedTimeHeadway(double BaseTimeHeadway, double Wetness, double Visibility, const FParams& Params)
{
    return BaseTimeHeadway * HeadwayFactor(Wetness, Visibility, Params);
}
