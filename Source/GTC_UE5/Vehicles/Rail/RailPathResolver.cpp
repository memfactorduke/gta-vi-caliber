// Copyright Epic Games, Inc. All Rights Reserved.

#include "RailPathResolver.h"

#include "Math/UnrealMathUtility.h"

double FRailPathResolver::LoopLength(const TArray<FVector>& Points)
{
    const int32 N = Points.Num();
    if (N < 2)
    {
        return 0.0;
    }

    double Total = 0.0;
    for (int32 i = 0; i < N; ++i)
    {
        // Segment i -> i+1, with the last one closing back to point 0 (it's a loop).
        const FVector& A = Points[i];
        const FVector& B = Points[(i + 1) % N];
        Total += (B - A).Size();
    }
    return Total;
}

FRailPathResolver::FSample FRailPathResolver::Sample(const TArray<FVector>& Points, double Distance)
{
    FSample Out;

    const int32 N = Points.Num();
    if (N == 0)
    {
        return Out; // bValid = false, Position = zero
    }
    if (N == 1)
    {
        Out.Position = Points[0];
        return Out; // single point: known position, but no travel direction -> invalid
    }

    const double Total = LoopLength(Points);
    if (Total <= 0.0)
    {
        Out.Position = Points[0]; // all points coincident
        return Out;
    }

    // Wrap the distance into [0, Total).
    double D = FMath::Fmod(Distance, Total);
    if (D < 0.0)
    {
        D += Total;
    }

    // Walk segments (including the closing segment) until D falls inside one.
    double Accum = 0.0;
    for (int32 i = 0; i < N; ++i)
    {
        const FVector& A = Points[i];
        const FVector& B = Points[(i + 1) % N];
        const FVector Seg = B - A;
        const double SegLen = Seg.Size();
        if (SegLen <= 0.0)
        {
            continue; // coincident consecutive points — skip
        }

        if (D <= Accum + SegLen)
        {
            const double T = (D - Accum) / SegLen;
            Out.Position = A + Seg * T;
            Out.Forward = Seg.GetSafeNormal();
            Out.bValid = true;
            return Out;
        }
        Accum += SegLen;
    }

    // Defensive fallback: the last non-zero segment always covers the wrapped distance, so this is
    // provably unreachable — clamp to the loop's start and keep the safe default Forward rather than
    // risk a zero tangent from a degenerate opening segment.
    Out.Position = Points[0];
    Out.bValid = true;
    return Out;
}
