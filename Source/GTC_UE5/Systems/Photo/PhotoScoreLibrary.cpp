// Copyright Epic Games, Inc. All Rights Reserved.

#include "PhotoScoreLibrary.h"
#include "PhotoScore.h"

float UPhotoScoreLibrary::PhotoAppeal(const FGtcPhotoShot& Shot)
{
    FPhotoScore::FShot S;
    S.Subject = Shot.Subject;
    S.SubjectFill = static_cast<double>(Shot.SubjectFill);
    S.TiltDegrees = static_cast<double>(Shot.TiltDegrees);
    S.PointsOfInterest = Shot.PointsOfInterest;
    S.Lighting = static_cast<double>(Shot.Lighting);

    const FPhotoScore::FParams Params;
    return static_cast<float>(FPhotoScore::Appeal(S, Params));
}

float UPhotoScoreLibrary::PhotoFramingScore(FVector2D Subject)
{
    return static_cast<float>(FPhotoScore::FramingScore(Subject));
}

float UPhotoScoreLibrary::PhotoFillScore(float SubjectFill)
{
    const FPhotoScore::FParams Params;
    return static_cast<float>(FPhotoScore::FillScore(static_cast<double>(SubjectFill), Params));
}

float UPhotoScoreLibrary::PhotoLevelScore(float TiltDegrees)
{
    const FPhotoScore::FParams Params;
    return static_cast<float>(FPhotoScore::LevelScore(static_cast<double>(TiltDegrees), Params));
}

float UPhotoScoreLibrary::PhotoInterestScore(int32 PointsOfInterest)
{
    const FPhotoScore::FParams Params;
    return static_cast<float>(FPhotoScore::InterestScore(PointsOfInterest, Params));
}
