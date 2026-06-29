// Copyright Epic Games, Inc. All Rights Reserved.

#include "WatercraftFloat.h"

#include "Math/UnrealMathUtility.h"

FWatercraftFloat::FPose FWatercraftFloat::ResolvePose(
    const double* SurfaceHeightsCm, const FHullSample* Samples, int32 Count, const FParams& Params)
{
    FPose Pose;
    const double Draft = FMath::Max(0.0, Params.DraftCm);

    if (Count <= 0 || SurfaceHeightsCm == nullptr || Samples == nullptr)
    {
        Pose.Zcm = -Draft; // sea level (0) minus draft
        return Pose;
    }

    // Accumulate the plane-fit sums and the height spread (for roughness) in one pass.
    double Sx = 0, Sy = 0, Sz = 0, Sxx = 0, Syy = 0, Sxy = 0, Sxz = 0, Syz = 0;
    double MinH = SurfaceHeightsCm[0];
    double MaxH = SurfaceHeightsCm[0];
    for (int32 I = 0; I < Count; ++I)
    {
        const double X = Samples[I].Xcm;
        const double Y = Samples[I].Ycm;
        const double Z = SurfaceHeightsCm[I];
        Sx += X; Sy += Y; Sz += Z;
        Sxx += X * X; Syy += Y * Y; Sxy += X * Y;
        Sxz += X * Z; Syz += Y * Z;
        MinH = FMath::Min(MinH, Z);
        MaxH = FMath::Max(MaxH, Z);
    }
    const double N = static_cast<double>(Count);
    const double MeanH = Sz / N;

    // Fit z = a*x + b*y + c by least squares (3x3 normal equations, Cramer's rule).
    const double D =
        Sxx * (Syy * N - Sy * Sy) - Sxy * (Sxy * N - Sy * Sx) + Sx * (Sxy * Sy - Syy * Sx);

    double A = 0.0, B = 0.0, C = MeanH; // level fallback (degenerate samples)
    if (FMath::Abs(D) > 1e-6)
    {
        const double Da =
            Sxz * (Syy * N - Sy * Sy) - Sxy * (Syz * N - Sy * Sz) + Sx * (Syz * Sy - Syy * Sz);
        const double Db =
            Sxx * (Syz * N - Sy * Sz) - Sxz * (Sxy * N - Sy * Sx) + Sx * (Sxy * Sz - Syz * Sx);
        const double Dc =
            Sxx * (Syy * Sz - Syz * Sy) - Sxy * (Sxy * Sz - Syz * Sx) + Sxz * (Sxy * Sy - Syy * Sx);
        A = Da / D;
        B = Db / D;
        C = Dc / D;
    }

    // a = dHeight/dX (forward slope) -> nose up when the swell rises toward the bow.
    // b = dHeight/dY (right slope)   -> starboard up when the swell rises to the right.
    Pose.PitchRad = FMath::Atan(A);
    Pose.RollRad = FMath::Atan(B);
    Pose.Zcm = C - Draft; // float at draft below the surface under the centre

    const double RoughRef = FMath::Max(KINDA_SMALL_NUMBER, Params.RoughnessRefCm);
    Pose.Roughness01 = FMath::Clamp((MaxH - MinH) / RoughRef, 0.0, 1.0);

    const double Capsize = FMath::Abs(Params.CapsizeAngleRad);
    Pose.bCapsized = FMath::Max(FMath::Abs(Pose.PitchRad), FMath::Abs(Pose.RollRad)) > Capsize;

    return Pose;
}
