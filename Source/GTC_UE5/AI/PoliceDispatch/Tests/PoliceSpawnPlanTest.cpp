// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../PoliceSpawnPlan.h"
#include "../PoliceDispatch.h"
#include "../PoliceEscalation.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FPoliceSpawnPlan — the Wave-3 composition that stitches the already-
 * tested dispatch/escalation primitives into one concrete spawn list. This is the
 * rule-#3 testable unit standing in for the (engine-bound) spawner actor: it
 * asserts the plan's STRUCTURE (count, ring geometry, unit mix, determinism)
 * against the same primitives, so the actor adapter can stay a thin shell.
 *
 * Prefix GTC.AI.PoliceDispatch.PoliceSpawnPlan. PlanarOffset is the pure-core
 * frame (metres, XZ planar, Y == 0); the Unreal world remap is the adapter's job
 * and is NOT tested here.
 */

namespace
{
    double PlanarLen(const FVector& V)
    {
        return FVector2D(V.X, V.Z).Size(); // pure-core planar is XZ (Y ignored)
    }
}

// --- empty when not wanted --------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoliceSpawnPlanEmptyTest,
    "GTC.AI.PoliceDispatch.PoliceSpawnPlan.EmptyWhenNotWanted",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoliceSpawnPlanEmptyTest::RunTest(const FString& Parameters)
{
    FRandomStream Rng(1234);
    // 0 stars: ResponseUnits is empty AND SpawnCount is zero -> no slots.
    TestEqual(TEXT("zero stars empty plan"),
        FPoliceSpawnPlan::Build(0, 0, 12, 8, 50.0, Rng).Num(), 0);
    // Wanted, but the field is already full -> zero deficit -> no slots.
    TestEqual(TEXT("satisfied wave empty"),
        FPoliceSpawnPlan::Build(2, 2, 12, 8, 50.0, Rng).Num(), 0);

    return true;
}

// --- wave size matches the dispatch deficit ---------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoliceSpawnPlanCountTest,
    "GTC.AI.PoliceDispatch.PoliceSpawnPlan.CountMatchesDispatch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoliceSpawnPlanCountTest::RunTest(const FString& Parameters)
{
    FRandomStream Rng(99);
    // 5 stars wants 8; none alive; ample wave cap -> 8 slots (== SpawnCount).
    const int32 Expected = FPoliceDispatch::SpawnCount(5, 0, 12, 99);
    TestEqual(TEXT("wave fills the deficit"),
        FPoliceSpawnPlan::Build(5, 0, 12, 99, 50.0, Rng).Num(), Expected);
    // Wave cap throttles the trickle: wants 8, none alive, cap 3 -> 3 slots.
    TestEqual(TEXT("wave respects the per-wave cap"),
        FPoliceSpawnPlan::Build(5, 0, 12, 3, 50.0, Rng).Num(), 3);

    return true;
}

// --- ring geometry & unit mix -----------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoliceSpawnPlanGeometryTest,
    "GTC.AI.PoliceDispatch.PoliceSpawnPlan.RingGeometryAndMix",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoliceSpawnPlanGeometryTest::RunTest(const FString& Parameters)
{
    FRandomStream Rng(7);
    const double Radius = 50.0;
    const double Jitter = FPoliceSpawnPlan::DefaultRadialJitterMeters;
    const TArray<EPoliceUnitType> Mix = FPoliceEscalation::ResponseUnits(5);
    const TArray<FPoliceSpawnSlot> Plan = FPoliceSpawnPlan::Build(5, 0, 12, 99, Radius, Rng);

    TestTrue(TEXT("plan is non-empty"), Plan.Num() > 0);
    for (const FPoliceSpawnSlot& Slot : Plan)
    {
        // Planar offset: Y carried as 0, length within the radius +/- radial jitter.
        TestEqual(TEXT("offset is planar (Y==0)"), Slot.PlanarOffset.Y, 0.0, Eps);
        const double Len = PlanarLen(Slot.PlanarOffset);
        TestTrue(TEXT("offset sits within the jittered ring"),
            Len >= Radius - Jitter - Eps && Len <= Radius + Jitter + Eps);
        // Every dealt unit type belongs to this heat's response mix.
        TestTrue(TEXT("unit type is in the escalation mix"), Mix.Contains(Slot.UnitType));
    }

    return true;
}

// --- deterministic & round-robin unit dealing -------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoliceSpawnPlanDeterminismTest,
    "GTC.AI.PoliceDispatch.PoliceSpawnPlan.DeterministicAndRoundRobin",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoliceSpawnPlanDeterminismTest::RunTest(const FString& Parameters)
{
    // Same seed -> identical plan (positions and types), so a seeded run is stable.
    FRandomStream A(42);
    FRandomStream B(42);
    const TArray<FPoliceSpawnSlot> PlanA = FPoliceSpawnPlan::Build(4, 1, 12, 99, 60.0, A);
    const TArray<FPoliceSpawnSlot> PlanB = FPoliceSpawnPlan::Build(4, 1, 12, 99, 60.0, B);
    TestEqual(TEXT("same seed same wave size"), PlanA.Num(), PlanB.Num());
    bool bSame = PlanA.Num() == PlanB.Num();
    for (int32 i = 0; bSame && i < PlanA.Num(); ++i)
    {
        bSame = PlanA[i].UnitType == PlanB[i].UnitType
            && PlanA[i].PlanarOffset.Equals(PlanB[i].PlanarOffset, Eps);
    }
    TestTrue(TEXT("same seed identical plan"), bSame);

    // When the wave is no larger than the mix, slot i deals the i-th mix entry in
    // order. 3 stars: DesiredUnits == 4, mix has 4 entries; cap 4, none alive -> 4.
    const TArray<EPoliceUnitType> Mix = FPoliceEscalation::ResponseUnits(3);
    FRandomStream C(1);
    const TArray<FPoliceSpawnSlot> Plan3 = FPoliceSpawnPlan::Build(3, 0, 12, 4, 50.0, C);
    TestEqual(TEXT("wave equals mix size"), Plan3.Num(), Mix.Num());
    bool bOrdered = Plan3.Num() == Mix.Num();
    for (int32 i = 0; bOrdered && i < Plan3.Num(); ++i)
    {
        bOrdered = Plan3[i].UnitType == Mix[i];
    }
    TestTrue(TEXT("units dealt in mix order"), bOrdered);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
