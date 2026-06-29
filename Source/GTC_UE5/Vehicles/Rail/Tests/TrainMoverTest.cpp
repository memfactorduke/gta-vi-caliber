// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../TrainMover.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FTrainMover — the on-rails station-to-station mover. GTC-original (no
 * Godot oracle). Pins the pull-away, the braking-distance speed cap never exceeding
 * line speed, stopping exactly at the platform, the dwell then departure to the next
 * station, the loop wrap, the no-stations cruise, and negative-Dt. Prefix
 * GTC.Vehicles.Rail.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTrainMoverTest,
    "GTC.Vehicles.Rail",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrainMoverTest::RunTest(const FString& Parameters)
{
    FTrainMover::FParams P;
    P.TrackLength = 1000.0;
    P.LineSpeed = 100.0;
    P.Accel = 20.0;
    P.Brake = 20.0;
    P.DwellSeconds = 3.0;
    TArray<double> Stations;
    Stations.Add(250.0);
    Stations.Add(750.0);

    // ---- Fresh train at the start of the line ---------------------------------
    {
        FTrainMover T;
        T.Configure(P, Stations);
        TestTrue(TEXT("starts at 0"), FMath::Abs(T.Position()) <= Eps);
        TestTrue(TEXT("starts stopped"), FMath::Abs(T.Speed()) <= Eps);
        TestFalse(TEXT("not dwelling"), T.IsDwelling());
        TestEqual(TEXT("heading for station 0"), T.TargetStation(), 0);
        TestTrue(TEXT("250 to the next stop"), FMath::IsNearlyEqual(T.DistanceToNextStop(), 250.0, Eps));
    }

    // ---- Pulls away, never exceeds line speed, stops exactly at the platform ---
    {
        FTrainMover T;
        T.Configure(P, Stations);
        T.Advance(0.5);
        TestTrue(TEXT("pulls away from the start"), T.Speed() > 0.0);

        double MaxSeen = 0.0;
        int32 Steps = 0;
        while (!T.IsDwelling() && Steps < 5000)
        {
            T.Advance(0.1);
            MaxSeen = FMath::Max(MaxSeen, T.Speed());
            ++Steps;
        }
        TestTrue(TEXT("arrived and dwelling"), T.IsDwelling());
        TestTrue(TEXT("stopped exactly at the platform (250)"), FMath::IsNearlyEqual(T.Position(), 250.0, Eps));
        TestTrue(TEXT("stopped (speed 0)"), FMath::Abs(T.Speed()) <= Eps);
        TestTrue(TEXT("never exceeded line speed"), MaxSeen <= P.LineSpeed + Eps);
    }

    // ---- Dwells, then departs to the next station -----------------------------
    {
        FTrainMover T;
        T.Configure(P, Stations);
        while (!T.IsDwelling())
        {
            T.Advance(0.1); // reach station 0
        }
        TestEqual(TEXT("dwelling at station 0"), T.TargetStation(), 0);
        T.Advance(2.0); // still within the 3s dwell
        TestTrue(TEXT("still dwelling mid-dwell"), T.IsDwelling());
        TestTrue(TEXT("still stopped"), FMath::Abs(T.Speed()) <= Eps);
        T.Advance(1.5); // past the dwell -> departs
        TestFalse(TEXT("departed after the dwell"), T.IsDwelling());
        TestEqual(TEXT("now heading for station 1"), T.TargetStation(), 1);

        // Run to the next platform.
        int32 Steps = 0;
        while (!T.IsDwelling() && Steps < 5000)
        {
            T.Advance(0.1);
            ++Steps;
        }
        TestTrue(TEXT("stops at station 1 (750)"), FMath::IsNearlyEqual(T.Position(), 750.0, Eps));
    }

    // ---- Loops: after the last station it wraps back to the first --------------
    {
        FTrainMover T;
        T.Configure(P, Stations);
        // Drive several station arrivals and confirm it cycles 250 -> 750 -> 250.
        auto RunToNextStop = [&]()
        {
            int32 Steps = 0;
            while (!T.IsDwelling() && Steps < 5000) { T.Advance(0.1); ++Steps; }
            const double Where = T.Position();
            // clear the dwell so the next call advances to the following stop
            int32 D = 0;
            while (T.IsDwelling() && D < 200) { T.Advance(0.1); ++D; }
            return Where;
        };
        TestTrue(TEXT("1st stop at 250"), FMath::IsNearlyEqual(RunToNextStop(), 250.0, Eps));
        TestTrue(TEXT("2nd stop at 750"), FMath::IsNearlyEqual(RunToNextStop(), 750.0, Eps));
        TestTrue(TEXT("3rd stop wraps to 250"), FMath::IsNearlyEqual(RunToNextStop(), 250.0, Eps));
    }

    // ---- No stations: just cruise the loop ------------------------------------
    {
        FTrainMover T;
        T.Configure(P, TArray<double>());
        TestEqual(TEXT("no target station"), T.TargetStation(), INDEX_NONE);
        for (int32 I = 0; I < 200; ++I)
        {
            T.Advance(0.1);
        }
        TestFalse(TEXT("never dwells with no stations"), T.IsDwelling());
        TestTrue(TEXT("cruises at line speed"), FMath::IsNearlyEqual(T.Speed(), P.LineSpeed, Eps));
        TestTrue(TEXT("position stays on the loop"), T.Position() >= 0.0 && T.Position() < P.TrackLength);
    }

    // ---- Negative Dt is a no-op -----------------------------------------------
    {
        FTrainMover T;
        T.Configure(P, Stations);
        T.Advance(0.5);
        const double Pos = T.Position();
        const double Spd = T.Speed();
        T.Advance(-2.0);
        TestTrue(TEXT("negative Dt freezes position"), FMath::IsNearlyEqual(T.Position(), Pos, Eps));
        TestTrue(TEXT("negative Dt freezes speed"), FMath::IsNearlyEqual(T.Speed(), Spd, Eps));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
