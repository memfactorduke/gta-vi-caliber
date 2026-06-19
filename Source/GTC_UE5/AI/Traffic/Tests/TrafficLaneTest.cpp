// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../TrafficLane.h"
#include "../../../Worldcore/TrafficModel.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FTrafficLane — leader/gap selection that produces a spaced queue.
 * GTC-original. The final block pairs the selected gap with FTrafficModel to
 * show that a tight follower brakes while the front car (open road) accelerates,
 * i.e. "queue, don't overlap".
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTrafficLaneTest,
    "GTC.AI.Traffic.Lane",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficLaneTest::RunTest(const FString& Parameters)
{
    using FAgent = FTrafficLane::FAgent;

    // Three cars queued along the lane: centres at s = 0, 8, 30; 4.5 m long.
    TArray<FAgent> Cars;
    Cars.Add(FAgent{0.0, 10.0, 4.5});
    Cars.Add(FAgent{8.0, 10.0, 4.5});
    Cars.Add(FAgent{30.0, 10.0, 4.5});

    // Leader selection: each car follows the next one ahead; the front has none.
    TestEqual(TEXT("car0 leader is car1"), FTrafficLane::LeaderIndex(Cars, 0), 1);
    TestEqual(TEXT("car1 leader is car2"), FTrafficLane::LeaderIndex(Cars, 1), 2);
    TestEqual(TEXT("front car has no leader"), FTrafficLane::LeaderIndex(Cars, 2), -1);

    // Bumper gaps net out half of each body: (8-0) - 4.5 = 3.5; (30-8) - 4.5 = 17.5.
    TestTrue(TEXT("gap car0->car1"), FMath::Abs(FTrafficLane::BumperGap(Cars[0], Cars[1]) - 3.5) <= Eps);
    TestTrue(TEXT("gap car1->car2"), FMath::Abs(FTrafficLane::BumperGap(Cars[1], Cars[2]) - 17.5) <= Eps);

    // Overlap detection: 3.5 m bumper gap is not overlapping; centres 4 m apart are.
    TestFalse(TEXT("queued cars do not overlap"), FTrafficLane::Overlaps(Cars[0], Cars[1]));
    TestTrue(TEXT("cars 4m apart overlap"), FTrafficLane::Overlaps(FAgent{0.0, 0.0, 4.5}, FAgent{4.0, 0.0, 4.5}));

    // Out-of-range self is safe.
    TestEqual(TEXT("bad index has no leader"), FTrafficLane::LeaderIndex(Cars, 9), -1);

    // Determinism: on an exact distance tie the lower index wins.
    {
        TArray<FAgent> Tie;
        Tie.Add(FAgent{0.0, 0.0, 4.5});  // self
        Tie.Add(FAgent{5.0, 0.0, 4.5});  // ahead, index 1
        Tie.Add(FAgent{5.0, 0.0, 4.5});  // ahead, index 2 (same distance)
        TestEqual(TEXT("tie picks lower index"), FTrafficLane::LeaderIndex(Tie, 0), 1);
    }

    // Pairing with FTrafficModel: the tight follower brakes, the front accelerates.
    {
        const int32 Lead0 = FTrafficLane::LeaderIndex(Cars, 0);
        const double Gap0 = FTrafficLane::BumperGap(Cars[0], Cars[Lead0]);
        const double FollowAccel = FTrafficModel::CarFollowingAccel(
            Cars[0].Speed, Gap0, Cars[Lead0].Speed, /*v0*/ 14.0, /*a*/ 1.5, /*b*/ 2.0, /*s0*/ 2.0, /*T*/ 1.5);
        TestTrue(TEXT("tight follower brakes"), FollowAccel < 0.0);

        // Front car: no leader -> free road (huge gap, leader at own desired speed).
        const double FreeAccel = FTrafficModel::CarFollowingAccel(
            Cars[2].Speed, 1.0e6, 14.0, 14.0, 1.5, 2.0, 2.0, 1.5);
        TestTrue(TEXT("front car accelerates on open road"), FreeAccel > 0.0);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
