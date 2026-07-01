// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../RailPathResolver.h"
#include "../TrainMover.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FRailPathResolver — mapping a distance-along-the-line to a world transform on the rail
 * polyline, plus the train's station-to-station envelope through the real FTrainMover core.
 * Prefix GTC.Vehicles.RailPath.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRailPathResolverTest,
    "GTC.Vehicles.RailPath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRailPathResolverTest::RunTest(const FString& Parameters)
{
    // A 1000-cm square loop: perimeter 4000.
    TArray<FVector> Square;
    Square.Add(FVector(0, 0, 0));
    Square.Add(FVector(1000, 0, 0));
    Square.Add(FVector(1000, 1000, 0));
    Square.Add(FVector(0, 1000, 0));

    // --- Rail path sampling. ---
    TestTrue(TEXT("loop length sums every segment + closing"),
        FMath::IsNearlyEqual(FRailPathResolver::LoopLength(Square), 4000.0, Eps));
    {
        const FRailPathResolver::FSample S = FRailPathResolver::Sample(Square, 500.0);
        TestTrue(TEXT("halfway along the first side"), S.Position.X == 500.0 && S.Position.Y == 0.0);
        TestTrue(TEXT("forward along +X"), FMath::IsNearlyEqual(S.Forward.X, 1.0, Eps));
    }
    {
        const FRailPathResolver::FSample S = FRailPathResolver::Sample(Square, 1500.0);
        TestTrue(TEXT("into the second side"), S.Position.X == 1000.0 && S.Position.Y == 500.0);
        TestTrue(TEXT("forward turns to +Y"), FMath::IsNearlyEqual(S.Forward.Y, 1.0, Eps));
    }
    {
        const FRailPathResolver::FSample S = FRailPathResolver::Sample(Square, 4500.0);
        TestTrue(TEXT("distance wraps past the loop"), S.Position.X == 500.0 && S.Position.Y == 0.0);
    }
    {
        const FRailPathResolver::FSample S = FRailPathResolver::Sample(Square, -500.0);
        TestTrue(TEXT("negative distance wraps backward"), S.Position.X == 0.0 && S.Position.Y == 500.0);
    }
    {
        TArray<FVector> One;
        One.Add(FVector(7, 7, 7));
        TestFalse(TEXT("single point -> invalid"), FRailPathResolver::Sample(One, 100.0).bValid);
    }

    // --- Train controller envelope. ---
    FTrainMover::FParams P;
    P.TrackLength = 4000.0;
    P.LineSpeed = 2000.0;
    P.Accel = 300.0;
    P.Brake = 400.0;
    P.DwellSeconds = 5.0;
    {
        FTrainMover T;
        TArray<double> Stops;
        Stops.Add(1000.0);
        Stops.Add(3000.0);
        T.Configure(P, Stops);
        int32 guard = 0;
        while (!T.IsDwelling() && guard++ < 5000)
        {
            T.Advance(0.05);
        }
        TestTrue(TEXT("reaches the first station and dwells"), T.IsDwelling());
        TestTrue(TEXT("stops exactly at the platform"), FMath::IsNearlyEqual(T.Position(), 1000.0, 1.0));
        TestTrue(TEXT("stopped -> zero speed"), T.Speed() == 0.0);

        for (int32 i = 0; i < 60; ++i)
        {
            T.Advance(0.1);
        }
        TestFalse(TEXT("the dwell ends and it departs"), T.IsDwelling());
        TestTrue(TEXT("now heading for the second station"), T.TargetStation() == 1);
        TestTrue(TEXT("pulling away -> moving"), T.Speed() > 0.0);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
