// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../WatercraftFloat.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;
using FHullSample = FWatercraftFloat::FHullSample;

/**
 * Tests for FWatercraftFloat — the kinematic wave-riding pose (plane fit over hull-point
 * ocean heights). GTC-original. Pins flat-water level float at draft, bow/starboard swell
 * tilt, capsize on a steep face, roughness, and the no-sample / degenerate fallbacks.
 * Mirrors Scripts/gtc_watercraft/watercraft_float_verify.cpp. Prefix
 * GTC.Vehicles.Watercraft.Float.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWatercraftFloatTest,
    "GTC.Vehicles.Watercraft.Float",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWatercraftFloatTest::RunTest(const FString& Parameters)
{
    const FWatercraftFloat::FParams P; // draft 40, capsize 1.2, roughRef 200
    const FHullSample Hull[4] = { {100, 100}, {100, -100}, {-100, 100}, {-100, -100} };

    // ---- Flat water -----------------------------------------------------------
    {
        const double H[4] = { 50, 50, 50, 50 };
        const FWatercraftFloat::FPose Pose = FWatercraftFloat::ResolvePose(H, Hull, 4, P);
        TestTrue(TEXT("flat -> level"), FMath::Abs(Pose.PitchRad) <= Eps && FMath::Abs(Pose.RollRad) <= Eps);
        TestEqual(TEXT("floats at surface - draft"), Pose.Zcm, 10.0, Eps);
        TestTrue(TEXT("flat -> calm"), Pose.Roughness01 <= Eps);
        TestFalse(TEXT("flat -> not capsized"), Pose.bCapsized);
    }

    // ---- Bow swell -> nose up -------------------------------------------------
    {
        const double H[4] = { 60, 60, 40, 40 };
        const FWatercraftFloat::FPose Pose = FWatercraftFloat::ResolvePose(H, Hull, 4, P);
        TestEqual(TEXT("bow swell -> nose up"), Pose.PitchRad, FMath::Atan(0.1), Eps);
        TestTrue(TEXT("no roll"), FMath::Abs(Pose.RollRad) <= Eps);
        TestEqual(TEXT("roughness = spread/ref"), Pose.Roughness01, 0.1, Eps);
    }

    // ---- Starboard swell -> roll right ----------------------------------------
    {
        const double H[4] = { 60, 40, 60, 40 };
        const FWatercraftFloat::FPose Pose = FWatercraftFloat::ResolvePose(H, Hull, 4, P);
        TestEqual(TEXT("starboard swell -> roll right"), Pose.RollRad, FMath::Atan(0.1), Eps);
        TestTrue(TEXT("no pitch"), FMath::Abs(Pose.PitchRad) <= Eps);
    }

    // ---- Steep face -> capsize ------------------------------------------------
    {
        const double H[4] = { 410, 410, -310, -310 };
        const FWatercraftFloat::FPose Pose = FWatercraftFloat::ResolvePose(H, Hull, 4, P);
        TestEqual(TEXT("steep -> big pitch"), Pose.PitchRad, FMath::Atan(3.6), Eps);
        TestTrue(TEXT("past 1.2 rad -> capsized"), Pose.bCapsized);
        TestEqual(TEXT("huge spread clamps roughness"), Pose.Roughness01, 1.0, Eps);
    }

    // ---- Fallbacks ------------------------------------------------------------
    {
        const FWatercraftFloat::FPose None = FWatercraftFloat::ResolvePose(nullptr, nullptr, 0, P);
        TestEqual(TEXT("no samples -> sea level - draft"), None.Zcm, -40.0, Eps);

        const FHullSample One[1] = { {10, 20} };
        const double H1[1] = { 70 };
        const FWatercraftFloat::FPose Single = FWatercraftFloat::ResolvePose(H1, One, 1, P);
        TestTrue(TEXT("single sample -> level"), FMath::Abs(Single.PitchRad) <= Eps && FMath::Abs(Single.RollRad) <= Eps);
        TestEqual(TEXT("single sample -> height - draft"), Single.Zcm, 30.0, Eps);

        const FHullSample Line[3] = { {-100, 0}, {0, 0}, {100, 0} };
        const double H3[3] = { 40, 50, 60 };
        const FWatercraftFloat::FPose Coll = FWatercraftFloat::ResolvePose(H3, Line, 3, P);
        TestTrue(TEXT("collinear -> level fallback"), FMath::Abs(Coll.PitchRad) <= Eps && FMath::Abs(Coll.RollRad) <= Eps);
        TestEqual(TEXT("collinear -> mean - draft"), Coll.Zcm, 10.0, Eps);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
