// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../MissionController.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Engine/GameInstance.h"

using GtcTest::Eps;

// BEHAVIOR tests (Wave 2 rule) — UMissionController has NO Godot parity oracle of
// its own (only the single accessor sequence pinned in MissionObjectiveDriverTest).
// These assert the UE port's ownership / lifecycle: the owned objective-set state
// machine, delegate emission, the timer/fail driver, and retry-via-Reset — all
// composing the merged, parity-tested MissionFlow. Scene wiring (group "mission",
// HUD poll, player_health poll, FloatingOrigin) is Wave 3; bPlayerDead is passed in.

namespace MissionControllerTestNS
{
    UMissionController* MakeControllerForTest(UGameInstance** OutGameInstance = nullptr)
    {
        UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
        if (OutGameInstance)
        {
            *OutGameInstance = GameInstance;
        }
        return NewObject<UMissionController>(GameInstance);
    }
} // namespace MissionControllerTestNS
using namespace MissionControllerTestNS;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionControllerDefaultsTest,
    "GTC.Missions.Controller.DefaultsMatchGodotExports",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionControllerDefaultsTest::RunTest(const FString& Parameters)
{
    UMissionController* Ctl = MakeControllerForTest();
    TestEqual(TEXT("title == MISSION"), Ctl->Title, FString(TEXT("MISSION")));
    TestEqual(TEXT("time_limit == 0 (untimed)"), Ctl->TimeLimit, 0.0, Eps);
    TestFalse(TEXT("inactive before begin"), Ctl->IsActive());
    TestFalse(TEXT("not complete before begin"), Ctl->IsComplete());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionControllerEmitsAndCompletesTest,
    "GTC.Missions.Controller.EmitsObjectiveAndCompletesMission",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionControllerEmitsAndCompletesTest::RunTest(const FString& Parameters)
{
    UMissionController* Ctl = MakeControllerForTest();
    Ctl->ObjectiveDefs = { { TEXT("a"), TEXT("A") }, { TEXT("b"), TEXT("B") } };

    TArray<FString> CompletedIds;
    int32 MissionDone = 0;
    Ctl->OnObjectiveCompleted.AddLambda([&](const FString& Id) { CompletedIds.Add(Id); });
    Ctl->OnMissionCompleted.AddLambda([&]() { ++MissionDone; });

    Ctl->Begin();
    TestTrue(TEXT("active after begin"), Ctl->IsActive());

    Ctl->Complete(TEXT("a"));
    TestEqual(TEXT("one objective emitted"), CompletedIds.Num(), 1);
    TestEqual(TEXT("emitted id == a"), CompletedIds[0], FString(TEXT("a")));
    TestEqual(TEXT("mission not done yet"), MissionDone, 0);

    // Completing the same objective twice is a no-op (no re-emit).
    Ctl->Complete(TEXT("a"));
    TestEqual(TEXT("no re-emit on repeat"), CompletedIds.Num(), 1);

    Ctl->Complete(TEXT("b"));
    TestEqual(TEXT("two objectives emitted"), CompletedIds.Num(), 2);
    TestEqual(TEXT("mission completed once"), MissionDone, 1);
    TestTrue(TEXT("is_complete"), Ctl->IsComplete());
    TestFalse(TEXT("no longer active"), Ctl->IsActive());

    // Completing after the mission ends is a no-op.
    Ctl->Complete(TEXT("a"));
    TestEqual(TEXT("mission stays completed once"), MissionDone, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionControllerHudAndWaypointTest,
    "GTC.Missions.Controller.HudTextAndWaypointTrackCurrent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionControllerHudAndWaypointTest::RunTest(const FString& Parameters)
{
    UMissionController* Ctl = MakeControllerForTest();
    Ctl->Title = TEXT("HEIST");
    Ctl->ObjectiveDefs = { { TEXT("a"), TEXT("Reach the docks") }, { TEXT("b"), TEXT("Reach the lighthouse") } };
    Ctl->Waypoints.Add(TEXT("a"), FVector(10, 0, 5));
    Ctl->Waypoints.Add(TEXT("b"), FVector(40, 0, -8));
    Ctl->Begin();

    // Drives the merged MissionFlow HUD/waypoint over the owned objective set.
    // HudLine numbers the CURRENT objective (1-based index), matching MissionFlow's
    // own Core oracle: with none done, current is objective 1 of 2.
    TestEqual(TEXT("hud shows current"), Ctl->HudText(), FString(TEXT("HEIST — Reach the docks (1/2)")));
    TestEqual(TEXT("waypoint maps current"), Ctl->CurrentWaypoint(), FVector(10, 0, 5));

    Ctl->Complete(TEXT("a"));
    TestEqual(TEXT("hud advances"), Ctl->HudText(), FString(TEXT("HEIST — Reach the lighthouse (2/2)")));
    TestEqual(TEXT("waypoint advances"), Ctl->CurrentWaypoint(), FVector(40, 0, -8));

    // Unmapped current falls back.
    const FVector Fallback(1, 2, 3);
    UMissionController* Ctl2 = MakeControllerForTest();
    Ctl2->ObjectiveDefs = { { TEXT("x"), TEXT("X") } };
    Ctl2->Begin();
    TestEqual(TEXT("fallback when unmapped"), Ctl2->CurrentWaypoint(Fallback), Fallback);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionControllerTimeoutFailsTest,
    "GTC.Missions.Controller.TickFailsOnTimeout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionControllerTimeoutFailsTest::RunTest(const FString& Parameters)
{
    UMissionController* Ctl = MakeControllerForTest();
    Ctl->ObjectiveDefs = { { TEXT("a"), TEXT("A") } };
    Ctl->TimeLimit = 1.0;

    int32 Failed = 0;
    Ctl->OnMissionFailed.AddLambda([&]() { ++Failed; });
    Ctl->Begin();

    TestEqual(TEXT("starts with full clock"), Ctl->GetTimeLeft(), 1.0, Eps);
    TestFalse(TEXT("no fail mid-clock"), Ctl->Tick(false, 0.4));
    TestEqual(TEXT("clock counts down"), Ctl->GetTimeLeft(), 0.6, Eps);

    const bool bFailed = Ctl->Tick(false, 0.6); // clock hits 0
    TestTrue(TEXT("fails at timeout"), bFailed);
    TestEqual(TEXT("failed once"), Failed, 1);
    TestFalse(TEXT("no longer active"), Ctl->IsActive());

    // Further ticks are inert once ended.
    TestFalse(TEXT("inert after fail"), Ctl->Tick(false, 1.0));
    TestEqual(TEXT("no second fail emit"), Failed, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionControllerDeathFailsTest,
    "GTC.Missions.Controller.TickFailsOnPlayerDeath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionControllerDeathFailsTest::RunTest(const FString& Parameters)
{
    UMissionController* Ctl = MakeControllerForTest();
    Ctl->ObjectiveDefs = { { TEXT("a"), TEXT("A") } };
    Ctl->Begin();

    int32 Failed = 0;
    Ctl->OnMissionFailed.AddLambda([&]() { ++Failed; });

    // Untimed mission, but a dead player always fails.
    TestTrue(TEXT("death fails even untimed"), Ctl->Tick(/*bPlayerDead*/ true, 0.016));
    TestEqual(TEXT("failed once"), Failed, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionControllerResetReplaysTest,
    "GTC.Missions.Controller.ResetRebuildsAndReplays",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionControllerResetReplaysTest::RunTest(const FString& Parameters)
{
    UMissionController* Ctl = MakeControllerForTest();
    Ctl->ObjectiveDefs = { { TEXT("a"), TEXT("A") }, { TEXT("b"), TEXT("B") } };
    Ctl->TimeLimit = 5.0;
    Ctl->Begin();
    Ctl->Complete(TEXT("a"));
    Ctl->Tick(false, 2.0);
    TestEqual(TEXT("progressed"), Ctl->CurrentObjectiveId(), FString(TEXT("b")));
    TestEqual(TEXT("clock spent"), Ctl->GetTimeLeft(), 3.0, Eps);

    // Reset (the retry button): rebuild from scratch + re-begin.
    Ctl->Reset();
    TestTrue(TEXT("active again"), Ctl->IsActive());
    TestEqual(TEXT("back to first objective"), Ctl->CurrentObjectiveId(), FString(TEXT("a")));
    TestEqual(TEXT("clock refilled"), Ctl->GetTimeLeft(), 5.0, Eps);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
