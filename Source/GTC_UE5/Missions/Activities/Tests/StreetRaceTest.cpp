// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../StreetRace.h"
#include "../../../Tests/GtcTestTolerances.h"

// Each test below maps 1:1 to a test_* assertion in the the reference reference behavior
// game/tests/unit/test_street_race.gd.

using GtcTest::Eps;

namespace GtcStreetRaceTest
{
    // Mirrors the oracle's R := 5.0 and _square_track() helper.
    constexpr double SrR = 5.0;

    static TArray<FVector> SquareTrack()
    {
        return TArray<FVector>{
            FVector(0.0, 0.0, 0.0),
            FVector(100.0, 0.0, 0.0),
            FVector(100.0, 0.0, 100.0),
            FVector(0.0, 0.0, 100.0),
        };
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceStartsAtFirstTest,
    "GTC.Missions.Activities.StreetRace.StartsAtFirstCheckpoint",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceStartsAtFirstTest::RunTest(const FString& Parameters)
{
    StreetRace Race(GtcStreetRaceTest::SquareTrack(), 2);
    TestEqual(TEXT("index == 0"), Race.CheckpointIndex(), 0);
    TestEqual(TEXT("lap == 1"), Race.CurrentLap(), 1);
    TestEqual(TEXT("total == 2"), Race.TotalLaps(), 2);
    TestFalse(TEXT("not finished"), Race.IsFinished());
    TestEqual(TEXT("current cp"), Race.CurrentCheckpoint(), FVector(0.0, 0.0, 0.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceReachedWithinRadiusTest,
    "GTC.Missions.Activities.StreetRace.ReachedAdvancesWithinRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceReachedWithinRadiusTest::RunTest(const FString& Parameters)
{
    StreetRace Race(GtcStreetRaceTest::SquareTrack(), 1);
    const bool bHit = Race.Reached(FVector(2.0, 9.0, 1.0), GtcStreetRaceTest::SrR);
    TestTrue(TEXT("hit"), bHit);
    TestEqual(TEXT("index == 1"), Race.CheckpointIndex(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceReachedIgnoresYTest,
    "GTC.Missions.Activities.StreetRace.ReachedIgnoresYHeight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceReachedIgnoresYTest::RunTest(const FString& Parameters)
{
    // 9 units up but on the gate in XZ -> still counts.
    StreetRace Race(GtcStreetRaceTest::SquareTrack(), 1);
    TestTrue(TEXT("hit"), Race.Reached(FVector(0.0, 50.0, 0.0), GtcStreetRaceTest::SrR));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceNotOutsideRadiusTest,
    "GTC.Missions.Activities.StreetRace.ReachedNotOutsideRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceNotOutsideRadiusTest::RunTest(const FString& Parameters)
{
    StreetRace Race(GtcStreetRaceTest::SquareTrack(), 1);
    const bool bHit = Race.Reached(FVector(50.0, 0.0, 50.0), GtcStreetRaceTest::SrR);
    TestFalse(TEXT("no hit"), bHit);
    TestEqual(TEXT("index == 0"), Race.CheckpointIndex(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceWrapsLapTest,
    "GTC.Missions.Activities.StreetRace.ReachedWrapsIntoNextLap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceWrapsLapTest::RunTest(const FString& Parameters)
{
    StreetRace Race(GtcStreetRaceTest::SquareTrack(), 2);
    // Clear all 4 checkpoints of lap 1.
    Race.Reached(FVector(0.0, 0.0, 0.0), GtcStreetRaceTest::SrR);
    Race.Reached(FVector(100.0, 0.0, 0.0), GtcStreetRaceTest::SrR);
    Race.Reached(FVector(100.0, 0.0, 100.0), GtcStreetRaceTest::SrR);
    Race.Reached(FVector(0.0, 0.0, 100.0), GtcStreetRaceTest::SrR);
    TestEqual(TEXT("index == 0"), Race.CheckpointIndex(), 0);
    TestEqual(TEXT("lap == 2"), Race.CurrentLap(), 2);
    TestFalse(TEXT("not finished"), Race.IsFinished());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceFinishedLastTest,
    "GTC.Missions.Activities.StreetRace.FinishedAfterLastCheckpointLastLap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceFinishedLastTest::RunTest(const FString& Parameters)
{
    StreetRace Race(TArray<FVector>{ FVector(0.0, 0.0, 0.0), FVector(10.0, 0.0, 0.0) }, 1);
    Race.Reached(FVector(0.0, 0.0, 0.0), GtcStreetRaceTest::SrR);
    const bool bDone = Race.Reached(FVector(10.0, 0.0, 0.0), GtcStreetRaceTest::SrR);
    TestTrue(TEXT("done"), bDone);
    TestTrue(TEXT("finished"), Race.IsFinished());
    TestEqual(TEXT("lap == 1"), Race.CurrentLap(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceNoopAfterFinishTest,
    "GTC.Missions.Activities.StreetRace.ReachedNoopAfterFinish",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceNoopAfterFinishTest::RunTest(const FString& Parameters)
{
    StreetRace Race(TArray<FVector>{ FVector(0.0, 0.0, 0.0) }, 1);
    Race.Reached(FVector(0.0, 0.0, 0.0), GtcStreetRaceTest::SrR);
    TestTrue(TEXT("finished"), Race.IsFinished());
    TestFalse(TEXT("noop"), Race.Reached(FVector(0.0, 0.0, 0.0), GtcStreetRaceTest::SrR));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceEmptyStartsFinishedTest,
    "GTC.Missions.Activities.StreetRace.EmptyCheckpointsStartsFinished",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceEmptyStartsFinishedTest::RunTest(const FString& Parameters)
{
    StreetRace Race(TArray<FVector>{}, 3);
    TestTrue(TEXT("finished"), Race.IsFinished());
    TestFalse(TEXT("no reach"), Race.Reached(FVector::ZeroVector, GtcStreetRaceTest::SrR));
    TestEqual(TEXT("progress == 1.0"), Race.Progress(), 1.0, Eps);
    TestEqual(TEXT("remaining == 0"), Race.CheckpointsRemaining(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceProgressRampsTest,
    "GTC.Missions.Activities.StreetRace.ProgressRampsZeroToOne",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceProgressRampsTest::RunTest(const FString& Parameters)
{
    StreetRace Race(GtcStreetRaceTest::SquareTrack(), 2); // 8 gates total
    TestEqual(TEXT("start == 0.0"), Race.Progress(), 0.0, Eps);
    Race.Reached(FVector(0.0, 0.0, 0.0), GtcStreetRaceTest::SrR);
    Race.Reached(FVector(100.0, 0.0, 0.0), GtcStreetRaceTest::SrR); // 2 of 8 done
    TestEqual(TEXT("mid == 0.25"), Race.Progress(), 0.25, Eps);
    // Clear 4 more gates in ring order: rest of lap 1 (P2, P3) + first 2 of lap 2.
    Race.Reached(FVector(100.0, 0.0, 100.0), GtcStreetRaceTest::SrR); // P2
    Race.Reached(FVector(0.0, 0.0, 100.0), GtcStreetRaceTest::SrR);   // P3 -> wraps into lap 2
    Race.Reached(FVector(0.0, 0.0, 0.0), GtcStreetRaceTest::SrR);     // P0 of lap 2
    Race.Reached(FVector(100.0, 0.0, 0.0), GtcStreetRaceTest::SrR);   // P1 of lap 2
    // 6 of 8 done.
    TestEqual(TEXT("late == 0.75"), Race.Progress(), 0.75, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceProgressOneFinishedTest,
    "GTC.Missions.Activities.StreetRace.ProgressOneWhenFinished",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceProgressOneFinishedTest::RunTest(const FString& Parameters)
{
    StreetRace Race(TArray<FVector>{ FVector(0.0, 0.0, 0.0) }, 1);
    Race.Reached(FVector(0.0, 0.0, 0.0), GtcStreetRaceTest::SrR);
    TestEqual(TEXT("progress == 1.0"), Race.Progress(), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceRemainingAllLapsTest,
    "GTC.Missions.Activities.StreetRace.CheckpointsRemainingCountsAllLaps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceRemainingAllLapsTest::RunTest(const FString& Parameters)
{
    StreetRace Race(GtcStreetRaceTest::SquareTrack(), 3); // 12 total
    const int32 Start = Race.CheckpointsRemaining();
    Race.Reached(FVector(0.0, 0.0, 0.0), GtcStreetRaceTest::SrR);
    TestEqual(TEXT("start == 12"), Start, 12);
    TestEqual(TEXT("remaining == 11"), Race.CheckpointsRemaining(), 11);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRacePlacementOrdersTest,
    "GTC.Missions.Activities.StreetRace.PlacementOrdersByProgress",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRacePlacementOrdersTest::RunTest(const FString& Parameters)
{
    const int32 First = StreetRace::Placement(0.9, TArray<double>{ 0.5, 0.2 });
    const int32 Second = StreetRace::Placement(0.5, TArray<double>{ 0.9, 0.2 });
    const int32 Third = StreetRace::Placement(0.1, TArray<double>{ 0.9, 0.5 });
    TestEqual(TEXT("first == 1"), First, 1);
    TestEqual(TEXT("second == 2"), Second, 2);
    TestEqual(TEXT("third == 3"), Third, 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRacePlacementTiesTest,
    "GTC.Missions.Activities.StreetRace.PlacementTiesKeepPlayerAhead",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRacePlacementTiesTest::RunTest(const FString& Parameters)
{
    // Equal progress is not "strictly further along" -> player stays 1st.
    TestEqual(TEXT("place == 1"), StreetRace::Placement(0.5, TArray<double>{ 0.5, 0.5 }), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRacePlacementNoRivalsTest,
    "GTC.Missions.Activities.StreetRace.PlacementNoRivalsIsFirst",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRacePlacementNoRivalsTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("place == 1"), StreetRace::Placement(0.3, TArray<double>{}), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceGapBehindTest,
    "GTC.Missions.Activities.StreetRace.GapToDistanceBehind",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceGapBehindTest::RunTest(const FString& Parameters)
{
    // 0.2 of a 1000m loop behind -> 200m.
    TestEqual(TEXT("gap == 200"), StreetRace::GapTo(0.7, 0.5, 1000.0), 200.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceGapFloorsTest,
    "GTC.Missions.Activities.StreetRace.GapToFloorsAtZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceGapFloorsTest::RunTest(const FString& Parameters)
{
    // Already ahead -> no positive gap; bad track length -> 0.
    TestEqual(TEXT("ahead == 0"), StreetRace::GapTo(0.3, 0.5, 1000.0), 0.0, Eps);
    TestEqual(TEXT("bad len == 0"), StreetRace::GapTo(0.7, 0.5, 0.0), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceTimingTest,
    "GTC.Missions.Activities.StreetRace.TimingAccrues",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceTimingTest::RunTest(const FString& Parameters)
{
    StreetRace Race(GtcStreetRaceTest::SquareTrack(), 1);
    Race.Tick(1.5);
    Race.Tick(2.0);
    Race.Tick(-9.0); // ignored
    TestEqual(TEXT("elapsed == 3.5"), Race.Elapsed(), 3.5, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceLapSplitsTest,
    "GTC.Missions.Activities.StreetRace.LapSplitsAndBestLap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceLapSplitsTest::RunTest(const FString& Parameters)
{
    StreetRace Race(GtcStreetRaceTest::SquareTrack(), 2);
    // Lap 1: clock to 10s, then clear 4 gates.
    Race.Tick(10.0);
    for (const FVector& Cp : GtcStreetRaceTest::SquareTrack())
    {
        Race.Reached(Cp, GtcStreetRaceTest::SrR);
    }
    TestEqual(TEXT("lap1 == 10"), Race.LastLap(), 10.0, Eps);
    // Lap 2: 4 more seconds, then clear 4 gates -> faster lap.
    Race.Tick(4.0);
    for (const FVector& Cp : GtcStreetRaceTest::SquareTrack())
    {
        Race.Reached(Cp, GtcStreetRaceTest::SrR);
    }
    TestEqual(TEXT("lap2 == 4"), Race.LastLap(), 4.0, Eps);
    TestEqual(TEXT("best == 4"), Race.BestLap(), 4.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceResetTest,
    "GTC.Missions.Activities.StreetRace.ResetClearsState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceResetTest::RunTest(const FString& Parameters)
{
    StreetRace Race(GtcStreetRaceTest::SquareTrack(), 2);
    Race.Tick(5.0);
    Race.Reached(FVector(0.0, 0.0, 0.0), GtcStreetRaceTest::SrR);
    Race.Reset();
    TestEqual(TEXT("index == 0"), Race.CheckpointIndex(), 0);
    TestEqual(TEXT("lap == 1"), Race.CurrentLap(), 1);
    TestEqual(TEXT("elapsed == 0"), Race.Elapsed(), 0.0, Eps);
    TestEqual(TEXT("best == 0"), Race.BestLap(), 0.0, Eps);
    TestFalse(TEXT("not finished"), Race.IsFinished());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceRewardByPlacementTest,
    "GTC.Missions.Activities.StreetRace.RewardByPlacement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceRewardByPlacementTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("first == 1000"), StreetRace::Reward(1, 1000), 1000); // full
    TestEqual(TEXT("second == 750"), StreetRace::Reward(2, 1000), 750);  // 0.75
    TestEqual(TEXT("fourth == 250"), StreetRace::Reward(4, 1000), 250);  // 0.25
    TestEqual(TEXT("sixth == 250"), StreetRace::Reward(6, 1000), 250);   // floored 0.25
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceRewardDnfTest,
    "GTC.Missions.Activities.StreetRace.RewardDnfIsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceRewardDnfTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("place 0 == 0"), StreetRace::Reward(0, 1000), 0);
    TestEqual(TEXT("place -1 == 0"), StreetRace::Reward(-1, 1000), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStreetRaceRewardZeroBaseTest,
    "GTC.Missions.Activities.StreetRace.RewardZeroBase",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStreetRaceRewardZeroBaseTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("zero base == 0"), StreetRace::Reward(1, 0), 0);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
