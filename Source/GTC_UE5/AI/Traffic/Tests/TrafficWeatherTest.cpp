// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../TrafficWeather.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FTrafficWeather — weather caution for ambient traffic (the IDM
 * DesiredSpeed/TimeHeadway side of the "wetness -> traffic speed" TODO). GTC-original
 * (no Godot oracle). Pins clear-dry = no change, the wet/fog speed bleed and that
 * they compound, the MinSpeedFactor floor, the headway gain tracking the same
 * severity, the Adjusted* helpers, monotonicity, and input clamping. Prefix
 * GTC.AI.Traffic.Weather. All-static, so no shared helpers for the unity build.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTrafficWeatherTest,
    "GTC.AI.Traffic.Weather",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficWeatherTest::RunTest(const FString& Parameters)
{
    const FTrafficWeather::FParams P; // wet loss .25, fog loss .35, floor .55, max headway 1.8

    // ---- Clear, dry: traffic drives normally ----------------------------------
    {
        TestTrue(TEXT("clear-dry speed factor 1"), FMath::IsNearlyEqual(FTrafficWeather::SpeedFactor(0.0, 1.0, P), 1.0, Eps));
        TestTrue(TEXT("clear-dry headway factor 1"), FMath::IsNearlyEqual(FTrafficWeather::HeadwayFactor(0.0, 1.0, P), 1.0, Eps));
    }

    // ---- Rain slows and spaces traffic ----------------------------------------
    {
        TestTrue(TEXT("full wet (clear) speed 0.75"), FMath::IsNearlyEqual(FTrafficWeather::SpeedFactor(1.0, 1.0, P), 0.75, Eps));
        // severity 0.25/0.60 -> headway 1 + 0.8*0.41667 = 1.33333
        TestTrue(TEXT("full wet (clear) headway ~1.333"),
            FMath::IsNearlyEqual(FTrafficWeather::HeadwayFactor(1.0, 1.0, P), 1.0 + 0.8 * (0.25 / 0.60), Eps));
    }

    // ---- Fog slows and spaces traffic -----------------------------------------
    {
        TestTrue(TEXT("thick fog (dry) speed 0.65"), FMath::IsNearlyEqual(FTrafficWeather::SpeedFactor(0.0, 0.0, P), 0.65, Eps));
        TestTrue(TEXT("thick fog (dry) headway ~1.467"),
            FMath::IsNearlyEqual(FTrafficWeather::HeadwayFactor(0.0, 0.0, P), 1.0 + 0.8 * (0.35 / 0.60), Eps));
    }

    // ---- Rain and fog compound, but the speed floor holds ----------------------
    {
        // Raw reduction 0.60 -> 0.40, but floored at 0.55.
        TestTrue(TEXT("foggy downpour floors speed at 0.55"),
            FMath::IsNearlyEqual(FTrafficWeather::SpeedFactor(1.0, 0.0, P), 0.55, Eps));
        TestTrue(TEXT("worst conditions max the headway gain"),
            FMath::IsNearlyEqual(FTrafficWeather::HeadwayFactor(1.0, 0.0, P), 1.8, Eps));
    }

    // ---- The Adjusted* helpers apply the factors to base IDM params -----------
    {
        TestTrue(TEXT("adjusted cruise: 12 * 0.75 = 9"),
            FMath::IsNearlyEqual(FTrafficWeather::AdjustedDesiredSpeed(12.0, 1.0, 1.0, P), 9.0, Eps));
        TestTrue(TEXT("adjusted headway: 1.4 * 1.8 = 2.52"),
            FMath::IsNearlyEqual(FTrafficWeather::AdjustedTimeHeadway(1.4, 1.0, 0.0, P), 2.52, Eps));
    }

    // ---- Monotonic: worse weather -> slower + more spaced ----------------------
    {
        TestTrue(TEXT("wetter is slower"), FTrafficWeather::SpeedFactor(0.3, 1.0, P) > FTrafficWeather::SpeedFactor(0.8, 1.0, P));
        TestTrue(TEXT("foggier is slower"), FTrafficWeather::SpeedFactor(0.0, 0.8, P) > FTrafficWeather::SpeedFactor(0.0, 0.3, P));
        TestTrue(TEXT("wetter spaces more"), FTrafficWeather::HeadwayFactor(0.8, 1.0, P) > FTrafficWeather::HeadwayFactor(0.3, 1.0, P));
    }

    // ---- Input clamping -------------------------------------------------------
    {
        TestTrue(TEXT("over-range wetness clamps to full wet"),
            FMath::IsNearlyEqual(FTrafficWeather::SpeedFactor(2.0, 1.0, P), FTrafficWeather::SpeedFactor(1.0, 1.0, P), Eps));
        TestTrue(TEXT("over-range visibility clamps to clear"),
            FMath::IsNearlyEqual(FTrafficWeather::SpeedFactor(0.0, 2.0, P), 1.0, Eps));
        TestTrue(TEXT("negative visibility clamps to whiteout"),
            FMath::IsNearlyEqual(FTrafficWeather::SpeedFactor(0.0, -1.0, P), 0.65, Eps));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
