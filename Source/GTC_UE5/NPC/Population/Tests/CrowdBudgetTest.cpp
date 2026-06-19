// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../CrowdBudget.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FCrowdBudget — crowd streaming arithmetic. GTC-original. Pins the
 * eased-up spawn count, the despawn/ring/LOD thresholds, and the farthest-first
 * retire pick.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrowdBudgetTest,
    "GTC.NPC.CrowdBudget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrowdBudgetTest::RunTest(const FString& Parameters)
{
    // Spawn count eases the headcount up to target, capped per pass, never over.
    TestEqual(TEXT("small deficit spawns the deficit"), FCrowdBudget::SpawnCount(38, 40, 4), 2);
    TestEqual(TEXT("at target spawns nothing"), FCrowdBudget::SpawnCount(40, 40, 4), 0);
    TestEqual(TEXT("big deficit is capped per pass"), FCrowdBudget::SpawnCount(10, 40, 4), 4);
    TestEqual(TEXT("over target spawns nothing"), FCrowdBudget::SpawnCount(45, 40, 4), 0);

    // Despawn radius (strict).
    TestTrue(TEXT("beyond radius despawns"), FCrowdBudget::ShouldDespawn(9000, 8000));
    TestFalse(TEXT("inside radius stays"), FCrowdBudget::ShouldDespawn(5000, 8000));
    TestFalse(TEXT("exactly at radius stays"), FCrowdBudget::ShouldDespawn(8000, 8000));

    // Spawn ring is inclusive of its bounds.
    TestTrue(TEXT("in the ring"), FCrowdBudget::InSpawnRing(4000, 2500, 6000));
    TestTrue(TEXT("on the inner edge"), FCrowdBudget::InSpawnRing(2500, 2500, 6000));
    TestFalse(TEXT("too close"), FCrowdBudget::InSpawnRing(1000, 2500, 6000));
    TestFalse(TEXT("too far"), FCrowdBudget::InSpawnRing(7000, 2500, 6000));

    // Distance LOD.
    TestTrue(TEXT("near ticks every frame"), FMath::Abs(FCrowdBudget::TickInterval(3000, 4000, 0.2)) <= Eps);
    TestTrue(TEXT("far ticks on the coarse interval"),
        FMath::Abs(FCrowdBudget::TickInterval(5000, 4000, 0.2) - 0.2) <= Eps);

    // Farthest-beyond retire pick.
    {
        const TArray<double> D({1000.0, 9000.0, 8500.0, 100.0});
        TestEqual(TEXT("retires the farthest over-radius citizen"), FCrowdBudget::FarthestBeyond(D, 8000.0), 1);
        TestEqual(TEXT("none beyond -> -1"),
            FCrowdBudget::FarthestBeyond(TArray<double>({100.0, 200.0}), 8000.0), -1);
        TestEqual(TEXT("tie keeps the lower index"),
            FCrowdBudget::FarthestBeyond(TArray<double>({9000.0, 9000.0, 100.0}), 8000.0), 0);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
