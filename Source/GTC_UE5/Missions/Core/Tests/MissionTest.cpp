// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../Mission.h"
#include "../../../Tests/GtcTestTolerances.h"

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_mission.gd.

using GtcTest::Eps;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionStartsActiveTest,
    "GTC.Missions.Core.Mission.StartsActive",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionStartsActiveTest::RunTest(const FString& Parameters)
{
    Mission M(TEXT("Rampage"), TEXT("Take down targets"), 5);
    TestTrue(TEXT("is_active"), M.IsActive());
    TestEqual(TEXT("progress == 0"), M.Progress, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionRecordAdvancesProgressTest,
    "GTC.Missions.Core.Mission.RecordAdvancesProgress",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionRecordAdvancesProgressTest::RunTest(const FString& Parameters)
{
    Mission M(TEXT("Rampage"), TEXT("Take down targets"), 5);
    M.Record();
    M.Record(2);
    TestEqual(TEXT("progress == 3"), M.Progress, 3);
    TestTrue(TEXT("is_active"), M.IsActive());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionCompletesAtRequiredTest,
    "GTC.Missions.Core.Mission.CompletesAtRequired",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionCompletesAtRequiredTest::RunTest(const FString& Parameters)
{
    Mission M(TEXT("Rampage"), TEXT("Take down targets"), 3);
    M.Record(3);
    TestTrue(TEXT("status == COMPLETE"), M.Status == Mission::EStatus::Complete);
    TestFalse(TEXT("not is_active"), M.IsActive());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionProgressDoesNotOverflowTest,
    "GTC.Missions.Core.Mission.ProgressDoesNotOverflow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionProgressDoesNotOverflowTest::RunTest(const FString& Parameters)
{
    Mission M(TEXT("Rampage"), TEXT("Take down targets"), 3);
    M.Record(10);
    TestEqual(TEXT("progress == 3"), M.Progress, 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionNoProgressAfterCompleteTest,
    "GTC.Missions.Core.Mission.NoProgressAfterComplete",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionNoProgressAfterCompleteTest::RunTest(const FString& Parameters)
{
    Mission M(TEXT("Rampage"), TEXT("Take down targets"), 1);
    M.Record();
    M.Record();
    TestEqual(TEXT("progress == 1"), M.Progress, 1);
    TestTrue(TEXT("status == COMPLETE"), M.Status == Mission::EStatus::Complete);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionTimerFailsWhenElapsedTest,
    "GTC.Missions.Core.Mission.TimerFailsWhenElapsed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionTimerFailsWhenElapsedTest::RunTest(const FString& Parameters)
{
    Mission M(TEXT("Rampage"), TEXT("Take down targets"), 5, 10.0);
    M.Tick(11.0);
    TestTrue(TEXT("status == FAILED"), M.Status == Mission::EStatus::Failed);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionTimerDoesNotFailEarlyTest,
    "GTC.Missions.Core.Mission.TimerDoesNotFailEarly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionTimerDoesNotFailEarlyTest::RunTest(const FString& Parameters)
{
    Mission M(TEXT("Rampage"), TEXT("Take down targets"), 5, 10.0);
    M.Tick(4.0);
    TestTrue(TEXT("is_active"), M.IsActive());
    TestEqual(TEXT("time_left == 6.0"), M.TimeLeft, 6.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionUntimedNeverFailsTest,
    "GTC.Missions.Core.Mission.UntimedNeverFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionUntimedNeverFailsTest::RunTest(const FString& Parameters)
{
    Mission M(TEXT("Rampage"), TEXT("Take down targets"), 5, 0.0);
    M.Tick(9999.0);
    TestTrue(TEXT("is_active"), M.IsActive());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionNoRecordAfterFailureTest,
    "GTC.Missions.Core.Mission.NoRecordAfterFailure",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionNoRecordAfterFailureTest::RunTest(const FString& Parameters)
{
    Mission M(TEXT("Rampage"), TEXT("Take down targets"), 5, 1.0);
    M.Tick(2.0);
    M.Record(3);
    TestTrue(TEXT("status == FAILED"), M.Status == Mission::EStatus::Failed);
    TestEqual(TEXT("progress == 0"), M.Progress, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionFractionTest,
    "GTC.Missions.Core.Mission.Fraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionFractionTest::RunTest(const FString& Parameters)
{
    Mission M(TEXT("Rampage"), TEXT("Take down targets"), 4);
    M.Record(1);
    TestEqual(TEXT("fraction == 0.25"), M.Fraction(), 0.25, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionResetReactivatesTest,
    "GTC.Missions.Core.Mission.ResetReactivates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionResetReactivatesTest::RunTest(const FString& Parameters)
{
    Mission M(TEXT("Rampage"), TEXT("Take down targets"), 2, 5.0);
    M.Record(2);
    M.Reset();
    TestTrue(TEXT("is_active"), M.IsActive());
    TestEqual(TEXT("progress == 0"), M.Progress, 0);
    TestEqual(TEXT("time_left == 5.0"), M.TimeLeft, 5.0, Eps);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
