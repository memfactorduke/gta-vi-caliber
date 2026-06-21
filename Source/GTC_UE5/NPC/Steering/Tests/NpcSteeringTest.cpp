// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcSteering.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Parity tests for FNpcSteering, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_npc_steering.gd. Tolerance mirrors the oracle's
 * is_equal_approx / explicit 0.001 length compares (Eps = 1e-4 for vector
 * equality; SpeedEps = 1e-3 for the speed-magnitude asserts the oracle writes as
 * `absf(... ) < 0.001`).
 *
 * Grouped by the oracle's method sections (prefix GTC.NPC.Steering.NpcSteering).
 *
 * PURE-CORE boundary: only the per-agent steering math. Crowd integration
 * (CharacterMovementComponent, gravity, neighbour gathering) is the deferred
 * Wave-3 adapter — NOT tested here.
 */

namespace
{
    // The oracle asserts speed magnitudes with `absf(len - x) < 0.001`.
    using GtcTest::SpeedEps;
}

// --- seek -------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcSteeringSeekTest,
    "GTC.NPC.Steering.NpcSteering.Seek",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcSteeringSeekTest::RunTest(const FString& Parameters)
{
    // test_seek_points_at_target
    TestTrue(TEXT("seek points at target"),
        FNpcSteering::Seek(FVector::ZeroVector, FVector(10, 0, 0), 4.0)
            .Equals(FVector(4, 0, 0), Eps));
    // test_seek_ignores_height
    TestTrue(TEXT("seek ignores height"),
        FNpcSteering::Seek(FVector::ZeroVector, FVector(0, 3, 0), 4.0) == FVector::ZeroVector);
    // test_seek_zero_at_target
    TestTrue(TEXT("seek zero at target"),
        FNpcSteering::Seek(FVector(5, 0, 5), FVector(5, 0, 5), 4.0) == FVector::ZeroVector);

    return true;
}

// --- arrive -----------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcSteeringArriveTest,
    "GTC.NPC.Steering.NpcSteering.Arrive",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcSteeringArriveTest::RunTest(const FString& Parameters)
{
    // test_arrive_full_speed_when_far
    {
        const FVector V = FNpcSteering::Arrive(FVector::ZeroVector, FVector(100, 0, 0), 4.0, 5.0);
        TestTrue(TEXT("full speed when far"), FMath::Abs(V.Size() - 4.0) < SpeedEps);
    }
    // test_arrive_ramps_down_in_slow_radius
    {
        const FVector V = FNpcSteering::Arrive(FVector::ZeroVector, FVector(2.5, 0, 0), 4.0, 5.0);
        TestTrue(TEXT("ramps down in slow radius"), FMath::Abs(V.Size() - 2.0) < SpeedEps);
    }
    // test_arrive_stops_at_target
    TestTrue(TEXT("stops at target"),
        FNpcSteering::Arrive(FVector::ZeroVector, FVector(0.1, 0, 0), 4.0, 5.0)
            == FVector::ZeroVector);

    return true;
}

// --- separation -------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcSteeringSeparationTest,
    "GTC.NPC.Steering.NpcSteering.Separation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcSteeringSeparationTest::RunTest(const FString& Parameters)
{
    // test_separation_pushes_away_from_neighbor
    {
        const FVector V =
            FNpcSteering::Separation(FVector::ZeroVector, {FVector(1, 0, 0)}, 2.0, 4.0);
        TestTrue(TEXT("pushes away from neighbor"), V.X < 0.0 && FMath::Abs(V.Z) < SpeedEps);
    }
    // test_separation_ignores_distant_neighbors
    TestTrue(TEXT("ignores distant neighbors"),
        FNpcSteering::Separation(FVector::ZeroVector, {FVector(50, 0, 0)}, 2.0, 4.0)
            == FVector::ZeroVector);

    return true;
}

// --- combine ----------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcSteeringCombineTest,
    "GTC.NPC.Steering.NpcSteering.Combine",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcSteeringCombineTest::RunTest(const FString& Parameters)
{
    // test_combine_clamps_to_max_speed
    {
        const FVector V = FNpcSteering::Combine(
            {FVector(10, 0, 0), FVector(0, 0, 10)}, {1.0, 1.0}, 4.0);
        TestTrue(TEXT("clamps to max speed"), FMath::Abs(V.Size() - 4.0) < SpeedEps);
    }

    return true;
}

// --- advance_waypoint -------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcSteeringAdvanceWaypointTest,
    "GTC.NPC.Steering.NpcSteering.AdvanceWaypoint",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcSteeringAdvanceWaypointTest::RunTest(const FString& Parameters)
{
    // test_advance_waypoint_skips_reached
    {
        const TArray<FVector> Path = {FVector(0, 0, 0), FVector(1, 0, 0), FVector(10, 0, 0)};
        TestEqual(TEXT("skips reached -> index 2"),
            FNpcSteering::AdvanceWaypoint(FVector(0, 0, 0), Path, 0, 1.5), 2);
    }
    // test_advance_waypoint_holds_at_last
    {
        const TArray<FVector> Path = {FVector(0, 0, 0), FVector(1, 0, 0)};
        TestEqual(TEXT("holds at last"),
            FNpcSteering::AdvanceWaypoint(FVector(1, 0, 0), Path, 1, 1.5), 1);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
