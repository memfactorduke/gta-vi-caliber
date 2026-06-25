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

#endif // WITH_AUTOMATION_TESTS
