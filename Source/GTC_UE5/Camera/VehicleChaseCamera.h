// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure chase-camera feel for a driven vehicle — the "sense of speed" math that
 * sits behind the spring-arm follow cam. On-foot camera feel lives in
 * FCameraFeel (sprint FOV, smoothing, recenter, free-look); this is its vehicle
 * sibling and deliberately does NOT duplicate it: for the yaw that swings the
 * camera back behind the car, reuse FCameraFeel::ApproachAngle (shortest-angle,
 * wrap-aware) toward (VehicleYaw + LookBehindYawOffset).
 *
 * What it shapes:
 *   - SpeedFov: FOV widens with speed so fast driving feels fast.
 *   - FollowDistance: the camera eases back at speed to frame more road ahead.
 *   - LookBehindYawOffset: flip the chase direction 180 degrees while the player
 *     holds look-behind.
 *
 * PURE-CORE: float, scene-free, deterministic, no engine coupling. Speeds are in
 * the adapter's units (cm/s assumed by callers; the math is scale-free). FOV is
 * in degrees, angles in radians. Unit-tested headless:
 * Camera/Tests/VehicleChaseCameraTest.cpp, prefix GTC.Camera.VehicleChaseCamera.
 */
struct GTC_UE5_API FVehicleChaseCamera
{
    /**
     * Field of view (degrees) for the current speed: lerps BaseFov -> MaxFov as
     * speed climbs 0 -> SpeedAtMaxFov, then holds MaxFov. A non-positive
     * SpeedAtMaxFov disables the kick (returns BaseFov). Speed is clamped >= 0.
     */
    static float SpeedFov(float BaseFov, float MaxFov, float Speed, float SpeedAtMaxFov);

    /**
     * Spring-arm length for the current speed: lerps BaseDistance -> MaxDistance
     * as speed climbs 0 -> SpeedAtMaxDistance, then holds. A non-positive
     * SpeedAtMaxDistance disables the pull-back (returns BaseDistance). Speed is
     * clamped >= 0.
     */
    static float FollowDistance(float BaseDistance, float MaxDistance, float Speed, float SpeedAtMaxDistance);

    /**
     * Yaw offset (radians) added to the vehicle's facing to get the chase
     * direction: 0 when following normally, PI (180 deg) while looking behind.
     */
    static float LookBehindYawOffset(bool bLookBehind);

    /**
     * Boom pitch (radians) that frames a climbing/diving aircraft: atan2(climb, horiz)
     * scaled by Gain and clamped to +/- MaxPitch. Positive (camera tilts up to see
     * ahead) in a climb, negative in a dive, 0 in level flight. A craft going straight
     * up clamps to MaxPitch rather than swinging to vertical. Ground vehicles and boats
     * leave this unused (their vertical is negligible). Reuse FCameraFeel::ExpSmoothed
     * to ease it frame to frame.
     */
    static float PitchFollow(float ClimbRateCmS, float HorizSpeedCmS, float Gain, float MaxPitchRad);
};
