// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure camera-feel math (FOV kick, smoothing, recenter, turn-roll, free-look).
 *
 * Direct port of the the reference `CameraFeel` (RefCounted) static helpers at
 * `game/scripts/camera/camera_feel.gd`. Static methods only, no scene access —
 * the same testable-core pattern as PlayerMotion. Numeric/pure by design.
 *
 * Coordinate handedness (the reference RH Y-up -> UE LH Z-up): deliberately NOT handled
 * here. These helpers stay pure scalar/Vector2 math; the axis/sign remap that
 * wires them onto the UE camera rig is a Wave 3 integration concern, not part of
 * this parity unit. The `RecenterYaw` convention below mirrors the the reference math
 * exactly so its reference behavior holds; mapping its result onto a UE yaw axis is
 * likewise W3.
 *
 * Parity coverage (see Tests/CameraFeelTest.cpp, prefix GTC.Camera.CameraFeel),
 * mirroring game/tests/unit/test_camera_feel.gd:
 *   SprintBlend, FovForBlend, ExpSmoothed, RecenterYaw, ApproachAngle, TurnRoll.
 *
 * NOT parity-covered: LookOffset and LookReturn have NO the reference test oracle. They
 * are ported faithfully but are UNTESTED FOR PARITY — verify them behaviorally
 * during Wave 3 camera integration. No automation test asserts their behavior.
 */
struct GTC_UE5_API FCameraFeel
{
    /**
     * How far into "sprinting" the current speed is, 0..1. Used to blend the FOV
     * kick in proportionally instead of snapping on a key press. A degenerate
     * range (SprintSpeed <= WalkSpeed) yields 0.
     */
    static float SprintBlend(float Speed, float WalkSpeed, float SprintSpeed);

    /** Target field of view for a sprint blend amount. */
    static float FovForBlend(float BaseFov, float Kick, float Blend);

    /**
     * Frame-rate-independent exponential approach: composing two half-steps
     * gives exactly one full step, so feel doesn't change with FPS.
     */
    static float ExpSmoothed(float Current, float Target, float Smoothing, float Delta);

    /**
     * Camera yaw that looks along a horizontal travel direction — the angle to
     * recenter to so the player runs away from the camera. Matches PlayerMotion's
     * convention (forward input at yaw 0 travels -Z): solving -Z*R(yaw) = velocity
     * gives atan2(-Vx, -Vz). A zero vector yields 0.
     */
    static float RecenterYaw(float VelocityX, float VelocityZ);

    /**
     * Step an angle toward a target along the shortest arc, capped at MaxStep, so
     * recentering never spins the long way round a +/-PI wrap.
     */
    static float ApproachAngle(float Current, float Target, float MaxStep);

    /**
     * Camera roll (radians) banking into a turn for the driving chase cam:
     * proportional to the car's yaw rate, scaled by the speed blend (so it only
     * kicks in at speed), and capped at MaxRoll. Sign is negated so a left turn
     * (positive yaw rate) tilts the horizon into the corner.
     */
    static float TurnRoll(float YawRate, float Blend, float RollGain, float MaxRoll);

    /**
     * UNTESTED FOR PARITY — no the reference oracle exists; verify behaviorally during
     * Wave 3 camera integration.
     *
     * Apply a mouse-motion delta to a free-look (yaw, pitch) offset and clamp it,
     * so the driving chase cam can glance around the car. Mouse right/down subtract
     * (matching OrbitCamera) so the view turns the intuitive way. Yaw is bounded to
     * +/-YawLimit and pitch to [PitchMin, PitchMax]. Returns (X=yaw, Y=pitch).
     */
    static FVector2D LookOffset(
        const FVector2D& Current,
        const FVector2D& MouseRelative,
        float Sensitivity,
        float YawLimit,
        float PitchMin,
        float PitchMax);

    /**
     * UNTESTED FOR PARITY — no the reference oracle exists; verify behaviorally during
     * Wave 3 camera integration.
     *
     * Ease a free-look offset back toward neutral at Rate rad/s on each axis —
     * used once the player stops looking around so the camera follows the car
     * again without them steering it back by hand.
     */
    static FVector2D LookReturn(const FVector2D& Current, float Rate, float Delta);
};
