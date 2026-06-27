// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure "smart sharpen" math — the per-pixel sharpening policy, extracted as
 * static scalar/vector helpers so it can be unit-tested without a renderer.
 *
 * The actual sharpen runs in a post-process material (M_GTC_SmartSharpen): an
 * unsharp mask `out = center + Amount * (center - blur)` whose `Amount` is
 * modulated per-pixel by depth (distance control), luminance (shadow safety)
 * and a foliage mask (foliage control). Those modulation formulas are the
 * "smart" part of the effect, so they live here as a parity oracle: the HLSL
 * Custom node mirrors these functions exactly, and AGTCSmartSharpen feeds the
 * same constants into the material instance. Same testable-core pattern as
 * FPlayerMotion / FCameraFeel — numeric and pure by design, no scene access.
 *
 * Conventions: distances are centimetres (Unreal world units). "Amount" is the
 * unsharp-mask gain (0 = passthrough, 1 = a full edge added back). Luminance is
 * scene-referred linear (Rec.709). Weights returned are 0..1 multipliers.
 *
 * Coverage: Tests/SmartSharpenTest.cpp, prefix GTC.Camera.SmartSharpen.
 */
struct GTC_UE5_API FSmartSharpen
{
    /** Largest sharpen gain the effect will apply; clamps user/material input. */
    static constexpr float MaxAmount = 5.0f;

    /** Rec.709 relative luminance of a linear color — the edge/shadow signal. */
    static float LumaRec709(const FLinearColor& LinearColor);

    /** Clamp a sharpen gain into the safe [0, MaxAmount] range. */
    static float ClampAmount(float Amount);

    /**
     * Distance control: sharpen gain at a given scene depth, lerping from
     * NearAmount (at or nearer than NearCm) to FarAmount (at or beyond FarCm).
     * Lets distant geometry keep more (or less) sharpening than near surfaces.
     * A degenerate band (FarCm <= NearCm) collapses to NearAmount.
     */
    static float DistanceSharpen(float NearAmount, float FarAmount, float DistanceCm, float NearCm, float FarCm);

    /**
     * Shadow safety: luminance gate that fades sharpening out in near-black
     * regions so the unsharp mask doesn't amplify shadow grain / compression
     * noise. Smoothstep from 0 at Luma 0 to 1 at Luma == ShadowFloor; full
     * strength above the floor. A non-positive floor means "never gate" (1).
     */
    static float ShadowSafeWeight(float Luma, float ShadowFloor);

    /**
     * Foliage control: blend between the surface sharpen amount and a separate
     * foliage amount by a 0..1 foliage mask (1 = fully foliage). Foliage often
     * wants gentler sharpening to avoid shimmering thin leaves under TAA/TSR.
     */
    static float FoliageBlendedAmount(float SurfaceAmount, float FoliageAmount, float FoliageMask);

    /**
     * Local vs global: blend a global sharpen amount toward a local actor's
     * amount by that actor's influence weight (0 = global only, 1 = local
     * fully overrides). Stacked local actors compose via their post-process
     * blend weights; this models one local override over the global baseline.
     */
    static float CombineLocalGlobal(float GlobalAmount, float LocalAmount, float LocalWeight);

    /**
     * Resolution independence: UV step for sampling `PixelRadius` pixels away
     * from the center tap, given the viewport size in pixels. Because the step
     * is expressed in pixels-over-viewport, a 1px-wide sharpen kernel covers
     * exactly one pixel at any resolution. Degenerate viewport (<= 0 on either
     * axis) yields a zero step (kernel collapses to passthrough).
     */
    static FVector2D TexelStep(float ViewWidthPx, float ViewHeightPx, float PixelRadius);

    /**
     * Composed per-pixel gain the material applies: take the distance-faded
     * surface amount, blend in the foliage amount by the foliage mask, gate by
     * the shadow-safe luminance weight, and clamp. This is the single formula
     * the HLSL Custom node evaluates per pixel; exposed here so it is testable
     * end-to-end and the shader can be diffed against it.
     */
    static float EffectiveGain(
        float NearAmount,
        float FarAmount,
        float DistanceCm,
        float NearCm,
        float FarCm,
        float FoliageAmount,
        float FoliageMask,
        float Luma,
        float ShadowFloor);

    /**
     * Game-setting bridge: map a normalised UI "Sharpness" slider (0..1) to a
     * sharpen gain in [0, MaxUserAmount], then clamp into the effect's safe
     * range. 0 = off, 1 = MaxUserAmount. Lets a single options-menu slider drive
     * the global AGTCSmartSharpen without the UI knowing the effect's internal
     * units. Slider values outside 0..1 and negative spans clamp gracefully.
     */
    static float UserSharpnessToAmount(float Slider01, float MaxUserAmount);
};
