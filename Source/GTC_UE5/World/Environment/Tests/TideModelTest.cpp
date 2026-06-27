// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../TideModel.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FTideModel — semidiurnal tides with a spring-neap envelope. GTC-original
 * (no Godot oracle). Pins the high/low/mean levels at the cardinal phase points,
 * phase shifting the high-tide time, flood/ebb direction, TimeToNextHigh/Low across
 * the cycle, the spring-neap envelope swelling/shrinking the range, the normalized
 * band, and the degenerate no-period / cancelled-envelope cases. Prefix GTC.World.Tide.
 *
 * Assertions are anchored to tide physics (high at phase 0, low at half a period,
 * mean at a quarter period), not to the implementation. Helpers are in a NAMED
 * namespace so the unity build can't collide them with another test's anon helper.
 */
namespace TideModelTest
{
    using FParams = FTideModel::FParams;

    // A clean 12-hour tide, 80 cm amplitude, no spring-neap swing, high at hour 0.
    FParams Clean()
    {
        FParams P;
        P.MeanLevel = 0.0;
        P.MeanAmplitude = 80.0;
        P.SpringNeapRange = 0.0;
        P.PeriodHours = 12.0;
        P.SpringNeapHours = 300.0;
        P.PhaseHours = 0.0;
        P.SpringNeapPhaseHours = 0.0;
        return P;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTideModelTest,
    "GTC.World.Tide",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTideModelTest::RunTest(const FString& Parameters)
{
    using namespace TideModelTest;

    // ---- Cardinal levels: high at 0, low at half-period, mean at quarter --------
    {
        FTideModel T;
        T.Configure(Clean()); // period 12, amp 80, high at hour 0
        TestTrue(TEXT("high tide at hour 0"), FMath::IsNearlyEqual(T.LevelAt(0.0), 80.0, Eps));
        TestTrue(TEXT("low tide at hour 6"), FMath::IsNearlyEqual(T.LevelAt(6.0), -80.0, Eps));
        TestTrue(TEXT("mean at hour 3"), FMath::Abs(T.LevelAt(3.0)) <= Eps);
        TestTrue(TEXT("high again a full period later"), FMath::IsNearlyEqual(T.LevelAt(12.0), 80.0, Eps));
        TestTrue(TEXT("flat envelope is the mean amplitude"), FMath::IsNearlyEqual(T.AmplitudeAt(5.0), 80.0, Eps));
    }

    // ---- Normalized band: 1 at high, 0 at low, 0.5 at mean --------------------
    {
        FTideModel T;
        T.Configure(Clean());
        TestTrue(TEXT("normalized 1 at high"), FMath::IsNearlyEqual(T.NormalizedAt(0.0), 1.0, Eps));
        TestTrue(TEXT("normalized 0 at low"), FMath::Abs(T.NormalizedAt(6.0)) <= Eps);
        TestTrue(TEXT("normalized 0.5 at mean"), FMath::IsNearlyEqual(T.NormalizedAt(3.0), 0.5, Eps));
    }

    // ---- Flood vs ebb ---------------------------------------------------------
    {
        FTideModel T;
        T.Configure(Clean());
        TestTrue(TEXT("rising from low(6) toward high(12) at hour 9"), T.IsRisingAt(9.0));
        TestFalse(TEXT("falling from high(0) toward low(6) at hour 3"), T.IsRisingAt(3.0));
    }

    // ---- Time to the next high / low across the cycle -------------------------
    {
        FTideModel T;
        T.Configure(Clean());
        TestTrue(TEXT("at a high, next high is a full period"), FMath::IsNearlyEqual(T.TimeToNextHigh(0.0), 12.0, Eps));
        TestTrue(TEXT("3h after high, next high in 9h"), FMath::IsNearlyEqual(T.TimeToNextHigh(3.0), 9.0, Eps));
        TestTrue(TEXT("at a high, next low in half a period"), FMath::IsNearlyEqual(T.TimeToNextLow(0.0), 6.0, Eps));
        TestTrue(TEXT("3h after high, next low in 3h"), FMath::IsNearlyEqual(T.TimeToNextLow(3.0), 3.0, Eps));
        TestTrue(TEXT("at a low, next low is a full period"), FMath::IsNearlyEqual(T.TimeToNextLow(6.0), 12.0, Eps));
    }

    // ---- PhaseHours moves the high-tide time ---------------------------------
    {
        FTideModel T;
        FParams P = Clean();
        P.PhaseHours = 4.0; // high tide at hour 4
        T.Configure(P);
        TestTrue(TEXT("high tide now at hour 4"), FMath::IsNearlyEqual(T.LevelAt(4.0), 80.0, Eps));
        TestTrue(TEXT("low tide at hour 10"), FMath::IsNearlyEqual(T.LevelAt(10.0), -80.0, Eps));
        TestTrue(TEXT("next high from the high is a full period"), FMath::IsNearlyEqual(T.TimeToNextHigh(4.0), 12.0, Eps));
    }

    // ---- Spring-neap envelope swells and shrinks the range --------------------
    {
        FTideModel T;
        FParams P = Clean();
        P.SpringNeapRange = 30.0;
        P.SpringNeapHours = 300.0; // neap is half of this from a spring
        T.Configure(P);
        TestTrue(TEXT("spring amplitude at hour 0 is 110"), FMath::IsNearlyEqual(T.AmplitudeAt(0.0), 110.0, Eps));
        TestTrue(TEXT("neap amplitude at hour 150 is 50"), FMath::IsNearlyEqual(T.AmplitudeAt(150.0), 50.0, Eps));
        TestTrue(TEXT("spring high reaches 110"), FMath::IsNearlyEqual(T.LevelAt(0.0), 110.0, Eps));
        // hour 150 is a semidiurnal low (150/12 = 12.5 periods) under a 50cm neap envelope.
        TestTrue(TEXT("neap low only reaches -50"), FMath::IsNearlyEqual(T.LevelAt(150.0), -50.0, Eps));
        // Normalized stays full-scale regardless of envelope size.
        TestTrue(TEXT("normalized still 0 at the neap low"), FMath::Abs(T.NormalizedAt(150.0)) <= Eps);
    }

    // ---- A fully-cancelling envelope flattens to the mean safely --------------
    {
        FTideModel T;
        FParams P = Clean();
        P.MeanAmplitude = 80.0;
        P.SpringNeapRange = 80.0; // at neap the amplitude cancels to 0
        T.Configure(P);
        TestTrue(TEXT("envelope cancels to 0 at the neap"), FMath::Abs(T.AmplitudeAt(150.0)) <= Eps);
        TestTrue(TEXT("level is flat at the mean when cancelled"), FMath::Abs(T.LevelAt(150.0)) <= Eps);
        TestTrue(TEXT("normalized falls back to 0.5 when cancelled"), FMath::IsNearlyEqual(T.NormalizedAt(150.0), 0.5, Eps));
    }

    // ---- No period: flat at the datum, nothing crashes -----------------------
    {
        FTideModel T;
        FParams P = Clean();
        P.MeanLevel = 5.0;
        P.PeriodHours = 0.0;
        T.Configure(P);
        TestTrue(TEXT("no-period level is the datum"), FMath::IsNearlyEqual(T.LevelAt(7.0), 5.0, Eps));
        TestFalse(TEXT("no-period never rises"), T.IsRisingAt(7.0));
        TestTrue(TEXT("no-period next high is 0"), FMath::Abs(T.TimeToNextHigh(7.0)) <= Eps);
        TestTrue(TEXT("no-period next low is 0"), FMath::Abs(T.TimeToNextLow(7.0)) <= Eps);
        TestTrue(TEXT("no-period normalized is 0.5"), FMath::IsNearlyEqual(T.NormalizedAt(7.0), 0.5, Eps));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
