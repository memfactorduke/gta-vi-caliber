// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleDriveInput.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Unit tests for FVehicleDriveInput — driver input shaping (deadzone, speed-
 * sensitive steering, steering smoothing, pedal/auto-reverse resolution).
 * Prefix GTC.Vehicles.DriveInput.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleDriveInputDeadzoneTest,
    "GTC.Vehicles.DriveInput.Deadzone",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleDriveInputDeadzoneTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("inside deadzone -> 0"), FVehicleDriveInput::ApplyDeadzone(0.03, 0.05), 0.0, Eps);
    TestEqual(TEXT("at deadzone edge -> 0"), FVehicleDriveInput::ApplyDeadzone(0.05, 0.05), 0.0, Eps);
    TestEqual(TEXT("full positive -> 1"), FVehicleDriveInput::ApplyDeadzone(1.0, 0.05), 1.0, Eps);
    TestEqual(TEXT("full negative -> -1"), FVehicleDriveInput::ApplyDeadzone(-1.0, 0.05), -1.0, Eps);
    // Range above the deadzone is rescaled back to full throw: (0.525-0.05)/0.95 = 0.5.
    TestEqual(TEXT("rescaled positive"), FVehicleDriveInput::ApplyDeadzone(0.525, 0.05), 0.5, Eps);
    TestEqual(TEXT("rescaled negative"), FVehicleDriveInput::ApplyDeadzone(-0.525, 0.05), -0.5, Eps);
    // Out-of-range input is clamped before shaping.
    TestEqual(TEXT("over-range clamps"), FVehicleDriveInput::ApplyDeadzone(2.0, 0.05), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleDriveInputSteerTest,
    "GTC.Vehicles.DriveInput.Steer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleDriveInputSteerTest::RunTest(const FString& Parameters)
{
    // Authority: full at/under the low speed, floored at the high speed, lerp between.
    TestEqual(TEXT("parked -> full authority"), FVehicleDriveInput::SteerAuthority(0.0), 1.0, Eps);
    TestEqual(TEXT("full-speed edge -> 1"), FVehicleDriveInput::SteerAuthority(800.0), 1.0, Eps);
    TestEqual(TEXT("min-speed edge -> floor"), FVehicleDriveInput::SteerAuthority(6000.0), FVehicleDriveInput::MinSteerAuthority, Eps);
    TestEqual(TEXT("midpoint lerps"), FVehicleDriveInput::SteerAuthority(3400.0), 0.675, Eps);
    TestEqual(TEXT("negative speed clamps to full"), FVehicleDriveInput::SteerAuthority(-10.0), 1.0, Eps);

    // Target combines deadzone + authority.
    TestEqual(TEXT("full lock at speed scales by authority"), FVehicleDriveInput::SteerTarget(1.0, 6000.0), 0.35, Eps);
    TestEqual(TEXT("deadzoned steer -> 0"), FVehicleDriveInput::SteerTarget(0.03, 1000.0), 0.0, Eps);
    TestEqual(TEXT("full lock parked -> full"), FVehicleDriveInput::SteerTarget(1.0, 0.0), 1.0, Eps);

    // Interp: no time -> unchanged; half-step; large dt clamps to target.
    TestEqual(TEXT("dt 0 holds"), FVehicleDriveInput::SteerInterp(0.2, 1.0, 0.0), 0.2, Eps);
    TestEqual(TEXT("half step"), FVehicleDriveInput::SteerInterp(0.0, 1.0, 0.1), 0.5, Eps);
    TestEqual(TEXT("large dt clamps to target"), FVehicleDriveInput::SteerInterp(0.0, 1.0, 1.0), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleDriveInputPedalsTest,
    "GTC.Vehicles.DriveInput.Pedals",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleDriveInputPedalsTest::RunTest(const FString& Parameters)
{
    // Gas only: straight throttle.
    {
        const auto O = FVehicleDriveInput::ResolvePedals(1.0, 0.0, 500.0);
        TestEqual(TEXT("gas throttle"), O.Throttle, 1.0, Eps);
        TestEqual(TEXT("gas no brake"), O.Brake, 0.0, Eps);
        TestFalse(TEXT("gas not reverse"), O.bReverse);
    }
    // Brake while rolling forward fast: brakes, does not reverse.
    {
        const auto O = FVehicleDriveInput::ResolvePedals(0.0, 1.0, 1000.0);
        TestEqual(TEXT("brake fwd throttle 0"), O.Throttle, 0.0, Eps);
        TestEqual(TEXT("brake fwd brake 1"), O.Brake, 1.0, Eps);
        TestFalse(TEXT("brake fwd not reverse"), O.bReverse);
    }
    // Brake at rest with no gas: engages reverse.
    {
        const auto O = FVehicleDriveInput::ResolvePedals(0.0, 1.0, 0.0);
        TestTrue(TEXT("reverse engaged"), O.bReverse);
        TestEqual(TEXT("reverse throttle"), O.Throttle, 1.0, Eps);
        TestEqual(TEXT("reverse no brake"), O.Brake, 0.0, Eps);
    }
    // At the reverse-engage speed boundary: still reverses.
    {
        const auto O = FVehicleDriveInput::ResolvePedals(0.0, 1.0, FVehicleDriveInput::ReverseEngageSpeed);
        TestTrue(TEXT("reverse at edge"), O.bReverse);
    }
    // Gas + brake together: brake wins, no reverse (gas held blocks reverse).
    {
        const auto O = FVehicleDriveInput::ResolvePedals(0.5, 1.0, 10.0);
        TestFalse(TEXT("gas+brake not reverse"), O.bReverse);
        TestEqual(TEXT("gas+brake throttle"), O.Throttle, 0.5, Eps);
        TestEqual(TEXT("gas+brake brake"), O.Brake, 1.0, Eps);
    }
    // Out-of-range pedals are clamped.
    {
        const auto O = FVehicleDriveInput::ResolvePedals(2.0, -1.0, 2000.0);
        TestEqual(TEXT("throttle clamps"), O.Throttle, 1.0, Eps);
        TestEqual(TEXT("brake clamps"), O.Brake, 0.0, Eps);
    }
    return true;
}

#endif // WITH_AUTOMATION_TESTS
