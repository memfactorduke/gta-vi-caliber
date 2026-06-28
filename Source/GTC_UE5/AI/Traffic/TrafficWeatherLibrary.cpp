// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrafficWeatherLibrary.h"
#include "TrafficWeather.h"

namespace
{
    // File-local helper (uniquely named to avoid unity-build collisions).
    FTrafficWeather::FParams MakeTrafficWeatherParams_TWL(float WetSpeedLoss, float FogSpeedLoss,
        float MinSpeedFactor, float MaxHeadwayGain)
    {
        FTrafficWeather::FParams P;
        P.WetSpeedLoss = static_cast<double>(WetSpeedLoss);
        P.FogSpeedLoss = static_cast<double>(FogSpeedLoss);
        P.MinSpeedFactor = static_cast<double>(MinSpeedFactor);
        P.MaxHeadwayGain = static_cast<double>(MaxHeadwayGain);
        return P;
    }
}

float UTrafficWeatherLibrary::TrafficSpeedFactor(float Wetness, float Visibility,
    float WetSpeedLoss, float FogSpeedLoss, float MinSpeedFactor, float MaxHeadwayGain)
{
    const FTrafficWeather::FParams P = MakeTrafficWeatherParams_TWL(WetSpeedLoss, FogSpeedLoss, MinSpeedFactor, MaxHeadwayGain);
    return static_cast<float>(FTrafficWeather::SpeedFactor(Wetness, Visibility, P));
}

float UTrafficWeatherLibrary::TrafficHeadwayFactor(float Wetness, float Visibility,
    float WetSpeedLoss, float FogSpeedLoss, float MinSpeedFactor, float MaxHeadwayGain)
{
    const FTrafficWeather::FParams P = MakeTrafficWeatherParams_TWL(WetSpeedLoss, FogSpeedLoss, MinSpeedFactor, MaxHeadwayGain);
    return static_cast<float>(FTrafficWeather::HeadwayFactor(Wetness, Visibility, P));
}

float UTrafficWeatherLibrary::TrafficAdjustedDesiredSpeed(float BaseDesiredSpeed, float Wetness, float Visibility,
    float WetSpeedLoss, float FogSpeedLoss, float MinSpeedFactor, float MaxHeadwayGain)
{
    const FTrafficWeather::FParams P = MakeTrafficWeatherParams_TWL(WetSpeedLoss, FogSpeedLoss, MinSpeedFactor, MaxHeadwayGain);
    return static_cast<float>(FTrafficWeather::AdjustedDesiredSpeed(BaseDesiredSpeed, Wetness, Visibility, P));
}

float UTrafficWeatherLibrary::TrafficAdjustedTimeHeadway(float BaseTimeHeadway, float Wetness, float Visibility,
    float WetSpeedLoss, float FogSpeedLoss, float MinSpeedFactor, float MaxHeadwayGain)
{
    const FTrafficWeather::FParams P = MakeTrafficWeatherParams_TWL(WetSpeedLoss, FogSpeedLoss, MinSpeedFactor, MaxHeadwayGain);
    return static_cast<float>(FTrafficWeather::AdjustedTimeHeadway(BaseTimeHeadway, Wetness, Visibility, P));
}
