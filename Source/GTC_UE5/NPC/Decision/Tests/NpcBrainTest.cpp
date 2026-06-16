// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcBrain.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Parity tests for FNpcBrain, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_npc_brain.gd. Tolerance mirrors the oracle's
 * is_equal_approx (Eps = 1e-4); vector and state equality assert exactly.
 *
 * PURE-CORE boundary: the moment-to-moment locomotion wiring (Pedestrian node
 * owning target/timers) is the deferred Wave-3 adapter and is NOT tested.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcBrainTest,
    "GTC.NPC.Decision.NpcBrain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcBrainTest::RunTest(const FString& Parameters)
{
    // test_wander_target_within_radius
    {
        bool bAllWithin = true;
        for (int32 i = 0; i <= 10; ++i)
        {
            for (int32 j = 0; j <= 10; ++j)
            {
                const FVector P = FNpcBrain::WanderTarget(
                    FVector(5, 0, 5), 8.0, double(i) / 10.0, double(j) / 10.0);
                if (FNpcBrain::PlanarDistance(P, FVector(5, 0, 5)) > 8.001)
                {
                    bAllWithin = false;
                }
            }
        }
        TestTrue(TEXT("wander target within radius"), bAllWithin);
    }

    // test_wander_target_ignores_y
    {
        const FVector P = FNpcBrain::WanderTarget(FVector(0, 3, 0), 5.0, 0.5, 0.5);
        TestEqual(TEXT("wander target ignores y"), P.Y, 3.0, Eps);
    }

    // test_planar_distance_ignores_height
    TestEqual(TEXT("planar distance ignores height"),
        FNpcBrain::PlanarDistance(FVector(0, 10, 0), FVector(3, -2, 4)), 5.0, Eps);

    // test_arrived_true_within_tolerance
    TestTrue(TEXT("arrived true within tolerance"),
        FNpcBrain::Arrived(FVector(0, 0, 0), FVector(0.5, 0, 0), 1.0));

    // test_arrived_false_when_far
    TestFalse(TEXT("arrived false when far"),
        FNpcBrain::Arrived(FVector(0, 0, 0), FVector(5, 0, 0), 1.0));

    // test_planar_dir_is_unit
    {
        const FVector D = FNpcBrain::PlanarDir(FVector(0, 0, 0), FVector(3, 9, 4));
        TestEqual(TEXT("planar dir is unit"), D.Size(), 1.0, Eps);
        TestEqual(TEXT("planar dir y zero"), D.Y, 0.0, Eps);
    }

    // test_planar_dir_zero_when_coincident
    TestTrue(TEXT("planar dir zero when coincident"),
        FNpcBrain::PlanarDir(FVector(1, 0, 1), FVector(1, 5, 1)) == FVector::ZeroVector);

    // test_flee_dir_points_away_from_threat: threat at -x, self at origin -> +x
    {
        const FVector D = FNpcBrain::FleeDir(FVector(0, 0, 0), FVector(-2, 0, 0));
        TestTrue(TEXT("flee dir points away from threat"), D.Equals(FVector(1, 0, 0), Eps));
    }

    // test_pursue_dir_points_toward_target: target at +x, self at origin -> +x
    {
        const FVector D = FNpcBrain::PursueDir(FVector(0, 0, 0), FVector(5, 0, 0));
        TestTrue(TEXT("pursue dir points toward target"), D.Equals(FVector(1, 0, 0), Eps));
    }

    // test_pursue_and_flee_are_opposite
    {
        const FVector SelfPos(2, 0, 3);
        const FVector Other(-4, 0, 9);
        TestTrue(TEXT("pursue and flee are opposite"),
            FNpcBrain::PursueDir(SelfPos, Other).Equals(-FNpcBrain::FleeDir(SelfPos, Other), Eps));
    }

    // test_enters_flee_when_threat_close_and_active
    TestTrue(TEXT("enters flee when threat close and active"),
        FNpcBrain::NextState(FNpcBrain::EState::Wander, true, 4.0, 8.0, 14.0) == FNpcBrain::EState::Flee);

    // test_stays_wander_when_no_threat
    TestTrue(TEXT("stays wander when no threat"),
        FNpcBrain::NextState(FNpcBrain::EState::Wander, false, 2.0, 8.0, 14.0) == FNpcBrain::EState::Wander);

    // test_ignores_distant_threat
    TestTrue(TEXT("ignores distant threat"),
        FNpcBrain::NextState(FNpcBrain::EState::Wander, true, 20.0, 8.0, 14.0) == FNpcBrain::EState::Wander);

    // test_flee_hysteresis_keeps_running_inside_calm_radius
    TestTrue(TEXT("flee hysteresis keeps running inside calm radius"),
        FNpcBrain::NextState(FNpcBrain::EState::Flee, true, 10.0, 8.0, 14.0) == FNpcBrain::EState::Flee);

    // test_flee_ends_beyond_calm_radius
    TestTrue(TEXT("flee ends beyond calm radius"),
        FNpcBrain::NextState(FNpcBrain::EState::Flee, true, 16.0, 8.0, 14.0) == FNpcBrain::EState::Wander);

    // test_flee_ends_when_threat_gone
    TestTrue(TEXT("flee ends when threat gone"),
        FNpcBrain::NextState(FNpcBrain::EState::Flee, false, 3.0, 8.0, 14.0) == FNpcBrain::EState::Wander);

    // test_speed_for_states
    {
        const double IdleSpeed = FNpcBrain::SpeedFor(FNpcBrain::EState::Idle, 3.0, 7.0);
        const double WanderSpeed = FNpcBrain::SpeedFor(FNpcBrain::EState::Wander, 3.0, 7.0);
        const double FleeSpeed = FNpcBrain::SpeedFor(FNpcBrain::EState::Flee, 3.0, 7.0);
        TestEqual(TEXT("idle speed"), IdleSpeed, 0.0, Eps);
        TestEqual(TEXT("wander speed"), WanderSpeed, 3.0, Eps);
        TestEqual(TEXT("flee speed"), FleeSpeed, 7.0, Eps);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
