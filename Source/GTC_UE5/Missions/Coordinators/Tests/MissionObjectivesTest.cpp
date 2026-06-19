// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../MissionObjectives.h"

// PARITY tests (1:1). Each test below maps to one func in the the reference reference behavior
// game/tests/unit/test_mission_objectives.gd (7 funcs), with the oracle's own
// literals. the reference compound `a and b` returns are split into separate assertions.

namespace MissionObjectivesTestNS
{
    // Oracle: const DEFS := [{a, Reach A}, {b, Reach B}]; _mission() -> new("Test", DEFS).
    FMissionObjectives MakeMissionObjectivesSample()
    {
        return FMissionObjectives(
            TEXT("Test"),
            { { TEXT("a"), TEXT("Reach A") }, { TEXT("b"), TEXT("Reach B") } });
    }
} // namespace MissionObjectivesTestNS
using namespace MissionObjectivesTestNS;

// test_starts_inactive_then_active
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionObjectivesStartsInactiveThenActiveTest,
    "GTC.Missions.Objectives.StartsInactiveThenActive",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionObjectivesStartsInactiveThenActiveTest::RunTest(const FString& Parameters)
{
    FMissionObjectives M = MakeMissionObjectivesSample();
    TestEqual(
        TEXT("starts INACTIVE"),
        static_cast<int32>(M.State),
        static_cast<int32>(FMissionObjectives::EObjectiveSetState::Inactive));
    M.Start();
    TestTrue(TEXT("active after start"), M.IsActive());
    return true;
}

// test_cannot_complete_before_start
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionObjectivesCannotCompleteBeforeStartTest,
    "GTC.Missions.Objectives.CannotCompleteBeforeStart",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionObjectivesCannotCompleteBeforeStartTest::RunTest(const FString& Parameters)
{
    FMissionObjectives M = MakeMissionObjectivesSample();
    TestFalse(TEXT("complete before start == false"), M.CompleteObjective(TEXT("a")));
    return true;
}

// test_completing_all_objectives_completes_mission
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionObjectivesCompletingAllCompletesMissionTest,
    "GTC.Missions.Objectives.CompletingAllObjectivesCompletesMission",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionObjectivesCompletingAllCompletesMissionTest::RunTest(const FString& Parameters)
{
    FMissionObjectives M = MakeMissionObjectivesSample();
    M.Start();
    M.CompleteObjective(TEXT("a"));
    TestFalse(TEXT("not complete with one left"), M.IsComplete());
    M.CompleteObjective(TEXT("b"));
    TestTrue(TEXT("complete after both"), M.IsComplete());
    return true;
}

// test_progress_counts
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionObjectivesProgressCountsTest,
    "GTC.Missions.Objectives.ProgressCounts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionObjectivesProgressCountsTest::RunTest(const FString& Parameters)
{
    FMissionObjectives M = MakeMissionObjectivesSample();
    M.Start();
    M.CompleteObjective(TEXT("a"));
    // Oracle: progress() == Vector2i(1, 2).
    TestEqual(TEXT("progress == (1,2)"), M.Progress(), FIntPoint(1, 2));
    return true;
}

// test_unknown_objective_is_noop
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionObjectivesUnknownObjectiveNoopTest,
    "GTC.Missions.Objectives.UnknownObjectiveIsNoop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionObjectivesUnknownObjectiveNoopTest::RunTest(const FString& Parameters)
{
    FMissionObjectives M = MakeMissionObjectivesSample();
    M.Start();
    TestFalse(TEXT("unknown id == false"), M.CompleteObjective(TEXT("zzz")));
    return true;
}

// test_double_complete_is_noop
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionObjectivesDoubleCompleteNoopTest,
    "GTC.Missions.Objectives.DoubleCompleteIsNoop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionObjectivesDoubleCompleteNoopTest::RunTest(const FString& Parameters)
{
    FMissionObjectives M = MakeMissionObjectivesSample();
    M.Start();
    M.CompleteObjective(TEXT("a"));
    TestFalse(TEXT("second complete of a == false"), M.CompleteObjective(TEXT("a")));
    return true;
}

// test_fail_blocks_further_progress
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionObjectivesFailBlocksProgressTest,
    "GTC.Missions.Objectives.FailBlocksFurtherProgress",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionObjectivesFailBlocksProgressTest::RunTest(const FString& Parameters)
{
    FMissionObjectives M = MakeMissionObjectivesSample();
    M.Start();
    M.Fail();
    // Oracle compound: state == FAILED and complete_objective("a") == false.
    TestEqual(
        TEXT("state FAILED"),
        static_cast<int32>(M.State),
        static_cast<int32>(FMissionObjectives::EObjectiveSetState::Failed));
    TestFalse(TEXT("complete blocked after fail"), M.CompleteObjective(TEXT("a")));
    return true;
}

#endif // WITH_AUTOMATION_TESTS
