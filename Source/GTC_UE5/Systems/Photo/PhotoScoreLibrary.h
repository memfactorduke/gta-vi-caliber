// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PhotoScoreLibrary.generated.h"

/** Blueprint mirror of FPhotoScore::FShot — one composed shot to score. */
USTRUCT(BlueprintType)
struct FGtcPhotoShot
{
    GENERATED_BODY()

    /** Subject's normalized screen position; (0.5,0.5) is centre. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Photo")
    FVector2D Subject = FVector2D(0.5, 0.5);

    /** Fraction of the frame the subject fills, 0..1. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Photo")
    float SubjectFill = 0.0f;

    /** Horizon roll magnitude in degrees (0 = level). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Photo")
    float TiltDegrees = 0.0f;

    /** Count of notable things in frame. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Photo")
    int32 PointsOfInterest = 0;

    /** Light-quality read, 0..1 (golden hour ~1). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Photo")
    float Lighting = 0.5f;
};

/**
 * UPhotoScoreLibrary — Blueprint surface for the static FPhotoScore composition
 * scorer. Appeal() returns 0..1, the same value a social post is scored on — so the
 * phone camera drops straight into the clout economy (Post(PhotoAppeal(shot))).
 * Tuning uses FPhotoScore::FParams defaults.
 */
UCLASS()
class GTC_UE5_API UPhotoScoreLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Overall shot appeal 0..1 (weighted blend of the sub-scores). */
    UFUNCTION(BlueprintPure, Category = "GTC|Photo")
    static float PhotoAppeal(const FGtcPhotoShot& Shot);

    /** Rule-of-thirds framing score 0..1 for a subject screen position. */
    UFUNCTION(BlueprintPure, Category = "GTC|Photo")
    static float PhotoFramingScore(FVector2D Subject);

    /** Subject-fill score 0..1 (peaks at the ideal fill). */
    UFUNCTION(BlueprintPure, Category = "GTC|Photo")
    static float PhotoFillScore(float SubjectFill);

    /** Level score 0..1 (1 when level, 0 at/over the max tilt). */
    UFUNCTION(BlueprintPure, Category = "GTC|Photo")
    static float PhotoLevelScore(float TiltDegrees);

    /** Interest score 0..1 (points of interest, capped). */
    UFUNCTION(BlueprintPure, Category = "GTC|Photo")
    static float PhotoInterestScore(int32 PointsOfInterest);
};
