// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../MissionObjectiveTypes.h"
#include "../../../Tests/GtcTestTolerances.h"

// Each test below maps 1:1 to an assertion in the the reference reference behavior
// game/tests/unit/test_mission_objective_types.gd.

using GtcTest::Eps;
using MOT = MissionObjectiveTypes;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTReachInsideRadiusTest,
    "GTC.Missions.Core.MissionObjectiveTypes.ReachInsideRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTReachInsideRadiusTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("reach"), MOT::ReachSatisfied(FVector(1, 0, 0), FVector(3, 0, 0), 5.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTReachOutsideRadiusTest,
    "GTC.Missions.Core.MissionObjectiveTypes.ReachOutsideRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTReachOutsideRadiusTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("not reach"), MOT::ReachSatisfied(FVector::ZeroVector, FVector(0, 0, 10), 4.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTReachNegativeRadiusTest,
    "GTC.Missions.Core.MissionObjectiveTypes.ReachNegativeRadiusGuarded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTReachNegativeRadiusTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("exact hit"), MOT::ReachSatisfied(FVector::OneVector, FVector::OneVector, -2.0));
    TestFalse(TEXT("miss"), MOT::ReachSatisfied(FVector::ZeroVector, FVector(0, 1, 0), -2.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTCollectProgressFractionTest,
    "GTC.Missions.Core.MissionObjectiveTypes.CollectProgressFraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTCollectProgressFractionTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("progress == 0.25"), MOT::CollectProgress(2, 8), 0.25, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTCollectProgressOverCapsTest,
    "GTC.Missions.Core.MissionObjectiveTypes.CollectProgressOverCapsAtOne",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTCollectProgressOverCapsTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("progress == 1.0"), MOT::CollectProgress(12, 5), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTCollectProgressNegativeFlooredTest,
    "GTC.Missions.Core.MissionObjectiveTypes.CollectProgressNegativeFloored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTCollectProgressNegativeFlooredTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("progress == 0.0"), MOT::CollectProgress(-4, 5), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTCollectZeroRequiredInstantTest,
    "GTC.Missions.Core.MissionObjectiveTypes.CollectZeroRequiredInstant",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTCollectZeroRequiredInstantTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("satisfied"), MOT::CollectSatisfied(0, 0));
    TestEqual(TEXT("progress == 1.0"), MOT::CollectProgress(0, 0), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTCollectSatisfiedAtAndOverTest,
    "GTC.Missions.Core.MissionObjectiveTypes.CollectSatisfiedAtAndOverRequired",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTCollectSatisfiedAtAndOverTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("not at 2/3"), MOT::CollectSatisfied(2, 3));
    TestTrue(TEXT("at 3/3"), MOT::CollectSatisfied(3, 3));
    TestTrue(TEXT("over 9/3"), MOT::CollectSatisfied(9, 3));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTEliminateSatisfiedTest,
    "GTC.Missions.Core.MissionObjectiveTypes.EliminateSatisfiedOnlyAtZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTEliminateSatisfiedTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("not at 2"), MOT::EliminateSatisfied(2));
    TestTrue(TEXT("at 0"), MOT::EliminateSatisfied(0));
    TestTrue(TEXT("at -3"), MOT::EliminateSatisfied(-3));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTEscortFailedTest,
    "GTC.Missions.Core.MissionObjectiveTypes.EscortFailedAtZeroHealth",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTEscortFailedTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("failed at 0"), MOT::EscortFailed(0.0));
    TestTrue(TEXT("failed at -1"), MOT::EscortFailed(-1.0));
    TestFalse(TEXT("alive at 12"), MOT::EscortFailed(12.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTEscortSatisfiedTest,
    "GTC.Missions.Core.MissionObjectiveTypes.EscortSatisfiedAtDestination",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTEscortSatisfiedTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("at dest"), MOT::EscortSatisfied(FVector(9, 0, 1), FVector(10, 0, 1), 2.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTEscortNotSatisfiedTest,
    "GTC.Missions.Core.MissionObjectiveTypes.EscortNotSatisfiedFarFromDestination",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTEscortNotSatisfiedTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("far"), MOT::EscortSatisfied(FVector::ZeroVector, FVector(50, 0, 0), 2.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTSurviveProgressRampsTest,
    "GTC.Missions.Core.MissionObjectiveTypes.SurviveProgressRamps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTSurviveProgressRampsTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("progress == 0.25"), MOT::SurviveProgress(15.0, 60.0), 0.25, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTSurviveProgressCapsFloorsTest,
    "GTC.Missions.Core.MissionObjectiveTypes.SurviveProgressCapsAndFloors",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTSurviveProgressCapsFloorsTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("caps at 1.0"), MOT::SurviveProgress(90.0, 60.0), 1.0, Eps);
    TestEqual(TEXT("floors at 0.0"), MOT::SurviveProgress(-5.0, 60.0), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTSurviveSatisfiedTest,
    "GTC.Missions.Core.MissionObjectiveTypes.SurviveSatisfiedAtDuration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTSurviveSatisfiedTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("not at 59"), MOT::SurviveSatisfied(59.0, 60.0));
    TestTrue(TEXT("at 60"), MOT::SurviveSatisfied(60.0, 60.0));
    TestTrue(TEXT("over 75"), MOT::SurviveSatisfied(75.0, 60.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTDefendFailedTest,
    "GTC.Missions.Core.MissionObjectiveTypes.DefendFailedThreshold",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTDefendFailedTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("failed at 0"), MOT::DefendFailed(0.0));
    TestTrue(TEXT("failed at -10"), MOT::DefendFailed(-10.0));
    TestFalse(TEXT("alive at 0.5"), MOT::DefendFailed(0.5));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTKindNameRoundTripTest,
    "GTC.Missions.Core.MissionObjectiveTypes.KindNameRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTKindNameRoundTripTest::RunTest(const FString& Parameters)
{
    TestEqual(
        TEXT("survive"),
        MOT::KindName(static_cast<int32>(MOT::EKind::Survive)),
        FString(TEXT("survive")));
    TestEqual(
        TEXT("defend"),
        MOT::KindName(static_cast<int32>(MOT::EKind::Defend)),
        FString(TEXT("defend")));
    TestEqual(TEXT("unknown == empty"), MOT::KindName(999), FString());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTCounterAddRemainingTest,
    "GTC.Missions.Core.MissionObjectiveTypes.CounterAddAndRemaining",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTCounterAddRemainingTest::RunTest(const FString& Parameters)
{
    MOT::Counter C(5);
    C.Add(2);
    TestEqual(TEXT("count == 2"), C.Count(), 2);
    TestEqual(TEXT("remaining == 3"), C.Remaining(), 3);
    TestFalse(TEXT("not done"), C.IsDone());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTCounterDoneOverCapTest,
    "GTC.Missions.Core.MissionObjectiveTypes.CounterDoneAndOverCap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTCounterDoneOverCapTest::RunTest(const FString& Parameters)
{
    MOT::Counter C(3);
    C.Add(10);
    TestTrue(TEXT("done"), C.IsDone());
    TestEqual(TEXT("remaining == 0"), C.Remaining(), 0);
    TestEqual(TEXT("count == 10"), C.Count(), 10);
    TestEqual(TEXT("progress == 1.0"), C.Progress(), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTCounterNegativeDeltaIgnoredTest,
    "GTC.Missions.Core.MissionObjectiveTypes.CounterNegativeDeltaIgnored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTCounterNegativeDeltaIgnoredTest::RunTest(const FString& Parameters)
{
    MOT::Counter C(4);
    C.Add(-7);
    C.Add(0);
    TestEqual(TEXT("count == 0"), C.Count(), 0);
    TestEqual(TEXT("progress == 0.0"), C.Progress(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTCounterProgressFractionTest,
    "GTC.Missions.Core.MissionObjectiveTypes.CounterProgressFraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTCounterProgressFractionTest::RunTest(const FString& Parameters)
{
    MOT::Counter C(4);
    C.Add(1);
    TestEqual(TEXT("progress == 0.25"), C.Progress(), 0.25, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTCounterZeroTargetDoneTest,
    "GTC.Missions.Core.MissionObjectiveTypes.CounterZeroTargetDoneImmediately",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTCounterZeroTargetDoneTest::RunTest(const FString& Parameters)
{
    MOT::Counter C(0);
    TestTrue(TEXT("done"), C.IsDone());
    TestEqual(TEXT("remaining == 0"), C.Remaining(), 0);
    TestEqual(TEXT("progress == 1.0"), C.Progress(), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMOTCounterResetClearsTest,
    "GTC.Missions.Core.MissionObjectiveTypes.CounterResetClearsCount",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMOTCounterResetClearsTest::RunTest(const FString& Parameters)
{
    MOT::Counter C(5);
    C.Add(4);
    C.Reset();
    TestEqual(TEXT("count == 0"), C.Count(), 0);
    TestFalse(TEXT("not done"), C.IsDone());
    TestEqual(TEXT("target == 5"), C.Target(), 5);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
