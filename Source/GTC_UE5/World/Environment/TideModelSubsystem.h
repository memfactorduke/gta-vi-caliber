// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TideModel.h"
#include "TideModelSubsystem.generated.h"

/**
 * UTideModelSubsystem — coastal tides for the waterfront (UE adapter over FTideModel).
 *
 * A WorldSubsystem (the sea belongs to the world). The tide is a pure function of the
 * world clock, so this holds only tuning and answers queries by the hours you pass
 * (world time-of-day + day count, or worldSeconds/3600). Feed LevelAt() into the ocean
 * surface datum; TimeToNextHigh()/Low() drive a "tide turns in 2h" tell.
 */
UCLASS()
class GTC_UE5_API UTideModelSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    /** The still-water datum the tide swings around (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Tide")
    float MeanLevel = 0.0f;

    /** Average semidiurnal amplitude — half the typical range (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Tide")
    float MeanAmplitude = 80.0f;

    /** How much the amplitude grows/shrinks across the spring-neap cycle (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Tide")
    float SpringNeapRange = 30.0f;

    /** Semidiurnal period (hours) — principal lunar M2 constituent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Tide")
    float PeriodHours = 12.42f;

    /** Spring-neap envelope period (hours). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Tide")
    float SpringNeapHours = 354.4f;

    /** Hour of a high tide — shifts the semidiurnal phase. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Tide")
    float PhaseHours = 0.0f;

    /** Hour of a spring (max-range) tide — shifts the envelope phase. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Tide")
    float SpringNeapPhaseHours = 0.0f;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /** Re-apply the tuning UPROPERTYs to the model. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Tide")
    void ApplyTuning();

    /** The still-water level (cm) at world time Hours. */
    UFUNCTION(BlueprintPure, Category = "GTC|Tide")
    float LevelAt(float Hours) const { return static_cast<float>(Model.LevelAt(Hours)); }

    /** Current semidiurnal envelope amplitude (cm) at Hours. */
    UFUNCTION(BlueprintPure, Category = "GTC|Tide")
    float AmplitudeAt(float Hours) const { return static_cast<float>(Model.AmplitudeAt(Hours)); }

    /** Where the level sits within the current envelope band, 0..1. */
    UFUNCTION(BlueprintPure, Category = "GTC|Tide")
    float NormalizedAt(float Hours) const { return static_cast<float>(Model.NormalizedAt(Hours)); }

    /** True when the water is coming in (flood tide) at Hours. */
    UFUNCTION(BlueprintPure, Category = "GTC|Tide")
    bool IsRisingAt(float Hours) const { return Model.IsRisingAt(Hours); }

    /** Hours until the next high tide from Hours. */
    UFUNCTION(BlueprintPure, Category = "GTC|Tide")
    float TimeToNextHigh(float Hours) const { return static_cast<float>(Model.TimeToNextHigh(Hours)); }

    /** Hours until the next low tide from Hours. */
    UFUNCTION(BlueprintPure, Category = "GTC|Tide")
    float TimeToNextLow(float Hours) const { return static_cast<float>(Model.TimeToNextLow(Hours)); }

private:
    FTideModel Model;
};
