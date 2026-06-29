// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleAxisState.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FVehicleAxisState — the held-setting integrator behind a helicopter's
 * collective and a plane's throttle (raise/lower a lever that stays put). GTC-original.
 * Pins the raise/lower step, range clamping, input-direction clamping, and the
 * non-positive-dt hold. Mirrors Scripts/gtc_aircraft/axis_state_verify.cpp.
 * Prefix GTC.Vehicles.AxisState.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleAxisStateTest,
    "GTC.Vehicles.AxisState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleAxisStateTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("raise by rate*dt"), FVehicleAxisState::Integrate(0.5, 1.0, 0.4, 1.0), 0.9, Eps);
    TestEqual(TEXT("lower by rate*dt"), FVehicleAxisState::Integrate(0.5, -1.0, 0.4, 1.0), 0.1, Eps);

    TestEqual(TEXT("clamps at top"), FVehicleAxisState::Integrate(0.9, 1.0, 0.4, 1.0), 1.0, Eps);
    TestEqual(TEXT("clamps at bottom"), FVehicleAxisState::Integrate(0.1, -1.0, 0.4, 1.0), 0.0, Eps);

    TestEqual(TEXT("over-range input clamps up"), FVehicleAxisState::Integrate(0.0, 5.0, 0.4, 1.0), 0.4, Eps);
    TestEqual(TEXT("over-range input clamps down"), FVehicleAxisState::Integrate(0.4, -5.0, 0.4, 1.0), 0.0, Eps);

    TestEqual(TEXT("zero dt holds"), FVehicleAxisState::Integrate(0.37, 1.0, 0.4, 0.0), 0.37, Eps);
    TestEqual(TEXT("negative dt holds"), FVehicleAxisState::Integrate(0.37, 1.0, 0.4, -2.0), 0.37, Eps);
    TestEqual(TEXT("out-of-range current clamps even at dt 0"),
        FVehicleAxisState::Integrate(5.0, 0.0, 0.0, 0.0), 1.0, Eps);

    TestEqual(TEXT("custom hi"), FVehicleAxisState::Integrate(0.0, 1.0, 1.0, 1.0, -1.0, 1.0), 1.0, Eps);
    TestEqual(TEXT("custom lo"), FVehicleAxisState::Integrate(0.0, -1.0, 5.0, 1.0, -1.0, 1.0), -1.0, Eps);
    TestEqual(TEXT("inverted Lo/Hi treated as range"),
        FVehicleAxisState::Integrate(0.0, 1.0, 0.4, 1.0, 1.0, 0.0), 0.4, Eps);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
