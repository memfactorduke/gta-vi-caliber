// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../PedestrianTraffic.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Parity tests for FPedestrianTraffic, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_pedestrian_traffic.gd. Tolerance mirrors the oracle's
 * is_equal_approx (Eps = 1e-4) for scalar/vector compares; the oracle's explicit
 * `< 0.001` / `> x` comparisons assert with TrafficEps / TestTrue as written.
 *
 * Scenarios lie on the XZ plane: a pedestrian near the origin, cars driving along
 * +X or +Z. Grouped by the oracle's method sections (prefix
 * GTC.NPC.Steering.PedestrianTraffic). The Godot {pos, vel} dictionaries map to
 * FPedestrianTraffic::FCar.
 *
 * PURE-CORE boundary: only the per-agent traffic math. Live-agent wiring
 * (neighbour gathering, blending dodge into combine, curb gating) is the deferred
 * Wave-3 adapter — NOT tested here.
 */

namespace
{
    using FCar = FPedestrianTraffic::FCar;

    // The oracle's explicit magnitude asserts use `< 0.001`.
    constexpr double TrafficEps = 1e-3;
}

// --- is_closing -------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPedestrianTrafficIsClosingTest,
    "GTC.NPC.Steering.PedestrianTraffic.IsClosing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPedestrianTrafficIsClosingTest::RunTest(const FString& Parameters)
{
    // test_closing_when_car_drives_at_pedestrian
    TestTrue(TEXT("closing when car drives at pedestrian"),
        FPedestrianTraffic::IsClosing(
            FVector::ZeroVector, FVector::ZeroVector, FVector(-10, 0, 0), FVector(8, 0, 0)));
    // test_not_closing_when_car_drives_away
    TestFalse(TEXT("not closing when car drives away"),
        FPedestrianTraffic::IsClosing(
            FVector::ZeroVector, FVector::ZeroVector, FVector(10, 0, 0), FVector(8, 0, 0)));

    return true;
}

// --- time_to_closest_approach -----------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPedestrianTrafficTtcaTest,
    "GTC.NPC.Steering.PedestrianTraffic.TimeToClosestApproach",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPedestrianTrafficTtcaTest::RunTest(const FString& Parameters)
{
    // test_ttca_head_on_is_positive
    TestEqual(TEXT("head-on is positive"),
        FPedestrianTraffic::TimeToClosestApproach(
            FVector::ZeroVector, FVector::ZeroVector, FVector(-16, 0, 0), FVector(8, 0, 0)),
        2.0, Eps);
    // test_ttca_zero_when_receding
    TestEqual(TEXT("zero when receding"),
        FPedestrianTraffic::TimeToClosestApproach(
            FVector::ZeroVector, FVector::ZeroVector, FVector(4, 0, 0), FVector(8, 0, 0)),
        0.0, Eps);
    // test_ttca_zero_when_parallel_same_velocity
    TestEqual(TEXT("zero when parallel same velocity"),
        FPedestrianTraffic::TimeToClosestApproach(
            FVector::ZeroVector, FVector(1, 0, 0), FVector(0, 0, 5), FVector(1, 0, 0)),
        0.0, Eps);

    return true;
}

// --- closest_approach_distance ----------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPedestrianTrafficClosestDistanceTest,
    "GTC.NPC.Steering.PedestrianTraffic.ClosestApproachDistance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPedestrianTrafficClosestDistanceTest::RunTest(const FString& Parameters)
{
    // test_closest_distance_for_passing_car
    TestEqual(TEXT("passing car nearest approach ~3m"),
        FPedestrianTraffic::ClosestApproachDistance(
            FVector::ZeroVector, FVector::ZeroVector, FVector(-20, 0, 3), FVector(10, 0, 0)),
        3.0, Eps);
    // test_closest_distance_zero_for_direct_hit
    TestTrue(TEXT("direct hit ~0"),
        FPedestrianTraffic::ClosestApproachDistance(
            FVector::ZeroVector, FVector::ZeroVector, FVector(-20, 0, 0), FVector(10, 0, 0))
            < TrafficEps);

    return true;
}

// --- car_threat -------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPedestrianTrafficCarThreatTest,
    "GTC.NPC.Steering.PedestrianTraffic.CarThreat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPedestrianTrafficCarThreatTest::RunTest(const FString& Parameters)
{
    // test_threat_high_for_imminent_direct_hit
    TestTrue(TEXT("high for imminent direct hit"),
        FPedestrianTraffic::CarThreat(
            FVector::ZeroVector, FVector::ZeroVector, FVector(-8, 0, 0), FVector(8, 0, 0),
            4.0, 3.0) > 0.6);
    // test_threat_zero_for_receding_car
    TestEqual(TEXT("zero for receding car"),
        FPedestrianTraffic::CarThreat(
            FVector::ZeroVector, FVector::ZeroVector, FVector(4, 0, 0), FVector(8, 0, 0),
            4.0, 3.0),
        0.0, Eps);
    // test_threat_zero_when_miss_exceeds_radius
    TestEqual(TEXT("zero when miss exceeds radius"),
        FPedestrianTraffic::CarThreat(
            FVector::ZeroVector, FVector::ZeroVector, FVector(-20, 0, 6), FVector(10, 0, 0),
            4.0, 5.0),
        0.0, Eps);
    // test_threat_zero_beyond_time_horizon
    TestEqual(TEXT("zero beyond time horizon"),
        FPedestrianTraffic::CarThreat(
            FVector::ZeroVector, FVector::ZeroVector, FVector(-100, 0, 0), FVector(10, 0, 0),
            4.0, 3.0),
        0.0, Eps);
    // test_threat_grows_as_car_nears
    {
        const double Far = FPedestrianTraffic::CarThreat(
            FVector::ZeroVector, FVector::ZeroVector, FVector(-20, 0, 0), FVector(8, 0, 0),
            5.0, 4.0);
        const double Near = FPedestrianTraffic::CarThreat(
            FVector::ZeroVector, FVector::ZeroVector, FVector(-8, 0, 0), FVector(8, 0, 0),
            5.0, 4.0);
        TestTrue(TEXT("grows as car nears"), Near > Far);
    }

    return true;
}

// --- nearest_threat ---------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPedestrianTrafficNearestThreatTest,
    "GTC.NPC.Steering.PedestrianTraffic.NearestThreat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPedestrianTrafficNearestThreatTest::RunTest(const FString& Parameters)
{
    // test_nearest_threat_picks_most_dangerous
    {
        const TArray<FCar> Cars = {
            {FVector(-40, 0, 0), FVector(8, 0, 0)},   // far, low threat
            {FVector(-8, 0, 0), FVector(10, 0, 0)},   // close, high threat
        };
        const FPedestrianTraffic::FThreat Best =
            FPedestrianTraffic::NearestThreat(FVector::ZeroVector, FVector::ZeroVector, Cars, 5.0, 4.0);
        TestEqual(TEXT("picks most dangerous index"), Best.Index, 1);
        TestTrue(TEXT("picks most dangerous threat"), Best.Threat > 0.0);
    }
    // test_nearest_threat_empty_when_no_cars
    {
        const FPedestrianTraffic::FThreat Best = FPedestrianTraffic::NearestThreat(
            FVector::ZeroVector, FVector::ZeroVector, TArray<FCar>(), 5.0, 4.0);
        TestEqual(TEXT("empty index"), Best.Index, -1);
        TestEqual(TEXT("empty threat"), Best.Threat, 0.0, Eps);
    }
    // test_nearest_threat_ignores_receding_traffic
    {
        const TArray<FCar> Cars = {{FVector(4, 0, 0), FVector(8, 0, 0)}};
        const FPedestrianTraffic::FThreat Best =
            FPedestrianTraffic::NearestThreat(FVector::ZeroVector, FVector::ZeroVector, Cars, 5.0, 4.0);
        TestEqual(TEXT("ignores receding traffic"), Best.Index, -1);
    }

    return true;
}

// --- dodge_velocity ---------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPedestrianTrafficDodgeTest,
    "GTC.NPC.Steering.PedestrianTraffic.DodgeVelocity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPedestrianTrafficDodgeTest::RunTest(const FString& Parameters)
{
    // test_dodge_is_perpendicular_to_car_heading
    {
        const FVector Dodge = FPedestrianTraffic::DodgeVelocity(
            FVector(0, 0, 2), FVector(-10, 0, 0), FVector(10, 0, 0), 3.0);
        TestTrue(TEXT("perpendicular to car heading"),
            FMath::Abs(Dodge.X) < TrafficEps && Dodge.Z > 0.0
                && FMath::Abs(Dodge.Size() - 3.0) < Eps);
    }
    // test_dodge_picks_pedestrians_side
    {
        const FVector Dodge = FPedestrianTraffic::DodgeVelocity(
            FVector(0, 0, -2), FVector(-10, 0, 0), FVector(10, 0, 0), 3.0);
        TestTrue(TEXT("picks pedestrian's side"), Dodge.Z < 0.0);
    }
    // test_dodge_pushes_away_from_stopped_car
    {
        const FVector Dodge = FPedestrianTraffic::DodgeVelocity(
            FVector::ZeroVector, FVector(-2, 0, 0), FVector::ZeroVector, 3.0);
        TestTrue(TEXT("pushes away from stopped car"),
            Dodge.X > 0.0 && FMath::Abs(Dodge.Size() - 3.0) < Eps);
    }

    return true;
}

// --- safe_to_cross ----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPedestrianTrafficSafeToCrossTest,
    "GTC.NPC.Steering.PedestrianTraffic.SafeToCross",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPedestrianTrafficSafeToCrossTest::RunTest(const FString& Parameters)
{
    // test_safe_to_cross_clear_road
    TestTrue(TEXT("clear road"),
        FPedestrianTraffic::SafeToCross(FVector::ZeroVector, TArray<FCar>(), 3.0, 3.0));
    // test_unsafe_to_cross_with_incoming_car
    {
        const TArray<FCar> Cars = {{FVector(-12, 0, 0), FVector(10, 0, 0)}};
        TestFalse(TEXT("unsafe with incoming car"),
            FPedestrianTraffic::SafeToCross(FVector::ZeroVector, Cars, 3.0, 3.0));
    }
    // test_safe_to_cross_when_car_far_in_time
    {
        const TArray<FCar> Cars = {{FVector(-100, 0, 0), FVector(10, 0, 0)}};
        TestTrue(TEXT("safe when car far in time"),
            FPedestrianTraffic::SafeToCross(FVector::ZeroVector, Cars, 3.0, 3.0));
    }
    // test_safe_to_cross_when_car_on_far_lane
    {
        const TArray<FCar> Cars = {{FVector(-12, 0, 8), FVector(10, 0, 0)}};
        TestTrue(TEXT("safe when car on far lane"),
            FPedestrianTraffic::SafeToCross(FVector::ZeroVector, Cars, 3.0, 3.0));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
