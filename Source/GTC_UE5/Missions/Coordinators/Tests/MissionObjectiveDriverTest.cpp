// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../MissionObjectiveDriver.h"
#include "../MissionController.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Engine/GameInstance.h"

using GtcTest::Eps;

// PARITY tests (1:1). Each test below maps to one assertion in the Godot parity
// oracle game/tests/unit/test_mission_objective_driver.gd (8 funcs). Constants,
// positions, radii, durations and tolerances are the oracle's, independent of the
// implementation. Godot's compound {satisfied, held} return is split into the
// FVerdict fields bSatisfied / Held.

namespace MissionObjectiveDriverTestNS
{
    // Oracle: const REACH := {"kind": "reach", "radius": 6.0}
    FMissionObjectiveDriver::FObjectiveDef ReachDef()
    {
        return FMissionObjectiveDriver::FObjectiveDef(TEXT("reach"), 6.0, 3.0);
    }
    // Oracle: const HOLD := {"kind": "hold", "radius": 8.0, "duration": 3.0}
    FMissionObjectiveDriver::FObjectiveDef HoldDef()
    {
        return FMissionObjectiveDriver::FObjectiveDef(TEXT("hold"), 8.0, 3.0);
    }
} // namespace MissionObjectiveDriverTestNS
using namespace MissionObjectiveDriverTestNS;

// test_reach_satisfied_inside_radius
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionObjectiveDriverReachInsideTest,
    "GTC.Missions.ObjectiveDriver.ReachSatisfiedInsideRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionObjectiveDriverReachInsideTest::RunTest(const FString& Parameters)
{
    const FMissionObjectiveDriver::FVerdict V =
        FMissionObjectiveDriver::Evaluate(ReachDef(), FVector(2, 0, 1), FVector(0, 0, 0), 0.0, 0.016);
    TestTrue(TEXT("satisfied"), V.bSatisfied);
    TestEqual(TEXT("held == 0.0"), V.Held, 0.0, Eps);
    return true;
}

// test_reach_not_satisfied_outside_radius
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionObjectiveDriverReachOutsideTest,
    "GTC.Missions.ObjectiveDriver.ReachNotSatisfiedOutsideRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionObjectiveDriverReachOutsideTest::RunTest(const FString& Parameters)
{
    const FMissionObjectiveDriver::FVerdict V =
        FMissionObjectiveDriver::Evaluate(ReachDef(), FVector(20, 0, 0), FVector(0, 0, 0), 0.0, 0.016);
    TestFalse(TEXT("not satisfied"), V.bSatisfied);
    return true;
}

// test_hold_accumulates_while_inside
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionObjectiveDriverHoldAccumulatesTest,
    "GTC.Missions.ObjectiveDriver.HoldAccumulatesWhileInside",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionObjectiveDriverHoldAccumulatesTest::RunTest(const FString& Parameters)
{
    const FMissionObjectiveDriver::FVerdict V =
        FMissionObjectiveDriver::Evaluate(HoldDef(), FVector(1, 0, 0), FVector::ZeroVector, 1.0, 0.5);
    TestFalse(TEXT("not yet satisfied"), V.bSatisfied);
    TestEqual(TEXT("held == 1.5"), V.Held, 1.5, Eps);
    return true;
}

// test_hold_completes_at_duration
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionObjectiveDriverHoldCompletesTest,
    "GTC.Missions.ObjectiveDriver.HoldCompletesAtDuration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionObjectiveDriverHoldCompletesTest::RunTest(const FString& Parameters)
{
    const FMissionObjectiveDriver::FVerdict V =
        FMissionObjectiveDriver::Evaluate(HoldDef(), FVector(1, 0, 0), FVector::ZeroVector, 2.9, 0.2);
    TestTrue(TEXT("satisfied"), V.bSatisfied);
    TestTrue(TEXT("held >= 3.0"), V.Held >= 3.0);
    return true;
}

// test_hold_resets_when_player_leaves
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionObjectiveDriverHoldResetsTest,
    "GTC.Missions.ObjectiveDriver.HoldResetsWhenPlayerLeaves",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionObjectiveDriverHoldResetsTest::RunTest(const FString& Parameters)
{
    const FMissionObjectiveDriver::FVerdict V =
        FMissionObjectiveDriver::Evaluate(HoldDef(), FVector(50, 0, 0), FVector::ZeroVector, 2.9, 0.2);
    TestFalse(TEXT("not satisfied"), V.bSatisfied);
    TestEqual(TEXT("held == 0.0"), V.Held, 0.0, Eps);
    return true;
}

// test_unknown_kind_degrades_to_reach
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionObjectiveDriverUnknownKindTest,
    "GTC.Missions.ObjectiveDriver.UnknownKindDegradesToReach",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionObjectiveDriverUnknownKindTest::RunTest(const FString& Parameters)
{
    // Oracle: var odd := {"kind": "dance_off", "radius": 5.0}
    const FMissionObjectiveDriver::FObjectiveDef Odd(TEXT("dance_off"), 5.0, 3.0);
    const FMissionObjectiveDriver::FVerdict V =
        FMissionObjectiveDriver::Evaluate(Odd, FVector(1, 0, 1), FVector::ZeroVector, 0.0, 0.016);
    TestTrue(TEXT("satisfied (degrades to reach)"), V.bSatisfied);
    return true;
}

// test_empty_def_uses_defaults
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionObjectiveDriverEmptyDefDefaultsTest,
    "GTC.Missions.ObjectiveDriver.EmptyDefUsesDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionObjectiveDriverEmptyDefDefaultsTest::RunTest(const FString& Parameters)
{
    // Oracle: default kind "reach", default radius 6: 4 m completes, 9 m doesn't.
    // FObjectiveDef's defaults (kind "reach", radius 6.0) are the empty-{} fallbacks.
    const FMissionObjectiveDriver::FObjectiveDef Empty;
    const FMissionObjectiveDriver::FVerdict Near =
        FMissionObjectiveDriver::Evaluate(Empty, FVector(4, 0, 0), FVector::ZeroVector, 0.0, 0.016);
    const FMissionObjectiveDriver::FVerdict Far =
        FMissionObjectiveDriver::Evaluate(Empty, FVector(9, 0, 0), FVector::ZeroVector, 0.0, 0.016);
    TestTrue(TEXT("4m satisfied"), Near.bSatisfied);
    TestFalse(TEXT("9m not satisfied"), Far.bSatisfied);
    return true;
}

// test_controller_reports_active_objective_id
// Oracle-pinned accessor parity: the controller's begin / current_objective_id /
// complete / is_complete sequence. Uses the Wave 2 UMissionController (owned
// objective-set state machine) out of tree, mirroring the oracle's out-of-tree use.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionControllerReportsActiveObjectiveIdTest,
    "GTC.Missions.ObjectiveDriver.ControllerReportsActiveObjectiveId",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionControllerReportsActiveObjectiveIdTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
    UMissionController* Ctl = NewObject<UMissionController>(GameInstance);

    // Oracle: auto_start = false; objective_defs = [{a,A},{b,B}]; begin().
    Ctl->ObjectiveDefs = { { TEXT("a"), TEXT("A") }, { TEXT("b"), TEXT("B") } };
    Ctl->Begin();

    TestEqual(TEXT("first id == a"), Ctl->CurrentObjectiveId(), FString(TEXT("a")));
    Ctl->Complete(TEXT("a"));
    TestEqual(TEXT("second id == b"), Ctl->CurrentObjectiveId(), FString(TEXT("b")));
    Ctl->Complete(TEXT("b"));
    TestEqual(TEXT("done id == empty"), Ctl->CurrentObjectiveId(), FString());
    TestTrue(TEXT("is_complete"), Ctl->IsComplete());
    return true;
}

#endif // WITH_AUTOMATION_TESTS
