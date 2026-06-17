// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../MissionFlow.h"

// Each test below maps 1:1 to an assertion in the the reference reference behavior
// game/tests/unit/test_mission_flow.gd.

namespace MissionFlowTestNS
{
// Mirrors the oracle's _objectives(done_flags): a 3-step list.
TArray<MissionFlow::FFlowObjective> Objectives(bool A, bool B, bool C)
{
    return {
        MissionFlow::FFlowObjective(TEXT("reach_a"), TEXT("Reach the docks"), A),
        MissionFlow::FFlowObjective(TEXT("reach_b"), TEXT("Reach the lighthouse"), B),
        MissionFlow::FFlowObjective(TEXT("survive"), TEXT("Survive 30s"), C),
    };
}
} // namespace MissionFlowTestNS
using namespace MissionFlowTestNS;

// --- current sequencing ----------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFlowCurrentIndexFirstOpenTest,
    "GTC.Missions.Core.MissionFlow.CurrentIndexIsFirstOpen",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFlowCurrentIndexFirstOpenTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("current_index == 1"), MissionFlow::CurrentIndex(Objectives(true, false, false)), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFlowCurrentIndexAllDoneTest,
    "GTC.Missions.Core.MissionFlow.CurrentIndexNoneWhenAllDone",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFlowCurrentIndexAllDoneTest::RunTest(const FString& Parameters)
{
    TestEqual(
        TEXT("current_index == NO_INDEX"),
        MissionFlow::CurrentIndex(Objectives(true, true, true)),
        MissionFlow::NoIndex);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFlowCurrentIndexEmptyTest,
    "GTC.Missions.Core.MissionFlow.CurrentIndexNoneWhenEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFlowCurrentIndexEmptyTest::RunTest(const FString& Parameters)
{
    TestEqual(
        TEXT("current_index == NO_INDEX"),
        MissionFlow::CurrentIndex(TArray<MissionFlow::FFlowObjective>{}),
        MissionFlow::NoIndex);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFlowCurrentReturnsDictTest,
    "GTC.Missions.Core.MissionFlow.CurrentReturnsObjectiveDict",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFlowCurrentReturnsDictTest::RunTest(const FString& Parameters)
{
    TestEqual(
        TEXT("current id == reach_b"),
        MissionFlow::Current(Objectives(true, false, false)).Id,
        FString(TEXT("reach_b")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFlowCurrentTextFirstOpenTest,
    "GTC.Missions.Core.MissionFlow.CurrentTextFirstOpen",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFlowCurrentTextFirstOpenTest::RunTest(const FString& Parameters)
{
    TestEqual(
        TEXT("current_text == Reach the docks"),
        MissionFlow::CurrentText(Objectives(false, false, false)),
        FString(TEXT("Reach the docks")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFlowCurrentTextEmptyTest,
    "GTC.Missions.Core.MissionFlow.CurrentTextEmptyWhenDone",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFlowCurrentTextEmptyTest::RunTest(const FString& Parameters)
{
    TestEqual(
        TEXT("current_text == empty"),
        MissionFlow::CurrentText(Objectives(true, true, true)),
        FString());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFlowDoneCountTest,
    "GTC.Missions.Core.MissionFlow.DoneCount",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFlowDoneCountTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("done_count == 2"), MissionFlow::DoneCount(Objectives(true, false, true)), 2);
    return true;
}

// --- hud line --------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFlowHudLineCurrentTest,
    "GTC.Missions.Core.MissionFlow.HudLineShowsCurrentObjective",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFlowHudLineCurrentTest::RunTest(const FString& Parameters)
{
    TestEqual(
        TEXT("hud line"),
        MissionFlow::HudLine(TEXT("HEIST"), Objectives(true, false, false)),
        FString(TEXT("HEIST — Reach the lighthouse (2/3)")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFlowHudLineCompleteTest,
    "GTC.Missions.Core.MissionFlow.HudLineComplete",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFlowHudLineCompleteTest::RunTest(const FString& Parameters)
{
    TestEqual(
        TEXT("hud line complete"),
        MissionFlow::HudLine(TEXT("HEIST"), Objectives(true, true, true)),
        FString(TEXT("HEIST — complete (3/3)")));
    return true;
}

// --- fail conditions -------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFlowTimedOutTrueTest,
    "GTC.Missions.Core.MissionFlow.TimedOutTrueWhenClockZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFlowTimedOutTrueTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("timed_out"), MissionFlow::TimedOut(30.0, 0.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFlowTimedOutFalseTest,
    "GTC.Missions.Core.MissionFlow.TimedOutFalseWithTimeLeft",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFlowTimedOutFalseTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("not timed_out"), MissionFlow::TimedOut(30.0, 5.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFlowUntimedNeverTimesOutTest,
    "GTC.Missions.Core.MissionFlow.UntimedNeverTimesOut",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFlowUntimedNeverTimesOutTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("not timed_out"), MissionFlow::TimedOut(0.0, -1.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFlowShouldFailDeathTest,
    "GTC.Missions.Core.MissionFlow.ShouldFailOnPlayerDeath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFlowShouldFailDeathTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("should_fail"), MissionFlow::ShouldFail(true, 0.0, 99.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFlowShouldFailTimeoutTest,
    "GTC.Missions.Core.MissionFlow.ShouldFailOnTimeout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFlowShouldFailTimeoutTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("should_fail"), MissionFlow::ShouldFail(false, 30.0, 0.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFlowShouldNotFailTest,
    "GTC.Missions.Core.MissionFlow.ShouldNotFailWhenAliveAndTimed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFlowShouldNotFailTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("not should_fail"), MissionFlow::ShouldFail(false, 30.0, 12.0));
    return true;
}

// --- waypoints -------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFlowWaypointMapsCurrentTest,
    "GTC.Missions.Core.MissionFlow.CurrentWaypointMapsCurrentObjective",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFlowWaypointMapsCurrentTest::RunTest(const FString& Parameters)
{
    TMap<FString, FVector> Wp;
    Wp.Add(TEXT("reach_a"), FVector(10, 0, 5));
    Wp.Add(TEXT("reach_b"), FVector(40, 0, -8));
    TestEqual(
        TEXT("waypoint == (40,0,-8)"),
        MissionFlow::CurrentWaypoint(Objectives(true, false, false), Wp, FVector::ZeroVector),
        FVector(40, 0, -8));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFlowWaypointFallbackUnmappedTest,
    "GTC.Missions.Core.MissionFlow.CurrentWaypointFallbackWhenUnmapped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFlowWaypointFallbackUnmappedTest::RunTest(const FString& Parameters)
{
    const FVector Fallback(1, 2, 3);
    TestEqual(
        TEXT("waypoint == fallback"),
        MissionFlow::CurrentWaypoint(Objectives(false, false, false), TMap<FString, FVector>{}, Fallback),
        Fallback);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFlowWaypointFallbackCompleteTest,
    "GTC.Missions.Core.MissionFlow.CurrentWaypointFallbackWhenComplete",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFlowWaypointFallbackCompleteTest::RunTest(const FString& Parameters)
{
    const FVector Fallback(7, 7, 7);
    TMap<FString, FVector> Wp;
    Wp.Add(TEXT("reach_a"), FVector(10, 0, 5));
    TestEqual(
        TEXT("waypoint == fallback"),
        MissionFlow::CurrentWaypoint(Objectives(true, true, true), Wp, Fallback),
        Fallback);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
