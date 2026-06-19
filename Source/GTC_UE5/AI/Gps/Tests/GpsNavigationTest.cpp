// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../GpsNavigation.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Parity tests for FGpsNavigation, mapped 1:1 from the the reference oracle
 * game/tests/unit/test_gps_navigation.gd. Tolerance mirrors the oracle's
 * is_equal_approx (Eps = 1e-4) for float/vector compares; enum/bool/int equality
 * and the ETA infinity guard (!FMath::IsFinite) assert exactly.
 *
 * Grouped by the oracle's method sections (prefix GTC.AI.Gps). Route builders
 * mirror the oracle's _straight / _l_left / _l_right.
 *
 * PURE-CORE boundary: Recast/navmesh path GENERATION is the deferred Wave-3
 * adapter — the model only consumes a caller-supplied polyline. NOT tested.
 */

namespace
{
    // A straight 20m route along +x with a midpoint waypoint.
    TArray<FVector> Straight()
    {
        return {FVector(0, 0, 0), FVector(10, 0, 0), FVector(20, 0, 0)};
    }

    // +x for 10m, then turn toward -z for 10m. Cross > 0 => left turn.
    TArray<FVector> LLeft()
    {
        return {FVector(0, 0, 0), FVector(10, 0, 0), FVector(10, 0, -10)};
    }

    // +x for 10m, then turn toward +z for 10m. Cross < 0 => right turn.
    TArray<FVector> LRight()
    {
        return {FVector(0, 0, 0), FVector(10, 0, 0), FVector(10, 0, 10)};
    }
}

// --- route_length -----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGpsNavigationRouteLengthTest,
    "GTC.AI.Gps.RouteLength",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGpsNavigationRouteLengthTest::RunTest(const FString& Parameters)
{
    // test_route_length_straight
    TestEqual(TEXT("straight length"), FGpsNavigation::RouteLength(Straight()), 20.0, Eps);
    // test_route_length_l_shaped
    TestEqual(TEXT("l-shaped length"), FGpsNavigation::RouteLength(LLeft()), 20.0, Eps);
    // test_route_length_empty_and_single
    TestEqual(TEXT("empty length"), FGpsNavigation::RouteLength(TArray<FVector>()), 0.0, Eps);
    TestEqual(TEXT("single length"), FGpsNavigation::RouteLength({FVector(3, 0, 4)}), 0.0, Eps);
    // test_route_length_ignores_height
    {
        const TArray<FVector> Route = {FVector(0, 5, 0), FVector(10, -2, 0)};
        TestEqual(TEXT("ignores height"), FGpsNavigation::RouteLength(Route), 10.0, Eps);
    }

    return true;
}

// --- nearest_segment --------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGpsNavigationNearestSegmentTest,
    "GTC.AI.Gps.NearestSegment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGpsNavigationNearestSegmentTest::RunTest(const FString& Parameters)
{
    // test_nearest_segment_picks_closest
    TestEqual(TEXT("near first leg -> seg 0"),
        FGpsNavigation::NearestSegment(FVector(3, 0, 1), Straight()), 0);
    TestEqual(TEXT("near second leg -> seg 1"),
        FGpsNavigation::NearestSegment(FVector(17, 0, -1), Straight()), 1);
    TestEqual(TEXT("on vertical leg -> seg 1"),
        FGpsNavigation::NearestSegment(FVector(10, 0, -7), LLeft()), 1);

    return true;
}

// --- distance_remaining -----------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGpsNavigationDistanceRemainingTest,
    "GTC.AI.Gps.DistanceRemaining",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGpsNavigationDistanceRemainingTest::RunTest(const FString& Parameters)
{
    // test_distance_remaining_at_start
    TestEqual(TEXT("at start"),
        FGpsNavigation::DistanceRemaining(FVector(0, 0, 0), Straight()), 20.0, Eps);
    // test_distance_remaining_mid
    TestEqual(TEXT("mid"),
        FGpsNavigation::DistanceRemaining(FVector(14, 0, 2), Straight()), 6.0, Eps);
    // test_distance_remaining_near_end
    TestEqual(TEXT("near end"),
        FGpsNavigation::DistanceRemaining(FVector(19.5, 0, 0), Straight()), 0.5, Eps);
    // test_distance_remaining_l_route
    TestEqual(TEXT("l route at corner"),
        FGpsNavigation::DistanceRemaining(FVector(10, 0, 0), LLeft()), 10.0, Eps);

    return true;
}

// --- progress ---------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGpsNavigationProgressTest,
    "GTC.AI.Gps.Progress",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGpsNavigationProgressTest::RunTest(const FString& Parameters)
{
    // test_progress_at_start
    TestEqual(TEXT("at start"),
        FGpsNavigation::Progress(FVector(0, 0, 0), Straight()), 0.0, Eps);
    // test_progress_mid
    TestEqual(TEXT("mid"),
        FGpsNavigation::Progress(FVector(10, 0, 0), Straight()), 0.5, Eps);
    // test_progress_at_destination
    TestEqual(TEXT("at destination"),
        FGpsNavigation::Progress(FVector(20, 0, 0), Straight()), 1.0, Eps);
    // test_progress_degenerate_route
    TestEqual(TEXT("degenerate route"),
        FGpsNavigation::Progress(FVector(0, 0, 0), {FVector::ZeroVector}), 1.0, Eps);

    return true;
}

// --- eta_seconds ------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGpsNavigationEtaTest,
    "GTC.AI.Gps.Eta",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGpsNavigationEtaTest::RunTest(const FString& Parameters)
{
    // test_eta_basic: 20m remaining at 5 m/s -> 4s.
    TestEqual(TEXT("basic"),
        FGpsNavigation::EtaSeconds(FVector(0, 0, 0), Straight(), 5.0), 4.0, Eps);
    // test_eta_zero_speed_is_inf
    const double Zero = FGpsNavigation::EtaSeconds(FVector(0, 0, 0), Straight(), 0.0);
    TestFalse(TEXT("zero speed is infinite"), FMath::IsFinite(Zero));

    return true;
}

// --- next_turn --------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGpsNavigationNextTurnTest,
    "GTC.AI.Gps.NextTurn",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGpsNavigationNextTurnTest::RunTest(const FString& Parameters)
{
    // test_next_turn_left
    {
        const FGpsNavigation::FNextTurn Turn =
            FGpsNavigation::NextTurn(FVector(0, 0, 0), LLeft(), FMath::DegreesToRadians(20.0));
        TestTrue(TEXT("left has turn"), Turn.bHasTurn);
        TestTrue(TEXT("left direction"), Turn.Direction == ETurnDirection::Left);
        TestTrue(TEXT("left position"), Turn.Position.Equals(FVector(10, 0, 0), Eps));
        TestEqual(TEXT("left distance"), Turn.Distance, 10.0, Eps);
    }
    // test_next_turn_right
    {
        const FGpsNavigation::FNextTurn Turn =
            FGpsNavigation::NextTurn(FVector(0, 0, 0), LRight(), FMath::DegreesToRadians(20.0));
        TestTrue(TEXT("right has turn"), Turn.bHasTurn);
        TestTrue(TEXT("right direction"), Turn.Direction == ETurnDirection::Right);
    }
    // test_next_turn_none_when_straight
    {
        const FGpsNavigation::FNextTurn Turn =
            FGpsNavigation::NextTurn(FVector(0, 0, 0), Straight(), FMath::DegreesToRadians(20.0));
        TestFalse(TEXT("straight has no turn"), Turn.bHasTurn);
    }
    // test_next_turn_skips_passed_corner
    {
        const FGpsNavigation::FNextTurn Turn =
            FGpsNavigation::NextTurn(FVector(10, 0, -5), LLeft(), FMath::DegreesToRadians(20.0));
        TestFalse(TEXT("passed corner has no turn"), Turn.bHasTurn);
    }

    return true;
}

// --- has_arrived ------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGpsNavigationHasArrivedTest,
    "GTC.AI.Gps.HasArrived",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGpsNavigationHasArrivedTest::RunTest(const FString& Parameters)
{
    // test_has_arrived_at_destination
    TestTrue(TEXT("at destination"),
        FGpsNavigation::HasArrived(FVector(20, 0, 0.5), Straight(), 1.0));
    // test_has_arrived_false_except_last
    TestFalse(TEXT("not at start"),
        FGpsNavigation::HasArrived(FVector(0, 0, 0), Straight(), 1.0));
    TestFalse(TEXT("not at midpoint"),
        FGpsNavigation::HasArrived(FVector(10, 0, 0), Straight(), 1.0));

    return true;
}

// --- direction_to_next ------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGpsNavigationDirectionToNextTest,
    "GTC.AI.Gps.DirectionToNext",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGpsNavigationDirectionToNextTest::RunTest(const FString& Parameters)
{
    // test_direction_to_next_along_route
    TestTrue(TEXT("along route"),
        FGpsNavigation::DirectionToNext(FVector(2, 0, 1), Straight()).Equals(FVector(1, 0, 0), Eps));
    // test_direction_to_next_on_vertical_leg
    TestTrue(TEXT("on vertical leg"),
        FGpsNavigation::DirectionToNext(FVector(10, 0, -3), LLeft()).Equals(FVector(0, 0, -1), Eps));
    // test_direction_to_next_degenerate
    TestTrue(TEXT("degenerate"),
        FGpsNavigation::DirectionToNext(FVector(0, 0, 0), {FVector::ZeroVector}).Equals(FVector::ZeroVector, Eps));

    return true;
}

#endif // WITH_AUTOMATION_TESTS
