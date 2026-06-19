// Copyright Epic Games, Inc. All Rights Reserved.

#include "LanePath.h"

#include "Math/UnrealMathUtility.h"

namespace
{
    // Matches FRoadNetwork::PointOnSegment's zero-length heading fallback.
    const FVector GtcLaneForward(0.0, 0.0, -1.0);

    // Planar (XZ) direction A->B, unit, with the forward fallback when degenerate.
    FVector PlanarDir(const FVector& A, const FVector& B)
    {
        const FVector D(B.X - A.X, 0.0, B.Z - A.Z);
        const FVector N = D.GetSafeNormal();
        return N == FVector::ZeroVector ? GtcLaneForward : N;
    }

    double PlanarDist(const FVector& A, const FVector& B)
    {
        const double Dx = B.X - A.X;
        const double Dz = B.Z - A.Z;
        return FMath::Sqrt(Dx * Dx + Dz * Dz);
    }
}

FVector FLanePath::RightOf(const FVector& Heading)
{
    const FVector H(Heading.X, 0.0, Heading.Z);
    const FVector N = H.GetSafeNormal();
    if (N == FVector::ZeroVector)
    {
        return FVector::ZeroVector;
    }
    // Right of travel in the XZ plane: rotate the heading -90° about +Y.
    // (1,0,0) -> (0,0,-1); (0,0,1) -> (1,0,0).
    return FVector(N.Z, 0.0, -N.X);
}

void FLanePath::BuildFromCenterline(const TArray<FVector>& Centerline, double LateralOffset)
{
    Points.Reset();
    Cumulative.Reset();

    const int32 N = Centerline.Num();
    if (N == 0)
    {
        return;
    }
    if (N == 1)
    {
        Points.Add(Centerline[0]);
        Cumulative.Add(0.0);
        return;
    }

    Points.Reserve(N);
    for (int32 I = 0; I < N; ++I)
    {
        // Average the right-perpendiculars of the adjacent segments at this vertex.
        FVector Perp(0.0, 0.0, 0.0);
        if (I > 0)
        {
            Perp = Perp + RightOf(PlanarDir(Centerline[I - 1], Centerline[I]));
        }
        if (I < N - 1)
        {
            Perp = Perp + RightOf(PlanarDir(Centerline[I], Centerline[I + 1]));
        }
        const FVector PerpN = Perp.GetSafeNormal();
        Points.Add(Centerline[I] + PerpN * LateralOffset);
    }

    // Cumulative arc length along the OFFSET polyline.
    Cumulative.Reserve(N);
    Cumulative.Add(0.0);
    for (int32 I = 1; I < N; ++I)
    {
        Cumulative.Add(Cumulative[I - 1] + PlanarDist(Points[I - 1], Points[I]));
    }
}

FLanePath::FPose FLanePath::PoseAtDistance(double S) const
{
    FPose Out;
    const int32 N = Points.Num();
    if (N == 0)
    {
        return Out; // zero pos, forward fallback
    }
    if (N == 1)
    {
        Out.Pos = Points[0];
        return Out;
    }

    const double L = Length();
    const double Sc = FMath::Clamp(S, 0.0, L);
    for (int32 I = 0; I < N - 1; ++I)
    {
        const bool bLast = (I == N - 2);
        if (Sc <= Cumulative[I + 1] || bLast)
        {
            const double SegLen = Cumulative[I + 1] - Cumulative[I];
            const double T = SegLen > Eps ? (Sc - Cumulative[I]) / SegLen : 0.0;
            Out.Pos = FMath::Lerp(Points[I], Points[I + 1], T);
            Out.Heading = PlanarDir(Points[I], Points[I + 1]);
            return Out;
        }
    }

    // Unreachable for N >= 2 (the bLast branch always returns), but stay defensive.
    Out.Pos = Points.Last();
    Out.Heading = PlanarDir(Points[N - 2], Points[N - 1]);
    return Out;
}

double FLanePath::DistanceToEnd(double S) const
{
    return FMath::Max(0.0, Length() - FMath::Clamp(S, 0.0, Length()));
}

double FLanePath::Advance(double S, double DeltaS, bool& bOutReachedEnd) const
{
    const double L = Length();
    const double Sc = FMath::Clamp(S + DeltaS, 0.0, L);
    bOutReachedEnd = DeltaS > 0.0 && Sc >= L - Eps;
    return Sc;
}
