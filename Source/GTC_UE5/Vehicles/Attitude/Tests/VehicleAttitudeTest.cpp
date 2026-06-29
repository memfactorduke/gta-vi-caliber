// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleAttitude.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FVehicleAttitude — bank-into-the-turn and pitch-follows-the-flight-path.
 * GTC-original. Pins no-bank at standstill / wings-level, the coordinated-turn value,
 * sign + gain + max-roll clamp, climb/level/dive pitch, and the near-vertical clamp.
 * Mirrors Scripts/gtc_aircraft/attitude_verify.cpp. Prefix GTC.Vehicles.Attitude.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleAttitudeTest,
    "GTC.Vehicles.Attitude",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleAttitudeTest::RunTest(const FString& Parameters)
{
    // ---- Bank into a turn -----------------------------------------------------
    TestTrue(TEXT("wings level -> no bank"), FMath::Abs(FVehicleAttitude::BankAngle(0.0, 3000.0, 1.0, 0.6)) <= Eps);
    TestTrue(TEXT("standstill -> no bank"), FMath::Abs(FVehicleAttitude::BankAngle(0.5, 0.0, 1.0, 0.6)) <= Eps);

    const double ExpectBank = FMath::Atan2(1000.0 * 0.1, FVehicleAttitude::GravityCmS2);
    TestEqual(TEXT("gentle bank = coordinated turn"), FVehicleAttitude::BankAngle(0.1, 1000.0, 1.0, 1.5), ExpectBank, Eps);
    TestTrue(TEXT("opposite yaw -> opposite bank"), FVehicleAttitude::BankAngle(-0.1, 1000.0, 1.0, 1.5) < 0.0);
    TestEqual(TEXT("gain scales the bank"), FVehicleAttitude::BankAngle(0.1, 1000.0, 2.0, 1.5), 2.0 * ExpectBank, Eps);
    TestEqual(TEXT("hard turn clamps to max roll"), FVehicleAttitude::BankAngle(0.5, 3000.0, 1.0, 0.6), 0.6, Eps);

    // ---- Pitch follows the flight path ----------------------------------------
    const double Climb = FMath::Atan2(500.0, 1000.0);
    TestEqual(TEXT("climbing -> nose up"), FVehicleAttitude::PitchFromVelocity(500.0, 1000.0, 0.6), Climb, Eps);
    TestEqual(TEXT("level -> no pitch"), FVehicleAttitude::PitchFromVelocity(0.0, 1000.0, 0.6), 0.0, Eps);
    TestEqual(TEXT("diving -> nose down"), FVehicleAttitude::PitchFromVelocity(-500.0, 1000.0, 0.6), -Climb, Eps);
    TestEqual(TEXT("near-vertical climb clamps to max pitch"), FVehicleAttitude::PitchFromVelocity(10000.0, 1.0, 0.5), 0.5, Eps);
    TestEqual(TEXT("negative horiz speed treated as 0"), FVehicleAttitude::PitchFromVelocity(500.0, -1000.0, 0.6), 0.6, Eps);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
