// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../RoadGrip.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FRoadGrip — wet-road grip feeding FVehicleHandling's GripFactor seam.
 * GTC-original (no Godot oracle). Pins dry = full grip at any speed, the linear wet
 * grip loss, aquaplaning needing BOTH water and speed, the speed ramp between the
 * thresholds, the collapse toward the floor, monotonicity in wetness and speed,
 * and the input clamps. Prefix GTC.Vehicles.Traction. The core is all-static, so no
 * shared helpers (nothing for the unity build to collide).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRoadGripTest,
    "GTC.Vehicles.Traction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRoadGripTest::RunTest(const FString& Parameters)
{
    const FRoadGrip::FParams P; // WetGripLoss 0.35, aquaplane 90..160 km/h, floor 0.25

    // ---- Dry road: full grip at any speed, never aquaplanes --------------------
    {
        TestTrue(TEXT("dry grip is 1 at rest"), FMath::IsNearlyEqual(FRoadGrip::GripFactor(0.0, 0.0, P), 1.0, Eps));
        TestTrue(TEXT("dry grip is 1 at 200 km/h"), FMath::IsNearlyEqual(FRoadGrip::GripFactor(0.0, 200.0, P), 1.0, Eps));
        TestTrue(TEXT("dry road doesn't aquaplane"), FMath::Abs(FRoadGrip::AquaplaneAmount(0.0, 200.0, P)) <= Eps);
        TestFalse(TEXT("dry IsAquaplaning false"), FRoadGrip::IsAquaplaning(0.0, 200.0, P));
    }

    // ---- A wet road is simply less grippy at low speed ------------------------
    {
        TestTrue(TEXT("full wet, low speed -> 0.65"), FMath::IsNearlyEqual(FRoadGrip::GripFactor(1.0, 0.0, P), 0.65, Eps));
        TestTrue(TEXT("half wet, low speed -> 0.825"), FMath::IsNearlyEqual(FRoadGrip::GripFactor(0.5, 0.0, P), 0.825, Eps));
        // Below the aquaplane speed, still just the wet baseline.
        TestTrue(TEXT("full wet at the aquaplane threshold is still 0.65"),
            FMath::IsNearlyEqual(FRoadGrip::GripFactor(1.0, 90.0, P), 0.65, Eps));
        TestFalse(TEXT("at the threshold, not yet aquaplaning"), FRoadGrip::IsAquaplaning(1.0, 90.0, P));
    }

    // ---- Aquaplaning: needs water AND speed, ramps, collapses to the floor -----
    {
        TestTrue(TEXT("aquaplane amount 0.5 at 125 km/h full wet"),
            FMath::IsNearlyEqual(FRoadGrip::AquaplaneAmount(1.0, 125.0, P), 0.5, Eps));
        TestTrue(TEXT("aquaplane amount 1.0 at 160 km/h full wet"),
            FMath::IsNearlyEqual(FRoadGrip::AquaplaneAmount(1.0, 160.0, P), 1.0, Eps));
        TestTrue(TEXT("aquaplane saturates beyond full speed"),
            FMath::IsNearlyEqual(FRoadGrip::AquaplaneAmount(1.0, 250.0, P), 1.0, Eps));

        TestTrue(TEXT("full wet at 125 -> 0.45 grip"), FMath::IsNearlyEqual(FRoadGrip::GripFactor(1.0, 125.0, P), 0.45, Eps));
        TestTrue(TEXT("full wet at 160 -> floor 0.25"), FMath::IsNearlyEqual(FRoadGrip::GripFactor(1.0, 160.0, P), 0.25, Eps));
        TestTrue(TEXT("IsAquaplaning when wet and fast"), FRoadGrip::IsAquaplaning(1.0, 120.0, P));

        // Less water -> less aquaplaning at the same speed.
        TestTrue(TEXT("half wet at 160 aquaplanes half as much"),
            FMath::IsNearlyEqual(FRoadGrip::AquaplaneAmount(0.5, 160.0, P), 0.5, Eps));
        TestTrue(TEXT("half wet at 160 -> 0.5375 grip"), FMath::IsNearlyEqual(FRoadGrip::GripFactor(0.5, 160.0, P), 0.5375, Eps));
    }

    // ---- Monotonic: wetter = less grip; faster (when wet) = less grip ----------
    {
        TestTrue(TEXT("wetter loses grip"), FRoadGrip::GripFactor(0.3, 50.0, P) > FRoadGrip::GripFactor(0.8, 50.0, P));
        TestTrue(TEXT("faster on a wet road loses grip"),
            FRoadGrip::GripFactor(1.0, 100.0, P) > FRoadGrip::GripFactor(1.0, 150.0, P));
        TestTrue(TEXT("dry beats wet at the same speed"),
            FRoadGrip::GripFactor(0.0, 150.0, P) > FRoadGrip::GripFactor(1.0, 150.0, P));
    }

    // ---- Input clamps ---------------------------------------------------------
    {
        TestTrue(TEXT("wetness over 1 clamps to full wet"),
            FMath::IsNearlyEqual(FRoadGrip::GripFactor(2.0, 0.0, P), FRoadGrip::GripFactor(1.0, 0.0, P), Eps));
        TestTrue(TEXT("negative wetness clamps to dry"),
            FMath::IsNearlyEqual(FRoadGrip::GripFactor(-1.0, 50.0, P), 1.0, Eps));
        TestTrue(TEXT("negative speed clamps to rest"),
            FMath::IsNearlyEqual(FRoadGrip::GripFactor(1.0, -50.0, P), 0.65, Eps));
        const double G = FRoadGrip::GripFactor(1.0, 9999.0, P);
        TestTrue(TEXT("grip never below 0"), G >= 0.0);
        TestTrue(TEXT("grip never above 1"), G <= 1.0);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
