// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TrafficWeatherLibrary.generated.h"

/**
 * UTrafficWeatherLibrary — Blueprint surface for the static FTrafficWeather model.
 *
 * FTrafficWeather is pure-core, all-static math that turns wetness + visibility
 * into an IDM speed factor and headway factor for ambient traffic. This library
 * re-exposes it so the traffic adapter (or a BP tuning pass) can fold the weather
 * into each agent's DesiredSpeed (v0) / TimeHeadway (T). Tuning is defaulted to
 * FTrafficWeather::FParams defaults.
 */
UCLASS()
class GTC_UE5_API UTrafficWeatherLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Cruise-speed multiplier (0..1) for Wetness/Visibility (Visibility 1 = clear). */
    UFUNCTION(BlueprintPure, Category = "GTC|AI|Traffic")
    static float TrafficSpeedFactor(float Wetness, float Visibility,
        float WetSpeedLoss = 0.25f, float FogSpeedLoss = 0.35f,
        float MinSpeedFactor = 0.55f, float MaxHeadwayGain = 1.8f);

    /** Time-headway multiplier (>= 1) for the given conditions. */
    UFUNCTION(BlueprintPure, Category = "GTC|AI|Traffic")
    static float TrafficHeadwayFactor(float Wetness, float Visibility,
        float WetSpeedLoss = 0.25f, float FogSpeedLoss = 0.35f,
        float MinSpeedFactor = 0.55f, float MaxHeadwayGain = 1.8f);

    /** An agent's base cruise (v0) scaled by the speed factor. */
    UFUNCTION(BlueprintPure, Category = "GTC|AI|Traffic")
    static float TrafficAdjustedDesiredSpeed(float BaseDesiredSpeed, float Wetness, float Visibility,
        float WetSpeedLoss = 0.25f, float FogSpeedLoss = 0.35f,
        float MinSpeedFactor = 0.55f, float MaxHeadwayGain = 1.8f);

    /** An agent's base time gap (T) scaled by the headway factor. */
    UFUNCTION(BlueprintPure, Category = "GTC|AI|Traffic")
    static float TrafficAdjustedTimeHeadway(float BaseTimeHeadway, float Wetness, float Visibility,
        float WetSpeedLoss = 0.25f, float FogSpeedLoss = 0.35f,
        float MinSpeedFactor = 0.55f, float MaxHeadwayGain = 1.8f);
};
