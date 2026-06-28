// Copyright Epic Games, Inc. All Rights Reserved.

#include "SmartSharpen.h"

#include "Math/UnrealMathUtility.h"

float FSmartSharpen::LumaRec709(const FLinearColor& LinearColor)
{
    // Rec.709 / sRGB primaries luma weights — the same coefficients the engine
    // tonemapper uses, so the gate matches perceived brightness.
    return 0.2126f * LinearColor.R + 0.7152f * LinearColor.G + 0.0722f * LinearColor.B;
}

float FSmartSharpen::ClampAmount(float Amount)
{
    return FMath::Clamp(Amount, 0.0f, MaxAmount);
}

float FSmartSharpen::DistanceSharpen(float NearAmount, float FarAmount, float DistanceCm, float NearCm, float FarCm)
{
    if (FarCm <= NearCm)
    {
        // Degenerate / disabled distance band: distance control is off, use Near.
        return NearAmount;
    }
    const float T = FMath::Clamp((DistanceCm - NearCm) / (FarCm - NearCm), 0.0f, 1.0f);
    return FMath::Lerp(NearAmount, FarAmount, T);
}

float FSmartSharpen::ShadowSafeWeight(float Luma, float ShadowFloor)
{
    if (ShadowFloor <= 0.0f)
    {
        // No shadow gate requested — sharpen everywhere.
        return 1.0f;
    }
    // SmoothStep already clamps the 0..1 result; below the floor we ramp toward
    // 0 so the unsharp mask can't lift noise out of crushed blacks.
    return FMath::SmoothStep(0.0f, ShadowFloor, Luma);
}

float FSmartSharpen::FoliageBlendedAmount(float SurfaceAmount, float FoliageAmount, float FoliageMask)
{
    return FMath::Lerp(SurfaceAmount, FoliageAmount, FMath::Clamp(FoliageMask, 0.0f, 1.0f));
}

float FSmartSharpen::CombineLocalGlobal(float GlobalAmount, float LocalAmount, float LocalWeight)
{
    return FMath::Lerp(GlobalAmount, LocalAmount, FMath::Clamp(LocalWeight, 0.0f, 1.0f));
}

FVector2D FSmartSharpen::TexelStep(float ViewWidthPx, float ViewHeightPx, float PixelRadius)
{
    if (ViewWidthPx <= 0.0f || ViewHeightPx <= 0.0f)
    {
        return FVector2D::ZeroVector;
    }
    return FVector2D(PixelRadius / ViewWidthPx, PixelRadius / ViewHeightPx);
}

float FSmartSharpen::EffectiveGain(
    float NearAmount,
    float FarAmount,
    float DistanceCm,
    float NearCm,
    float FarCm,
    float FoliageAmount,
    float FoliageMask,
    float Luma,
    float ShadowFloor)
{
    const float DistanceAmount = DistanceSharpen(NearAmount, FarAmount, DistanceCm, NearCm, FarCm);
    const float WithFoliage = FoliageBlendedAmount(DistanceAmount, FoliageAmount, FoliageMask);
    const float Gated = WithFoliage * ShadowSafeWeight(Luma, ShadowFloor);
    return ClampAmount(Gated);
}

float FSmartSharpen::UserSharpnessToAmount(float Slider01, float MaxUserAmount)
{
    const float Normalised = FMath::Clamp(Slider01, 0.0f, 1.0f);
    const float Span = FMath::Max(0.0f, MaxUserAmount);
    return ClampAmount(Normalised * Span);
}
