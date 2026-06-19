// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../PoliceDispatch.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Parity tests for FPoliceDispatch, mapped 1:1 from the the reference oracle
 * game/tests/unit/test_police_dispatch.gd. Tolerance mirrors the oracle's
 * is_equal_approx (Eps = 1e-4); integer counts and the despawn predicate assert
 * exactly. Grouped into the oracle's 5 sections (prefix
 * GTC.AI.PoliceDispatch.PoliceDispatch).
 *
 * PURE-CORE boundary: unit spawning / freeing / world placement are the deferred
 * Wave-3 adapter and are NOT tested.
 */

namespace
{
    // CENTER := Vector3(10, 2, -4) from the oracle, expressed in UE planar XZ
    // (verbatim component order, NO Z-up remap).
    const FVector GtcDispatchCenter(10.0, 2.0, -4.0);
}

// --- desired_units ----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoliceDispatchDesiredUnitsTest,
    "GTC.AI.PoliceDispatch.PoliceDispatch.DesiredUnits",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoliceDispatchDesiredUnitsTest::RunTest(const FString& Parameters)
{
    // test_desired_units_follows_heat: UNITS_PER_STAR == [0,1,2,4,6,8].
    TestEqual(TEXT("zero stars zero units"), FPoliceDispatch::DesiredUnits(0, 99), 0);
    TestEqual(TEXT("five stars eight units"), FPoliceDispatch::DesiredUnits(5, 99), 8);
    // test_desired_units_capped_by_budget
    TestEqual(TEXT("capped by budget"), FPoliceDispatch::DesiredUnits(5, 3), 3);

    return true;
}

// --- spawn_count ------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoliceDispatchSpawnCountTest,
    "GTC.AI.PoliceDispatch.PoliceDispatch.SpawnCount",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoliceDispatchSpawnCountTest::RunTest(const FString& Parameters)
{
    // test_spawn_count_fills_deficit: 5 stars wants 8; 2 alive; wave cap 3 -> 3.
    TestEqual(TEXT("fills deficit"), FPoliceDispatch::SpawnCount(5, 2, 12, 3), 3);
    // test_spawn_count_zero_when_satisfied
    TestEqual(TEXT("zero when satisfied"), FPoliceDispatch::SpawnCount(2, 2, 12, 4), 0);
    // test_spawn_count_never_negative
    TestEqual(TEXT("never negative"), FPoliceDispatch::SpawnCount(1, 6, 12, 4), 0);
    // test_spawn_count_respects_budget: wants 8 but budget 5, none alive -> 5.
    TestEqual(TEXT("respects budget"), FPoliceDispatch::SpawnCount(5, 0, 5, 99), 5);

    return true;
}

// --- ring_angle -------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoliceDispatchRingAngleTest,
    "GTC.AI.PoliceDispatch.PoliceDispatch.RingAngle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoliceDispatchRingAngleTest::RunTest(const FString& Parameters)
{
    // test_ring_angle_evenly_slices_without_jitter: 4 spawns, no jitter -> 0,pi/2,pi,3pi/2.
    for (int32 i = 0; i < 4; ++i)
    {
        TestEqual(*FString::Printf(TEXT("even slice %d"), i),
            FPoliceDispatch::RingAngle(i, 4, 0.5, 0.0), double(i) * UE_DOUBLE_TWO_PI / 4.0, Eps);
    }
    // test_ring_angle_jitter_stays_in_slice
    {
        const double Slice = UE_DOUBLE_TWO_PI / 6.0;
        const double Base = 2.0 * Slice;
        const double Lo = FPoliceDispatch::RingAngle(2, 6, 0.0, 1.0);
        const double Hi = FPoliceDispatch::RingAngle(2, 6, 1.0, 1.0);
        TestEqual(TEXT("jitter low edge"), Lo, Base - Slice * 0.5, Eps);
        TestEqual(TEXT("jitter high edge"), Hi, Base + Slice * 0.5, Eps);
    }

    return true;
}

// --- ring_position ----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoliceDispatchRingPositionTest,
    "GTC.AI.PoliceDispatch.PoliceDispatch.RingPosition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoliceDispatchRingPositionTest::RunTest(const FString& Parameters)
{
    // test_ring_position_sits_on_radius
    {
        const FVector P = FPoliceDispatch::RingPosition(GtcDispatchCenter, 40.0, 0.0, 0.5, 0.0);
        const double PlanarLen =
            FVector2D(P.X - GtcDispatchCenter.X, P.Z - GtcDispatchCenter.Z).Size();
        TestEqual(TEXT("sits on radius"), PlanarLen, 40.0, Eps);
    }
    // test_ring_position_keeps_center_height
    {
        const FVector P = FPoliceDispatch::RingPosition(GtcDispatchCenter, 40.0, 1.2, 0.7, 6.0);
        TestEqual(TEXT("keeps center height"), P.Y, GtcDispatchCenter.Y, Eps);
    }
    // test_ring_position_radial_jitter_bounded: u_radius extremes shift +/- radial_jitter.
    {
        const FVector Near = FPoliceDispatch::RingPosition(GtcDispatchCenter, 40.0, 0.0, 0.0, 6.0);
        const FVector Far = FPoliceDispatch::RingPosition(GtcDispatchCenter, 40.0, 0.0, 1.0, 6.0);
        const double Dn = FVector2D(Near.X - GtcDispatchCenter.X, Near.Z - GtcDispatchCenter.Z).Size();
        const double Df = FVector2D(Far.X - GtcDispatchCenter.X, Far.Z - GtcDispatchCenter.Z).Size();
        TestEqual(TEXT("near jitter"), Dn, 34.0, Eps);
        TestEqual(TEXT("far jitter"), Df, 46.0, Eps);
    }

    return true;
}

// --- should_despawn ---------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoliceDispatchShouldDespawnTest,
    "GTC.AI.PoliceDispatch.PoliceDispatch.ShouldDespawn",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoliceDispatchShouldDespawnTest::RunTest(const FString& Parameters)
{
    // test_despawn_when_no_longer_wanted: heat cleared -> recall even adjacent unit.
    TestTrue(TEXT("despawn when no longer wanted"),
        FPoliceDispatch::ShouldDespawn(0, 1.0, 160.0));
    // test_keep_nearby_unit_while_wanted
    TestFalse(TEXT("keep nearby unit while wanted"),
        FPoliceDispatch::ShouldDespawn(3, 30.0, 160.0));
    // test_despawn_unit_that_falls_too_far_behind
    TestTrue(TEXT("despawn unit too far behind"),
        FPoliceDispatch::ShouldDespawn(3, 200.0, 160.0));

    return true;
}

#endif // WITH_AUTOMATION_TESTS
