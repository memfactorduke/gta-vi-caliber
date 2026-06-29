// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleImpact.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FVehicleImpact — the crash speed->damage curve and graze restitution.
 * GTC-original. Pins the safe-speed floor, the quadratic growth, zero-scale/negative
 * guards, and the glance-keeps-most vs head-on-stops restitution. Mirrors
 * Scripts/gtc_aircraft/impact_verify.cpp. Prefix GTC.Vehicles.Impact.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleImpactTest,
    "GTC.Vehicles.Impact",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleImpactTest::RunTest(const FString& Parameters)
{
    // ---- Damage curve ---------------------------------------------------------
    TestTrue(TEXT("below safe -> no damage"), FVehicleImpact::DamageForSpeed(400.0, 500.0, 1e-4) <= Eps);
    TestTrue(TEXT("at safe -> no damage"), FVehicleImpact::DamageForSpeed(500.0, 500.0, 1e-4) <= Eps);
    TestEqual(TEXT("quadratic past safe"), FVehicleImpact::DamageForSpeed(1500.0, 500.0, 1e-4), 100.0, Eps);
    TestEqual(TEXT("double overspeed -> 4x damage"), FVehicleImpact::DamageForSpeed(2500.0, 500.0, 1e-4), 400.0, Eps);
    TestTrue(TEXT("zero scale -> no damage"), FVehicleImpact::DamageForSpeed(9000.0, 500.0, 0.0) <= Eps);
    TestTrue(TEXT("negative speed -> no damage"), FVehicleImpact::DamageForSpeed(-100.0, 500.0, 1e-4) <= Eps);

    // ---- Restitution ----------------------------------------------------------
    TestEqual(TEXT("perpendicular glance keeps all"), FVehicleImpact::Restitution(800.0, 0.0), 1.0, Eps);
    TestTrue(TEXT("head-on hit dead-stops"), FVehicleImpact::Restitution(800.0, 1.0) <= Eps);
    TestEqual(TEXT("partial graze keeps 0.75"), FVehicleImpact::Restitution(800.0, 0.25), 0.75, Eps);
    TestTrue(TEXT("no into-surface speed -> nothing retained"), FVehicleImpact::Restitution(0.0, 0.0) <= Eps);
    TestTrue(TEXT("moving away -> nothing retained"), FVehicleImpact::Restitution(-50.0, 0.0) <= Eps);
    TestTrue(TEXT("over-range dot clamps to head-on"), FVehicleImpact::Restitution(800.0, 5.0) <= Eps);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
