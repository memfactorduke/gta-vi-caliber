// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleChaseCamera.h"
#include "../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Unit tests for FVehicleChaseCamera — speed-based FOV, speed-based follow
 * distance, and the look-behind yaw offset. Prefix GTC.Camera.VehicleChaseCamera.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleChaseCameraFovTest,
    "GTC.Camera.VehicleChaseCamera.Fov",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleChaseCameraFovTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("idle -> base"), (double)FVehicleChaseCamera::SpeedFov(70, 90, 0, 1000), 70.0, Eps);
    TestEqual(TEXT("at max-speed -> max"), (double)FVehicleChaseCamera::SpeedFov(70, 90, 1000, 1000), 90.0, Eps);
    TestEqual(TEXT("halfway lerps"), (double)FVehicleChaseCamera::SpeedFov(70, 90, 500, 1000), 80.0, Eps);
    TestEqual(TEXT("over max-speed holds"), (double)FVehicleChaseCamera::SpeedFov(70, 90, 2000, 1000), 90.0, Eps);
    TestEqual(TEXT("disabled (0 ramp) -> base"), (double)FVehicleChaseCamera::SpeedFov(70, 90, 100, 0), 70.0, Eps);
    TestEqual(TEXT("negative speed clamps to base"), (double)FVehicleChaseCamera::SpeedFov(70, 90, -50, 1000), 70.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleChaseCameraDistanceTest,
    "GTC.Camera.VehicleChaseCamera.Distance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleChaseCameraDistanceTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("idle -> base"), (double)FVehicleChaseCamera::FollowDistance(400, 650, 0, 1000), 400.0, Eps);
    TestEqual(TEXT("at max-speed -> max"), (double)FVehicleChaseCamera::FollowDistance(400, 650, 1000, 1000), 650.0, Eps);
    TestEqual(TEXT("halfway lerps"), (double)FVehicleChaseCamera::FollowDistance(400, 650, 500, 1000), 525.0, Eps);
    TestEqual(TEXT("disabled (0 ramp) -> base"), (double)FVehicleChaseCamera::FollowDistance(400, 650, 500, 0), 400.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleChaseCameraLookBehindTest,
    "GTC.Camera.VehicleChaseCamera.LookBehind",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleChaseCameraLookBehindTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("forward -> 0"), (double)FVehicleChaseCamera::LookBehindYawOffset(false), 0.0, Eps);
    TestEqual(TEXT("behind -> PI"), (double)FVehicleChaseCamera::LookBehindYawOffset(true), (double)PI, Eps);
    return true;
}

// The Wings & Waves addition: 3D boom pitch that frames a climbing/diving aircraft.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleChaseCameraPitchFollowTest,
    "GTC.Camera.VehicleChaseCamera.PitchFollow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleChaseCameraPitchFollowTest::RunTest(const FString& Parameters)
{
    const double Climb = FMath::Atan2(500.0, 1000.0); // ~0.4636
    TestEqual(TEXT("climb tilts boom up"), (double)FVehicleChaseCamera::PitchFollow(500, 1000, 1, 0.6f), Climb, Eps);
    TestEqual(TEXT("level -> no boom pitch"), (double)FVehicleChaseCamera::PitchFollow(0, 1000, 1, 0.6f), 0.0, Eps);
    TestEqual(TEXT("dive tilts boom down"), (double)FVehicleChaseCamera::PitchFollow(-500, 1000, 1, 0.6f), -Climb, Eps);
    TestEqual(TEXT("straight up clamps to max pitch"), (double)FVehicleChaseCamera::PitchFollow(10000, 1, 1, 0.5f), 0.5, Eps);
    TestEqual(TEXT("gain scales the boom pitch"), (double)FVehicleChaseCamera::PitchFollow(500, 1000, 2, 2.0f), 2.0 * Climb, Eps);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
