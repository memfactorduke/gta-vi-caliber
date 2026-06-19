// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../GtcActivityCompiler.h"
#include "../GtcMissionDefinition.h"
#include "../../Activities/SideJob.h"
#include "../../Activities/StreetRace.h"

// Proves authored activity payloads compile into the REAL pure models and run to completion
// headlessly — the model-level playback the god-mode/PIE phase will drive from the world.

// 1) Authored side job compiles, prices, and runs Pickup -> Dropoff -> Complete.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcActivitySideJobTest,
    "GTC.Missions.Editor.Activity.SideJob",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcActivitySideJobTest::RunTest(const FString& Parameters)
{
    FGtcSideJobDefinition D;
    D.Kind = static_cast<int32>(SideJob::EKind::Delivery);
    D.Pickup = FVector(0.0, 0.0, 0.0);
    D.Dropoff = FVector(100.0, 0.0, 0.0);
    D.BaseReward = 200;
    D.ParTime = 30.0;

    const SideJob::FJob Job = GtcActivityCompiler::CompileSideJob(D);
    TestEqual(TEXT("kind"), Job.Kind, static_cast<int32>(SideJob::EKind::Delivery));
    TestEqual(TEXT("base"), Job.BaseReward, 200);
    TestTrue(TEXT("pickup"), Job.Pickup.Equals(D.Pickup));
    TestTrue(TEXT("dropoff"), Job.Dropoff.Equals(D.Dropoff));

    const int32 Pay = GtcActivityCompiler::SideJobPayout(D, 100.0, 25.0);
    TestTrue(TEXT("payout positive"), Pay > 0);
    TestTrue(TEXT("payout at least base"), Pay >= D.BaseReward);

    // Run the real stateful tracker with the compiled job.
    SideJob Tracker;
    Tracker.Start(Job);
    TestTrue(TEXT("active"), Tracker.HasActive());
    TestTrue(TEXT("starts at pickup"), Tracker.Stage() == SideJob::EStage::Pickup);
    Tracker.AdvanceStage();
    TestTrue(TEXT("advances to dropoff"), Tracker.Stage() == SideJob::EStage::Dropoff);
    Tracker.Complete();
    TestFalse(TEXT("cleared"), Tracker.HasActive());
    TestEqual(TEXT("one completed"), Tracker.CompletedCount(), 1);

    return true;
}

// 2) Authored street race compiles into a real StreetRace and runs to the finish.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcActivityStreetRaceTest,
    "GTC.Missions.Editor.Activity.StreetRace",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcActivityStreetRaceTest::RunTest(const FString& Parameters)
{
    FGtcStreetRaceDefinition D;
    D.Checkpoints.Add(FVector(0.0, 0.0, 0.0));
    D.Checkpoints.Add(FVector(100.0, 0.0, 0.0));
    D.Checkpoints.Add(FVector(200.0, 0.0, 0.0));
    D.Laps = 2;
    D.BaseReward = 1000;

    StreetRace Race = GtcActivityCompiler::CompileRace(D);
    TestFalse(TEXT("not finished at start"), Race.IsFinished());
    TestEqual(TEXT("two laps"), Race.TotalLaps(), 2);

    // Drive through every gate: 3 checkpoints x 2 laps = 6 hits.
    int32 Hits = 0;
    for (int32 Gate = 0; Gate < 6; ++Gate)
    {
        const FVector Target = Race.CurrentCheckpoint();
        if (Race.Reached(Target, 50.0))
        {
            ++Hits;
        }
    }
    TestEqual(TEXT("six gates cleared"), Hits, 6);
    TestTrue(TEXT("finished"), Race.IsFinished());

    // Reward scales with placement.
    TestEqual(TEXT("first place full base"), GtcActivityCompiler::RaceReward(D, 1), 1000);
    TestTrue(TEXT("fourth place less than base"), GtcActivityCompiler::RaceReward(D, 4) < 1000);
    TestEqual(TEXT("DNF pays nothing"), GtcActivityCompiler::RaceReward(D, 0), 0);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
