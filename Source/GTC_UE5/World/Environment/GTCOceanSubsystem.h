// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OceanSurface.h" // FGerstnerWave / FOceanSample
#include "GTCOceanSubsystem.generated.h"

/**
 * UGTCOceanSubsystem — the ONE owner of the live Gerstner wave-set + sea level, so the
 * physics that floats a boat and the material that renders the water read the SAME waves
 * (per OceanSurface.h: the physics surface and the rendered surface must agree). It is also
 * the single cm<->m bridge: the flight/handling cores are cm, FOceanSurface is SI metres,
 * so every sample goes through HeightAtCm/SampleCm here and the conversion can't drift.
 *
 * No tick needed — wave phase is read from the world clock. The water-material adapter
 * reads GetWaves() to drive the same crests on screen.
 */
UCLASS()
class GTC_UE5_API UGTCOceanSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    /** Unreal units per metre — the single cm<->m factor for the ocean seam. */
    static constexpr double UuPerM = 100.0;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /** Mean sea level (cm, Z-up). The boat floats relative to this. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Ocean")
    double SeaLevelZCm() const { return SeaLevelZ; }

    UFUNCTION(BlueprintCallable, Category = "GTC|Ocean")
    void SetSeaLevelZCm(double InZ) { SeaLevelZ = InZ; }

    /** Surface height (cm) at a world (X,Y) in cm — buoyancy's hot path. */
    double HeightAtCm(double Xcm, double Ycm) const;

    /** Full surface sample (height/normal/jacobian) at a world (X,Y) in cm. Height is cm. */
    FOceanSample SampleCm(double Xcm, double Ycm) const;

    /** Replace the wave-set (also feed the same set to the water material). */
    void SetWaves(const TArray<FGerstnerWave>& InWaves) { Waves = InWaves; }
    const TArray<FGerstnerWave>& GetWaves() const { return Waves; }

private:
    /** Current wave phase time (seconds) from the world clock. */
    double NowSeconds() const;

    TArray<FGerstnerWave> Waves;
    double SeaLevelZ = 0.0;
};
