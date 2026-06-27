// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTCSmartSharpen.generated.h"

class UPostProcessComponent;
class UBillboardComponent;
class UTextRenderComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * Drop-in smart-sharpen post-process actor.
 *
 * Place one in a level and it sharpens the whole scene (global / unbound) by
 * driving the M_GTC_SmartSharpen post-process material — an unsharp mask that
 * recovers detail lost to TAA/TSR blur. Place several with bUnbound off to
 * sharpen different areas by different amounts (local). All the per-pixel
 * "smart" logic (distance falloff, shadow-safe luminance gate, foliage blend,
 * resolution-independent kernel) lives in the material and is mirrored, for
 * testing, by FSmartSharpen — this actor only owns the placement and pushes the
 * tuning constants into a dynamic material instance.
 *
 * Lightweight by design: one UPostProcessComponent + one weighted blendable, no
 * tick. Works alongside TSR/TAA/FXAA/SMAA and both deferred and forward paths
 * because it runs as a normal post-process pass. The editor billboard + text
 * render show the live sharpen amount at a glance without selecting the actor.
 */
UCLASS(Blueprintable, ClassGroup = (GTC), meta = (DisplayName = "GTC Smart Sharpen"))
class GTC_UE5_API AGTCSmartSharpen : public AActor
{
    GENERATED_BODY()

public:
    AGTCSmartSharpen();

    /** Master sharpen amount for near surfaces (0 = off). Unsharp-mask gain. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Sharpen",
        meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "2.0"))
    float SharpenAmount = 1.0f;

    /** Sharpen kernel reach in pixels (resolution-independent — same look at any res). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Sharpen",
        meta = (ClampMin = "0.25", ClampMax = "4.0", UIMin = "0.5", UIMax = "2.0"))
    float PixelRadius = 1.0f;

    /** Luminance floor below which sharpening fades out (shadow safety, 0 = off). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Sharpen|Shadow Safety",
        meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.3"))
    float ShadowFloor = 0.05f;

    /** Sharpen distant surfaces by a different amount than near ones. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Sharpen|Distance")
    bool bDistanceControl = false;

    /** Sharpen amount applied at/beyond DistanceFar (used when bDistanceControl). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Sharpen|Distance",
        meta = (EditCondition = "bDistanceControl", ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "2.0"))
    float FarSharpenAmount = 0.4f;

    /** At/under this depth (cm) the near SharpenAmount applies. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Sharpen|Distance",
        meta = (EditCondition = "bDistanceControl", ClampMin = "0.0", Units = "cm"))
    float DistanceNear = 500.0f;

    /** At/beyond this depth (cm) the FarSharpenAmount applies. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Sharpen|Distance",
        meta = (EditCondition = "bDistanceControl", ClampMin = "0.0", Units = "cm"))
    float DistanceFar = 8000.0f;

    /** Apply a separate sharpen amount to foliage (custom-depth tagged). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Sharpen|Foliage")
    bool bFoliageControl = false;

    /** Sharpen amount for foliage pixels (used when bFoliageControl). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Sharpen|Foliage",
        meta = (EditCondition = "bFoliageControl", ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "2.0"))
    float FoliageAmount = 0.5f;

    /** Unbound = sharpen the whole scene (global). Off = local volume with falloff. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Sharpen|Influence")
    bool bUnbound = true;

    /** Blend strength of this actor's effect (local vs global mixing, 0..1). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Sharpen|Influence",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BlendWeight = 1.0f;

    /** Soft world-unit falloff outside the bounds when not unbound. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Sharpen|Influence",
        meta = (EditCondition = "!bUnbound", ClampMin = "0.0", Units = "cm"))
    float BlendRadius = 200.0f;

    /** Ordering when several smart-sharpen actors overlap (higher wins). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Sharpen|Influence")
    float Priority = 0.0f;

    /** The post-process sharpen material; defaults to /Game/Materials/PostProcess/M_GTC_SmartSharpen. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Sharpen|Advanced")
    TSoftObjectPtr<UMaterialInterface> SharpenMaterial;

    /** Rebuild the dynamic material instance and re-push all parameters. Safe to call anytime. */
    UFUNCTION(BlueprintCallable, Category = "Smart Sharpen")
    void ApplySettings();

protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    void EnsureMID();
    void PushParameters();
    void UpdateEditorIcon();

    UPROPERTY(VisibleAnywhere, Category = "Smart Sharpen")
    TObjectPtr<UPostProcessComponent> PostProcess;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> SharpenMID;

#if WITH_EDITORONLY_DATA
    UPROPERTY()
    TObjectPtr<UBillboardComponent> Icon;

    UPROPERTY()
    TObjectPtr<UTextRenderComponent> InfoText;
#endif
};
