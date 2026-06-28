// Copyright Epic Games, Inc. All Rights Reserved.

#include "PhotoScore.h"

double FPhotoScore::FramingScore(const FVector2D& Subject)
{
    const double X = FMath::Clamp(Subject.X, 0.0, 1.0);
    const double Y = FMath::Clamp(Subject.Y, 0.0, 1.0);

    // The four rule-of-thirds intersections.
    const double Thirds[2] = {1.0 / 3.0, 2.0 / 3.0};
    double NearestSq = TNumericLimits<double>::Max();
    for (const double TX : Thirds)
    {
        for (const double TY : Thirds)
        {
            const double DistSq = (X - TX) * (X - TX) + (Y - TY) * (Y - TY);
            NearestSq = FMath::Min(NearestSq, DistSq);
        }
    }

    // Normalize against the worst case: a frame corner, which is sqrt(2)/3 from its
    // nearest thirds intersection. Closer than that scores higher; a corner reads 0.
    const double MaxDist = FMath::Sqrt(2.0) / 3.0;
    const double Dist = FMath::Sqrt(NearestSq);
    return FMath::Clamp(1.0 - Dist / MaxDist, 0.0, 1.0);
}

double FPhotoScore::FillScore(double SubjectFill, const FParams& Params)
{
    const double Tolerance = FMath::Max(KINDA_SMALL_NUMBER, Params.FillTolerance);
    const double Off = FMath::Abs(SubjectFill - Params.IdealFill);
    return FMath::Clamp(1.0 - Off / Tolerance, 0.0, 1.0);
}

double FPhotoScore::LevelScore(double TiltDegrees, const FParams& Params)
{
    const double MaxTilt = FMath::Max(KINDA_SMALL_NUMBER, Params.MaxTilt);
    const double Tilt = FMath::Abs(TiltDegrees);
    return FMath::Clamp(1.0 - Tilt / MaxTilt, 0.0, 1.0);
}

double FPhotoScore::InterestScore(int32 PointsOfInterest, const FParams& Params)
{
    if (Params.InterestCap <= 0)
    {
        return PointsOfInterest > 0 ? 1.0 : 0.0;
    }
    const double Poi = static_cast<double>(FMath::Max(0, PointsOfInterest));
    return FMath::Clamp(Poi / static_cast<double>(Params.InterestCap), 0.0, 1.0);
}

double FPhotoScore::Appeal(const FShot& Shot, const FParams& Params)
{
    const double Framing = FramingScore(Shot.Subject);
    const double Fill = FillScore(Shot.SubjectFill, Params);
    const double Level = LevelScore(Shot.TiltDegrees, Params);
    const double Interest = InterestScore(Shot.PointsOfInterest, Params);
    const double Lighting = FMath::Clamp(Shot.Lighting, 0.0, 1.0);

    // Weights are clamped non-negative; if they all vanish, fall back to an even blend.
    const double WF = FMath::Max(0.0, Params.FramingWeight);
    const double WS = FMath::Max(0.0, Params.FillWeight);
    const double WL = FMath::Max(0.0, Params.LevelWeight);
    const double WI = FMath::Max(0.0, Params.InterestWeight);
    const double WG = FMath::Max(0.0, Params.LightingWeight);
    const double Sum = WF + WS + WL + WI + WG;

    if (Sum <= 0.0)
    {
        return FMath::Clamp((Framing + Fill + Level + Interest + Lighting) / 5.0, 0.0, 1.0);
    }

    const double Blend = (Framing * WF + Fill * WS + Level * WL + Interest * WI + Lighting * WG) / Sum;
    return FMath::Clamp(Blend, 0.0, 1.0);
}
