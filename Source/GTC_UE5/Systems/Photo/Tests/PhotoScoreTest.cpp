// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../PhotoScore.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FPhotoScore — photo-mode composition scoring. GTC-original (no Godot
 * oracle). Pins the rule-of-thirds framing (thirds = 1, centre = 0.5, corner = 0,
 * clamped), the fill sweet-spot falloff, the level/tilt falloff, the interest ramp
 * to a cap, the weighted Appeal blend (perfect and mediocre shots), lighting
 * clamping, and that Appeal stays in [0,1]. Prefix GTC.Systems.Photo. The core is
 * all-static, so no shared helpers for the unity build to collide.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPhotoScoreTest,
    "GTC.Systems.Photo",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhotoScoreTest::RunTest(const FString& Parameters)
{
    using FShot = FPhotoScore::FShot;
    const FPhotoScore::FParams P; // ideal fill .35, tol .30, maxtilt 30, cap 4, weights .30/.25/.20/.15/.10

    // ---- Framing: rule of thirds ----------------------------------------------
    {
        TestTrue(TEXT("a thirds intersection scores 1"),
            FMath::IsNearlyEqual(FPhotoScore::FramingScore(FVector2D(1.0 / 3.0, 1.0 / 3.0)), 1.0, Eps));
        TestTrue(TEXT("dead centre scores exactly 0.5"),
            FMath::IsNearlyEqual(FPhotoScore::FramingScore(FVector2D(0.5, 0.5)), 0.5, Eps));
        TestTrue(TEXT("a corner scores 0"),
            FMath::Abs(FPhotoScore::FramingScore(FVector2D(0.0, 0.0))) <= Eps);
        // Out-of-range positions clamp into the frame (here, to the (1,1) corner -> 0).
        TestTrue(TEXT("out-of-range subject clamps"),
            FMath::Abs(FPhotoScore::FramingScore(FVector2D(2.0, 2.0))) <= Eps);
        TestTrue(TEXT("the other three thirds points also score 1"),
            FMath::IsNearlyEqual(FPhotoScore::FramingScore(FVector2D(2.0 / 3.0, 1.0 / 3.0)), 1.0, Eps));
    }

    // ---- Fill: sweet spot then falloff ----------------------------------------
    {
        TestTrue(TEXT("ideal fill scores 1"), FMath::IsNearlyEqual(FPhotoScore::FillScore(0.35, P), 1.0, Eps));
        TestTrue(TEXT("half a tolerance off scores 0.5"), FMath::IsNearlyEqual(FPhotoScore::FillScore(0.5, P), 0.5, Eps));
        TestTrue(TEXT("a tolerance off scores 0"), FMath::Abs(FPhotoScore::FillScore(0.65, P)) <= Eps);
        TestTrue(TEXT("a distant speck scores 0"), FMath::Abs(FPhotoScore::FillScore(0.0, P)) <= Eps);
    }

    // ---- Level: tilt falloff --------------------------------------------------
    {
        TestTrue(TEXT("level scores 1"), FMath::IsNearlyEqual(FPhotoScore::LevelScore(0.0, P), 1.0, Eps));
        TestTrue(TEXT("half max tilt scores 0.5"), FMath::IsNearlyEqual(FPhotoScore::LevelScore(15.0, P), 0.5, Eps));
        TestTrue(TEXT("max tilt scores 0"), FMath::Abs(FPhotoScore::LevelScore(30.0, P)) <= Eps);
        TestTrue(TEXT("beyond max tilt stays 0"), FMath::Abs(FPhotoScore::LevelScore(90.0, P)) <= Eps);
        TestTrue(TEXT("tilt sign doesn't matter"), FMath::IsNearlyEqual(FPhotoScore::LevelScore(-15.0, P), 0.5, Eps));
    }

    // ---- Interest: ramp to a cap ----------------------------------------------
    {
        TestTrue(TEXT("nothing of interest scores 0"), FMath::Abs(FPhotoScore::InterestScore(0, P)) <= Eps);
        TestTrue(TEXT("half the cap scores 0.5"), FMath::IsNearlyEqual(FPhotoScore::InterestScore(2, P), 0.5, Eps));
        TestTrue(TEXT("the cap scores 1"), FMath::IsNearlyEqual(FPhotoScore::InterestScore(4, P), 1.0, Eps));
        TestTrue(TEXT("beyond the cap stays 1"), FMath::IsNearlyEqual(FPhotoScore::InterestScore(9, P), 1.0, Eps));
    }

    // ---- Appeal: the weighted blend -------------------------------------------
    {
        FShot Perfect;
        Perfect.Subject = FVector2D(1.0 / 3.0, 1.0 / 3.0);
        Perfect.SubjectFill = 0.35;
        Perfect.TiltDegrees = 0.0;
        Perfect.PointsOfInterest = 4;
        Perfect.Lighting = 1.0;
        TestTrue(TEXT("a perfect shot scores 1"), FMath::IsNearlyEqual(FPhotoScore::Appeal(Perfect, P), 1.0, Eps));

        FShot Mediocre;
        Mediocre.Subject = FVector2D(0.5, 0.5); // 0.5
        Mediocre.SubjectFill = 0.05;            // 0
        Mediocre.TiltDegrees = 15.0;            // 0.5
        Mediocre.PointsOfInterest = 1;          // 0.25
        Mediocre.Lighting = 0.5;                // 0.5
        // .30*.5 + .25*0 + .20*.5 + .15*.25 + .10*.5 = .3375
        TestTrue(TEXT("a mediocre shot scores 0.3375"), FMath::IsNearlyEqual(FPhotoScore::Appeal(Mediocre, P), 0.3375, Eps));

        FShot Worst;
        Worst.Subject = FVector2D(0.0, 0.0); // 0
        Worst.SubjectFill = 1.0;             // 0 (way over)
        Worst.TiltDegrees = 90.0;            // 0
        Worst.PointsOfInterest = 0;          // 0
        Worst.Lighting = 0.0;                // 0
        TestTrue(TEXT("a worst-case shot scores 0"), FMath::Abs(FPhotoScore::Appeal(Worst, P)) <= Eps);
    }

    // ---- Lighting is clamped; Appeal stays in [0,1] ---------------------------
    {
        FShot S;
        S.Subject = FVector2D(1.0 / 3.0, 2.0 / 3.0);
        S.SubjectFill = 0.35;
        S.TiltDegrees = 0.0;
        S.PointsOfInterest = 9;   // clamps to 1
        S.Lighting = 5.0;         // clamps to 1
        const double A = FPhotoScore::Appeal(S, P);
        TestTrue(TEXT("over-bright lighting clamps -> perfect"), FMath::IsNearlyEqual(A, 1.0, Eps));
        TestTrue(TEXT("appeal never exceeds 1"), A <= 1.0);
        TestTrue(TEXT("appeal never below 0"), A >= 0.0);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
