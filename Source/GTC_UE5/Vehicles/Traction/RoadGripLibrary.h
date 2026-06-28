// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RoadGripLibrary.generated.h"

/**
 * URoadGripLibrary — Blueprint surface for the static FRoadGrip wet-road model.
 *
 * FRoadGrip is pure-core, double-precision, all-static math (no UObject). This
 * library re-exposes it as BlueprintPure functions so the car BP can fold road
 * grip into FVehicleHandling's GripFactor seam without any new state. Tuning is
 * passed as defaulted float args (matching FRoadGrip::FParams defaults) so a
 * caller that just wants "wet grip from speed" can ignore them.
 */
UCLASS()
class GTC_UE5_API URoadGripLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Road grip factor (0..1) for the given Wetness (0..1) and SpeedKmh. 1.0 on a dry road. */
    UFUNCTION(BlueprintPure, Category = "GTC|Vehicles|Traction")
    static float RoadGripFactor(float Wetness, float SpeedKmh,
        float WetGripLoss = 0.35f, float AquaplaneSpeedKmh = 90.0f,
        float AquaplaneSpeedFullKmh = 160.0f, float AquaplaneFloor = 0.25f);

    /** How developed aquaplaning is, 0..1 (0 dry/slow, 1 full wet + full speed). */
    UFUNCTION(BlueprintPure, Category = "GTC|Vehicles|Traction")
    static float RoadAquaplaneAmount(float Wetness, float SpeedKmh,
        float WetGripLoss = 0.35f, float AquaplaneSpeedKmh = 90.0f,
        float AquaplaneSpeedFullKmh = 160.0f, float AquaplaneFloor = 0.25f);

    /** True when the car is aquaplaning (water present and over the aquaplane speed). */
    UFUNCTION(BlueprintPure, Category = "GTC|Vehicles|Traction")
    static bool RoadIsAquaplaning(float Wetness, float SpeedKmh,
        float WetGripLoss = 0.35f, float AquaplaneSpeedKmh = 90.0f,
        float AquaplaneSpeedFullKmh = 160.0f, float AquaplaneFloor = 0.25f);
};
