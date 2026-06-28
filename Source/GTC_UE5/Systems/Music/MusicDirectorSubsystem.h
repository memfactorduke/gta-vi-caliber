// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MusicDirector.h"
#include "MusicDirectorSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGtcOnMusicLayerChanged, int32, Layer);

/**
 * UMusicDirectorSubsystem — the interactive non-diegetic score (UE adapter over
 * FMusicDirector).
 *
 * A GameInstanceSubsystem (global score, outlives sublevels). Gameplay cooks a 0..1
 * heat (wanted stars, gunfight, chase) and calls UpdateMusic(Heat, Dt) each frame;
 * the audio adapter crossfades stems on OnMusicLayerChanged and reads GetIntensity().
 */
UCLASS()
class GTC_UE5_API UMusicDirectorSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Music")
    float AttackPerSec = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Music")
    float ReleasePerSec = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Music")
    int32 LayerCount = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Music")
    float Hysteresis = 0.06f;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Music")
    FGtcOnMusicLayerChanged OnMusicLayerChanged;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "GTC|Music")
    void ApplyTuning();

    /** Advance the score one tick toward Heat (0..1) over DeltaSeconds. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Music")
    void UpdateMusic(float Heat, float DeltaSeconds);

    UFUNCTION(BlueprintPure, Category = "GTC|Music")
    float GetIntensity() const { return static_cast<float>(Model.Intensity()); }

    UFUNCTION(BlueprintPure, Category = "GTC|Music")
    int32 GetLayer() const { return Model.Layer(); }

    UFUNCTION(BlueprintPure, Category = "GTC|Music")
    int32 GetLayerCount() const { return Model.LayerCount(); }

private:
    FMusicDirector Model;
    int32 LastLayer = 0;
};
