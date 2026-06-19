// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCMinimapProjection.h"

#include "Math/UnrealMathUtility.h"

FVector2D FGTCMinimapProjection::WorldToLocal(const FVector& World) const
{
    const double Dx = World.X - Center.X;
    const double Dy = World.Y - Center.Y;
    const double C = FMath::Cos(HeadingRadians);
    const double S = FMath::Sin(HeadingRadians);

    // Rotate the world delta by -Heading so the player's forward maps to +Y (up) and
    // the player's right to +X. forward = (C, S), right = (-S, C):
    //   Forward = d . forward,  Right = d . right.
    const double Right = Dy * C - Dx * S;
    const double Forward = Dx * C + Dy * S;
    return FVector2D(Right, Forward);
}

FVector2D FGTCMinimapProjection::WorldToNormalized(const FVector& World) const
{
    if (RangeCm <= 0.0)
    {
        return FVector2D::ZeroVector;
    }
    return WorldToLocal(World) / RangeCm;
}

FVector2D FGTCMinimapProjection::Project(const FVector& World, bool& bOnEdge) const
{
    return ClampToDisc(WorldToNormalized(World), bOnEdge);
}

FVector2D FGTCMinimapProjection::ClampToDisc(const FVector2D& Normalized, bool& bClamped)
{
    const double LenSq = Normalized.X * Normalized.X + Normalized.Y * Normalized.Y;
    if (LenSq <= 1.0)
    {
        bClamped = false;
        return Normalized;
    }
    bClamped = true;
    const double Len = FMath::Sqrt(LenSq);
    return Normalized / Len;
}
