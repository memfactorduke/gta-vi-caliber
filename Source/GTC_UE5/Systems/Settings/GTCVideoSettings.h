// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GTCVideoSettings.generated.h"

class UWorld;

/**
 * Player-facing video settings, config-backed and persisted to Game.ini.
 *
 * First knob: "Sharpness" — a normalised 0..1 options-menu slider that drives
 * the global AGTCSmartSharpen post-process (unsharp mask that recovers detail
 * lost to TAA/TSR). The UI talks only in 0..1; the mapping to the effect's
 * internal gain lives in FSmartSharpen::UserSharpnessToAmount so it stays
 * unit-testable and the slider can't push the effect out of its safe range.
 *
 * Lightweight CDO-config pattern: read the live object via Get(), mutate, and
 * SaveConfig() to persist — no UGameUserSettings subclass / engine registration
 * needed. Any UI (the Slate options panel, or a Blueprint) can bind SetSharpness
 * to a slider and the effect updates live.
 */
UCLASS(Config = Game, DefaultConfig, BlueprintType)
class GTC_UE5_API UGTCVideoSettings : public UObject
{
    GENERATED_BODY()

public:
    /** Sharpen gain the slider maps to at 1.0 (matches AGTCSmartSharpen UIMax). */
    static constexpr float MaxUserSharpen = 2.0f;

    /** The persistent settings object (the config-loaded class-default object). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Video")
    static UGTCVideoSettings* Get();

    /** Normalised sharpness slider, 0 = off .. 1 = max. Persisted to Game.ini. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GTC|Video",
        meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float Sharpness = 0.5f;

    /** Resolve the sharpen gain the effect should use from the current Sharpness. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Video")
    float ResolveSharpenAmount() const;

    /** Set + persist the sharpness; if a World is given, apply it live as well. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Video")
    void SetSharpness(float NewSharpness, UWorld* World = nullptr);

    /**
     * Push the current Sharpness onto the world's global smart-sharpen actor,
     * spawning an unbound one if none exists. Safe to call repeatedly; a 0
     * slider just sets the amount to 0 (the effect stays inert).
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|Video")
    void ApplySharpen(UWorld* World);
};
