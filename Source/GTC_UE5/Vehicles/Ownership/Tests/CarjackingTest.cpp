// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../Carjacking.h"

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_carjacking.gd (22 tests).

static constexpr double CarjackEps = 1.0e-4;

// --- can_reach -------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingCanReachWithinTest,
    "GTC.Vehicles.Ownership.Carjacking.CanReachWithinRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingCanReachWithinTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("1.5m within 2.0m"),
        Carjacking::CanReach(FVector(1.5, 0.0, 0.0), FVector::ZeroVector, 2.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingCanReachIgnoresHeightTest,
    "GTC.Vehicles.Ownership.Carjacking.CanReachIgnoresHeight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingCanReachIgnoresHeightTest::RunTest(const FString& Parameters)
{
    // Same flat distance (1.5), player 5 m above on a ramp (Y is up) — still reachable.
    TestTrue(TEXT("height ignored"),
        Carjacking::CanReach(FVector(1.5, 5.0, 0.0), FVector::ZeroVector, 2.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingCannotReachBeyondTest,
    "GTC.Vehicles.Ownership.Carjacking.CannotReachBeyondRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingCannotReachBeyondTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("3.0m beyond 2.0m"),
        Carjacking::CanReach(FVector(3.0, 0.0, 0.0), FVector::ZeroVector, 2.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingZeroRadiusTest,
    "GTC.Vehicles.Ownership.Carjacking.ZeroRadiusNeverReaches",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingZeroRadiusTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("zero radius"),
        Carjacking::CanReach(FVector::ZeroVector, FVector::ZeroVector, 0.0));
    return true;
}

// --- door_side -------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingDoorDriverLeftTest,
    "GTC.Vehicles.Ownership.Carjacking.DoorSideDriverLeft",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingDoorDriverLeftTest::RunTest(const FString& Parameters)
{
    // Car faces +Z; right = +X, so a player at -X is on the left (driver) side.
    TestEqual(TEXT("driver side -1"),
        Carjacking::DoorSide(FVector::ZeroVector, FVector(0.0, 0.0, 1.0), FVector(-1.0, 0.0, 0.0)), -1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingDoorPassengerRightTest,
    "GTC.Vehicles.Ownership.Carjacking.DoorSidePassengerRight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingDoorPassengerRightTest::RunTest(const FString& Parameters)
{
    // Car faces +Z; player at +X is on the right (passenger) side.
    TestEqual(TEXT("passenger side +1"),
        Carjacking::DoorSide(FVector::ZeroVector, FVector(0.0, 0.0, 1.0), FVector(1.0, 0.0, 0.0)), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingDoorDeadCentreTest,
    "GTC.Vehicles.Ownership.Carjacking.DoorSideDeadCentreIsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingDoorDeadCentreTest::RunTest(const FString& Parameters)
{
    // Player directly ahead — no lateral offset.
    TestEqual(TEXT("centre 0"),
        Carjacking::DoorSide(FVector::ZeroVector, FVector(0.0, 0.0, 1.0), FVector(0.0, 0.0, 4.0)), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingDoorNoForwardTest,
    "GTC.Vehicles.Ownership.Carjacking.DoorSideNoForwardIsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingDoorNoForwardTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("no forward 0"),
        Carjacking::DoorSide(FVector::ZeroVector, FVector::ZeroVector, FVector(1.0, 0.0, 0.0)), 0);
    return true;
}

// --- crime / heat ----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingOccupiedCrimeTest,
    "GTC.Vehicles.Ownership.Carjacking.OccupiedIsCrime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingOccupiedCrimeTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("occupied crime"), Carjacking::IsOccupiedCrime(true));
    TestFalse(TEXT("empty not crime"), Carjacking::IsOccupiedCrime(false));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingHeatOccupiedTest,
    "GTC.Vehicles.Ownership.Carjacking.HeatOccupiedDrawsBase",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingHeatOccupiedTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("heat == 2.5"), Carjacking::HeatForJack(true, 2.5), 2.5, CarjackEps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingHeatEmptyTest,
    "GTC.Vehicles.Ownership.Carjacking.HeatEmptyDrawsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingHeatEmptyTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("heat == 0"), Carjacking::HeatForJack(false, 2.5), 0.0, CarjackEps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingHeatNegativeFlooredTest,
    "GTC.Vehicles.Ownership.Carjacking.HeatNegativeBaseFloored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingHeatNegativeFlooredTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("heat == 0"), Carjacking::HeatForJack(true, -4.0), 0.0, CarjackEps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingResistRangeTest,
    "GTC.Vehicles.Ownership.Carjacking.ResistModifierRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingResistRangeTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("0.0 -> 1.0"), Carjacking::ResistModifier(0.0), 1.0, CarjackEps);
    TestEqual(TEXT("1.0 -> 2.0"), Carjacking::ResistModifier(1.0), 2.0, CarjackEps);
    TestEqual(TEXT("2.0 -> 2.0"), Carjacking::ResistModifier(2.0), 2.0, CarjackEps);
    return true;
}

// --- struggle timer --------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingStartsIdleTest,
    "GTC.Vehicles.Ownership.Carjacking.StartsIdle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingStartsIdleTest::RunTest(const FString& Parameters)
{
    Carjacking J(1.0);
    TestEqual(TEXT("progress == 0"), J.Progress(), 0.0, CarjackEps);
    TestFalse(TEXT("not complete"), J.IsComplete());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingTickBeforeBeginTest,
    "GTC.Vehicles.Ownership.Carjacking.TickBeforeBeginDoesNothing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingTickBeforeBeginTest::RunTest(const FString& Parameters)
{
    Carjacking J(1.0);
    J.Tick(2.0);
    TestEqual(TEXT("progress == 0"), J.Progress(), 0.0, CarjackEps);
    TestFalse(TEXT("not complete"), J.IsComplete());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingNotCompleteUntilTest,
    "GTC.Vehicles.Ownership.Carjacking.NotCompleteUntilDurationElapses",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingNotCompleteUntilTest::RunTest(const FString& Parameters)
{
    Carjacking J(1.0);
    J.Begin();
    J.Tick(0.4);
    J.Tick(0.4);
    // 0.8 s of a 1.0 s struggle — still wrestling.
    TestEqual(TEXT("progress == 0.8"), J.Progress(), 0.8, CarjackEps);
    TestFalse(TEXT("not complete"), J.IsComplete());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingCompletesTest,
    "GTC.Vehicles.Ownership.Carjacking.CompletesWhenDurationReached",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingCompletesTest::RunTest(const FString& Parameters)
{
    Carjacking J(1.0);
    J.Begin();
    J.Tick(0.6);
    J.Tick(0.6);
    // 1.2 s >= 1.0 s — driver out, progress clamps to 1.0.
    TestTrue(TEXT("complete"), J.IsComplete());
    TestEqual(TEXT("progress == 1.0"), J.Progress(), 1.0, CarjackEps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingCompleteFlipsOnceTest,
    "GTC.Vehicles.Ownership.Carjacking.CompleteFlipsOnce",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingCompleteFlipsOnceTest::RunTest(const FString& Parameters)
{
    Carjacking J(1.0);
    J.Begin();
    J.Tick(1.0);
    const bool bFirst = J.IsComplete();
    J.Tick(5.0);
    // Extra ticks after completion are inert and progress stays clamped.
    TestTrue(TEXT("first complete"), bFirst);
    TestTrue(TEXT("still complete"), J.IsComplete());
    TestEqual(TEXT("progress == 1.0"), J.Progress(), 1.0, CarjackEps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingNegativeDeltaTest,
    "GTC.Vehicles.Ownership.Carjacking.NegativeDeltaIgnored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingNegativeDeltaTest::RunTest(const FString& Parameters)
{
    Carjacking J(1.0);
    J.Begin();
    J.Tick(0.5);
    J.Tick(-0.5);
    TestEqual(TEXT("progress == 0.5"), J.Progress(), 0.5, CarjackEps);
    TestFalse(TEXT("not complete"), J.IsComplete());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingCancelTest,
    "GTC.Vehicles.Ownership.Carjacking.CancelAbortsAndStaysIncomplete",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingCancelTest::RunTest(const FString& Parameters)
{
    Carjacking J(1.0);
    J.Begin();
    J.Tick(0.5);
    J.Cancel();
    // Walked away: not complete, and further ticks can't revive it.
    J.Tick(2.0);
    TestFalse(TEXT("not complete"), J.IsComplete());
    TestEqual(TEXT("progress == 0"), J.Progress(), 0.0, CarjackEps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingBeginRearmsTest,
    "GTC.Vehicles.Ownership.Carjacking.BeginRearmsAfterComplete",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingBeginRearmsTest::RunTest(const FString& Parameters)
{
    Carjacking J(1.0);
    J.Begin();
    J.Tick(1.0);
    const bool bDone = J.IsComplete();
    J.Begin();
    // Re-armed: completion cleared, clock back to zero.
    TestTrue(TEXT("was done"), bDone);
    TestFalse(TEXT("not complete after rearm"), J.IsComplete());
    TestEqual(TEXT("progress == 0"), J.Progress(), 0.0, CarjackEps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingResetTest,
    "GTC.Vehicles.Ownership.Carjacking.ResetClearsState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingResetTest::RunTest(const FString& Parameters)
{
    Carjacking J(1.0);
    J.Begin();
    J.Tick(0.7);
    J.Reset();
    J.Tick(2.0);
    TestFalse(TEXT("not complete"), J.IsComplete());
    TestEqual(TEXT("progress == 0"), J.Progress(), 0.0, CarjackEps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarjackingResistLengthensTest,
    "GTC.Vehicles.Ownership.Carjacking.ResistModifierLengthensStruggle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarjackingResistLengthensTest::RunTest(const FString& Parameters)
{
    // A max-toughness driver doubles a 1.0 s base to 2.0 s.
    Carjacking J(1.0 * Carjacking::ResistModifier(1.0));
    J.Begin();
    J.Tick(1.0);
    // Halfway through the doubled struggle — not out yet.
    TestFalse(TEXT("not complete"), J.IsComplete());
    TestEqual(TEXT("progress == 0.5"), J.Progress(), 0.5, CarjackEps);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
