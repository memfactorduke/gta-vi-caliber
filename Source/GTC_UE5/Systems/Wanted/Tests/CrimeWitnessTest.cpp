// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../CrimeWitness.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_crime_witness.gd (25 funcs). LOS booleans / witness counts are
// exact; heat/progress are float curves checked with Eps. Compound `a and b` oracle
// returns are split into separate Test* calls.
//
// Vector axes: the oracle works on the X/Z ground plane with Y up; FVector maps 1:1 to
// Godot Vector3 (X->X, Y->Y, Z->Z), so the literals below are the oracle's literals.

namespace
{
    // Ped-ish defaults from the oracle: a 60-degree half-cone (120 total), 10m sight.
    constexpr double PedFov = UE_DOUBLE_PI / 3.0;
    constexpr double PedRange = 10.0;
}

// --- can_witness -----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessDeadAheadTest,
    "GTC.Systems.Wanted.CrimeWitness.CanWitnessDeadAhead",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessDeadAheadTest::RunTest(const FString& Parameters)
{
    // Observer at origin facing +X, crime 5m straight ahead.
    TestTrue(TEXT("sees dead ahead"), FCrimeWitness::CanWitness(
        FVector::ZeroVector, FVector(1, 0, 0), FVector(5, 0, 0), PedRange, PedFov));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessWithinConeEdgeTest,
    "GTC.Systems.Wanted.CrimeWitness.CanWitnessWithinConeEdge",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessWithinConeEdgeTest::RunTest(const FString& Parameters)
{
    // 45 degrees off-axis is inside a 60-degree half-cone.
    const FVector Crime(5, 0, 5); // 45 deg from +X
    TestTrue(TEXT("sees cone edge"), FCrimeWitness::CanWitness(
        FVector::ZeroVector, FVector(1, 0, 0), Crime, PedRange, PedFov));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessBehindTest,
    "GTC.Systems.Wanted.CrimeWitness.CannotWitnessBehind",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessBehindTest::RunTest(const FString& Parameters)
{
    // Crime directly behind the observer is outside any forward FOV.
    TestFalse(TEXT("misses behind"), FCrimeWitness::CanWitness(
        FVector::ZeroVector, FVector(1, 0, 0), FVector(-5, 0, 0), PedRange, PedFov));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessOutsideConeTest,
    "GTC.Systems.Wanted.CrimeWitness.CannotWitnessOutsideCone",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessOutsideConeTest::RunTest(const FString& Parameters)
{
    // 90 degrees to the side is outside a 60-degree half-cone.
    TestFalse(TEXT("misses 90deg"), FCrimeWitness::CanWitness(
        FVector::ZeroVector, FVector(1, 0, 0), FVector(0, 0, 5), PedRange, PedFov));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessRangeBoundaryTest,
    "GTC.Systems.Wanted.CrimeWitness.RangeBoundary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessRangeBoundaryTest::RunTest(const FString& Parameters)
{
    // Dead ahead, 9.9m in range / 20m out of a 10m sight.
    const bool bInside = FCrimeWitness::CanWitness(
        FVector::ZeroVector, FVector(1, 0, 0), FVector(9.9, 0, 0), PedRange, PedFov);
    const bool bOutside = FCrimeWitness::CanWitness(
        FVector::ZeroVector, FVector(1, 0, 0), FVector(20, 0, 0), PedRange, PedFov);
    TestTrue(TEXT("9.9m inside"), bInside);
    TestFalse(TEXT("20m outside"), bOutside);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessZeroFacingTest,
    "GTC.Systems.Wanted.CrimeWitness.ZeroFacingCannotWitness",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessZeroFacingTest::RunTest(const FString& Parameters)
{
    // No defined forward -> sees nothing, even point-blank.
    TestFalse(TEXT("zero facing sees nothing"), FCrimeWitness::CanWitness(
        FVector::ZeroVector, FVector::ZeroVector, FVector(1, 0, 0), PedRange, PedFov));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessOnTopTest,
    "GTC.Systems.Wanted.CrimeWitness.CrimeOnTopOfObserverSeen",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessOnTopTest::RunTest(const FString& Parameters)
{
    // No meaningful bearing; counts as witnessed.
    TestTrue(TEXT("crime on top is seen"), FCrimeWitness::CanWitness(
        FVector(2, 0, 2), FVector(1, 0, 0), FVector(2, 0, 2), PedRange, PedFov));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessCopWiderTest,
    "GTC.Systems.Wanted.CrimeWitness.CopWiderSeesWhatPedMisses",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessCopWiderTest::RunTest(const FString& Parameters)
{
    // 90 deg to the side: ped (60 half-cone) misses, cop (120 half-cone) catches.
    const FVector Crime(0, 0, 5);
    const bool bPedSees = FCrimeWitness::CanWitness(
        FVector::ZeroVector, FVector(1, 0, 0), Crime, PedRange, PedFov);
    const bool bCopSees = FCrimeWitness::CanWitness(
        FVector::ZeroVector, FVector(1, 0, 0), Crime, 25.0, 2.0 * UE_DOUBLE_PI / 3.0);
    TestFalse(TEXT("ped misses"), bPedSees);
    TestTrue(TEXT("cop sees"), bCopSees);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessIgnoresVerticalTest,
    "GTC.Systems.Wanted.CrimeWitness.IgnoresVerticalOffset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessIgnoresVerticalTest::RunTest(const FString& Parameters)
{
    // Crime is 5m ahead but 100m up — still witnessed (we steer on XZ).
    TestTrue(TEXT("ignores vertical"), FCrimeWitness::CanWitness(
        FVector::ZeroVector, FVector(1, 0, 0), FVector(5, 100, 0), PedRange, PedFov));
    return true;
}

// --- count_witnesses -------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessCountOnlySeeingTest,
    "GTC.Systems.Wanted.CrimeWitness.CountOnlyThoseWhoSee",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessCountOnlySeeingTest::RunTest(const FString& Parameters)
{
    const FVector Crime(0, 0, 0);
    const TArray<FCrimeObserver> Observers = {
        FCrimeObserver(FVector(-5, 0, 0), FVector(1, 0, 0)),   // sees (faces crime)
        FCrimeObserver(FVector(5, 0, 0), FVector(1, 0, 0)),    // faces away, misses
        FCrimeObserver(FVector(0, 0, -5), FVector(0, 0, 1)),   // sees
        FCrimeObserver(FVector(100, 0, 0), FVector(-1, 0, 0)), // too far
    };
    TestEqual(TEXT("2 of 4 see"), FCrimeWitness::CountWitnesses(Crime, Observers, PedRange, PedFov), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessEmptyAlleyTest,
    "GTC.Systems.Wanted.CrimeWitness.CountEmptyAlleyZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessEmptyAlleyTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("empty -> 0"), FCrimeWitness::CountWitnesses(
        FVector::ZeroVector, TArray<FCrimeObserver>(), PedRange, PedFov), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessSkipsMalformedTest,
    "GTC.Systems.Wanted.CrimeWitness.CountSkipsMalformedEntries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessSkipsMalformedTest::RunTest(const FString& Parameters)
{
    // The Godot oracle mixes a non-dict and a missing-facing entry into the array; both
    // decode to "can't see" (a non-dict is skipped, a missing facing -> zero facing).
    // The struct port expresses the same shapes: a default observer (zero facing, the
    // non-dict/garbage case) and a pos-only observer with zero facing.
    const TArray<FCrimeObserver> Observers = {
        FCrimeObserver(),                                    // "not a dict" -> zero facing, can't see
        FCrimeObserver(FVector(-5, 0, 0), FVector(1, 0, 0)), // sees
        FCrimeObserver(FVector(-3, 0, 0), FVector::ZeroVector), // missing facing -> can't see
    };
    TestEqual(TEXT("1 valid witness"), FCrimeWitness::CountWitnesses(
        FVector::ZeroVector, Observers, PedRange, PedFov), 1);
    return true;
}

// --- heat_for_crime --------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessHeatZeroWitnessesTest,
    "GTC.Systems.Wanted.CrimeWitness.HeatZeroWitnessesIsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessHeatZeroWitnessesTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("0 witnesses -> 0"), FCrimeWitness::HeatForCrime(10.0, 0), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessHeatOneWitnessTest,
    "GTC.Systems.Wanted.CrimeWitness.HeatOneWitness",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessHeatOneWitnessTest::RunTest(const FString& Parameters)
{
    // base * (1 - 0.5^1) = 10 * 0.5 = 5.0
    TestEqual(TEXT("1 witness -> 5"), FCrimeWitness::HeatForCrime(10.0, 1), 5.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessHeatTwoWitnessesTest,
    "GTC.Systems.Wanted.CrimeWitness.HeatTwoWitnessesDiminishing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessHeatTwoWitnessesTest::RunTest(const FString& Parameters)
{
    // base * (1 - 0.5^2) = 10 * 0.75 = 7.5; more than one but less than double.
    TestEqual(TEXT("2 witnesses -> 7.5"), FCrimeWitness::HeatForCrime(10.0, 2), 7.5, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessHeatSaturatesTest,
    "GTC.Systems.Wanted.CrimeWitness.HeatSaturatesBelowBase",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessHeatSaturatesTest::RunTest(const FString& Parameters)
{
    // Many witnesses approach but never reach base_heat.
    const double H = FCrimeWitness::HeatForCrime(10.0, 50);
    TestTrue(TEXT("h < 10"), H < 10.0);
    TestTrue(TEXT("h > 9.99"), H > 9.99);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessHeatMonotonicTest,
    "GTC.Systems.Wanted.CrimeWitness.HeatMonotonicIncreasing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessHeatMonotonicTest::RunTest(const FString& Parameters)
{
    const double H1 = FCrimeWitness::HeatForCrime(10.0, 1);
    const double H2 = FCrimeWitness::HeatForCrime(10.0, 2);
    const double H3 = FCrimeWitness::HeatForCrime(10.0, 3);
    TestTrue(TEXT("h1 < h2"), H1 < H2);
    TestTrue(TEXT("h2 < h3"), H2 < H3);
    TestTrue(TEXT("h3 < 10"), H3 < 10.0);
    return true;
}

// --- stateful report timer -------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessReportNotDoneTest,
    "GTC.Systems.Wanted.CrimeWitness.ReportNotDoneBeforeDelay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessReportNotDoneTest::RunTest(const FString& Parameters)
{
    FCrimeWitness R(3.0);
    R.Tick(1.0);
    TestFalse(TEXT("not reported yet"), R.IsReported());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessProgressRampsTest,
    "GTC.Systems.Wanted.CrimeWitness.ReportProgressRamps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessProgressRampsTest::RunTest(const FString& Parameters)
{
    FCrimeWitness R(4.0);
    const double P0 = R.Progress();
    R.Tick(1.0);
    const double P1 = R.Progress(); // 0.25
    R.Tick(1.0);
    const double P2 = R.Progress(); // 0.5
    TestEqual(TEXT("p0 == 0"), P0, 0.0, Eps);
    TestEqual(TEXT("p1 == 0.25"), P1, 0.25, Eps);
    TestEqual(TEXT("p2 == 0.5"), P2, 0.5, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessCompletesClampsTest,
    "GTC.Systems.Wanted.CrimeWitness.ReportCompletesAndClamps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessCompletesClampsTest::RunTest(const FString& Parameters)
{
    // Overshooting the delay still lands exactly at reported / progress 1.0.
    FCrimeWitness R(3.0);
    R.Tick(10.0);
    TestTrue(TEXT("reported"), R.IsReported());
    TestEqual(TEXT("progress == 1"), R.Progress(), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessSilenceStaysUnreportedTest,
    "GTC.Systems.Wanted.CrimeWitness.SilenceBeforeCompletionStaysUnreported",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessSilenceStaysUnreportedTest::RunTest(const FString& Parameters)
{
    FCrimeWitness R(3.0);
    R.Tick(1.0);
    R.Silence();
    R.Tick(5.0); // more time, but the witness is gone
    TestFalse(TEXT("not reported"), R.IsReported());
    TestEqual(TEXT("progress == 0"), R.Progress(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessNegativeDeltaTest,
    "GTC.Systems.Wanted.CrimeWitness.NegativeDeltaIgnored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessNegativeDeltaTest::RunTest(const FString& Parameters)
{
    FCrimeWitness R(3.0);
    R.Tick(1.0);
    R.Tick(-5.0);
    TestEqual(TEXT("progress == 1/3"), R.Progress(), 1.0 / 3.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessResetRearmsTest,
    "GTC.Systems.Wanted.CrimeWitness.ResetRearms",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessResetRearmsTest::RunTest(const FString& Parameters)
{
    FCrimeWitness R(2.0);
    R.Tick(2.0);
    const bool bWasReported = R.IsReported();
    R.Reset();
    TestTrue(TEXT("was reported"), bWasReported);
    TestFalse(TEXT("not reported after reset"), R.IsReported());
    TestEqual(TEXT("progress == 0"), R.Progress(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessResetClearsSilenceTest,
    "GTC.Systems.Wanted.CrimeWitness.ResetClearsSilence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessResetClearsSilenceTest::RunTest(const FString& Parameters)
{
    FCrimeWitness R(2.0);
    R.Silence();
    R.Reset();
    R.Tick(2.0);
    TestTrue(TEXT("reported after reset+tick"), R.IsReported());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessZeroDelayTest,
    "GTC.Systems.Wanted.CrimeWitness.ZeroDelayReportsImmediately",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeWitnessZeroDelayTest::RunTest(const FString& Parameters)
{
    FCrimeWitness R(0.0);
    TestTrue(TEXT("reported immediately"), R.IsReported());
    TestEqual(TEXT("progress == 1"), R.Progress(), 1.0, Eps);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
