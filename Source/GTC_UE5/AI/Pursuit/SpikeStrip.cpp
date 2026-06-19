// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpikeStrip.h"

#include "Math/UnrealMathUtility.h"

namespace
{
    // Signed area / orientation of the triangle (A, B, C) on the X/Y ground plane.
    // >0 counter-clockwise, <0 clockwise, ~0 collinear.
    double Orient(const FVector& A, const FVector& B, const FVector& C)
    {
        return (B.X - A.X) * (C.Y - A.Y) - (B.Y - A.Y) * (C.X - A.X);
    }
}

bool FSpikeStrip::Crosses(const FVector& PrevPos, const FVector& CurrPos,
    const FVector& StripA, const FVector& StripB)
{
    // Reject degenerate (zero-length) segments — nothing to cross.
    if ((CurrPos - PrevPos).IsNearlyZero() || (StripB - StripA).IsNearlyZero())
    {
        return false;
    }

    const double D1 = Orient(StripA, StripB, PrevPos);
    const double D2 = Orient(StripA, StripB, CurrPos);
    const double D3 = Orient(PrevPos, CurrPos, StripA);
    const double D4 = Orient(PrevPos, CurrPos, StripB);

    // Proper intersection: each segment's endpoints straddle the other's line.
    const bool bStraddleStrip = ((D1 > 0.0 && D2 < 0.0) || (D1 < 0.0 && D2 > 0.0));
    const bool bStraddlePath = ((D3 > 0.0 && D4 < 0.0) || (D3 < 0.0 && D4 > 0.0));
    return bStraddleStrip && bStraddlePath;
}

void FSpikeStrip::Endpoints(const FVector& Center, const FVector& RoadRight, double Width,
    FVector& OutA, FVector& OutB)
{
    const FVector Right = RoadRight.GetSafeNormal();
    if (Right.IsNearlyZero())
    {
        OutA = Center;
        OutB = Center;
        return;
    }
    const double Half = 0.5 * FMath::Max(0.0, Width);
    OutA = Center - Right * Half;
    OutB = Center + Right * Half;
}
