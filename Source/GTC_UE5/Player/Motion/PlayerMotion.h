// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure movement math for the player controller.
 *
 * Direct port of the Godot `PlayerMotion` (RefCounted) static helpers at
 * `game/scripts/player/player_motion.gd`. Static methods only, no scene access —
 * the same testable-core pattern as FCameraFeel / FGpsNavigation. This is the
 * canonical source of truth for the camera-relative move-direction convention
 * (FCameraFeel's recenter test reproduces `DirectionFromInput` inline).
 *
 * Double precision throughout, to match the GDScript float math (FVector /
 * FVector2D components are doubles in UE5). Work happens in the Godot RH Y-up
 * frame: forward input (0, -1) at yaw 0 travels -Z, the horizontal plane is XZ,
 * and `up` is +Y. The rotation in `DirectionFromInput` is implemented as the
 * explicit Godot right-handed rotation about +Y (NOT FVector::RotateAngleAxis,
 * whose handedness/axis convention differs), so the parity oracle holds bit-for-
 * bit.
 *
 * NO Godot->UE Z-up axis remap is baked in here — the model stays in the Godot
 * frame so the ported unit tests match the oracle. Mapping these outputs onto a
 * UE CharacterMovement rig (Z-up, the actual velocity/jump/floor wiring) is a
 * DEFERRED Wave-3 adapter and is NOT implemented or parity-covered here.
 *
 * Parity coverage: Tests/PlayerMotionTest.cpp, prefix GTC.Player.Motion,
 * mirroring game/tests/unit/test_player_motion.gd 1:1.
 */
struct GTC_UE5_API FPlayerMotion
{
    /** Planar tolerance mirroring the Godot is_zero_approx / length guards. */
    static constexpr double Eps = 1e-4;

    /**
     * Convert 2D input (from Input.get_vector) into a world-space direction,
     * rotated so "forward" follows the camera yaw. Returns a unit vector or the
     * zero vector. Maps input (x, y) to local (x, 0, y) then rotates about +Y.
     */
    static FVector DirectionFromInput(const FVector2D& InputDir, double CameraYaw);

    /** Target horizontal velocity for a direction and speed (y is always 0). */
    static FVector HorizontalVelocity(const FVector& Direction, double Speed);

    /** Move the current horizontal velocity toward the target, leaving y intact. */
    static FVector Accelerated(const FVector& Current, const FVector& Target, double Acceleration, double Delta);

    /**
     * Pick this frame's acceleration rate: speeding up and braking use separate
     * rates (braking is stronger), both scaled by AirControl while airborne so
     * jumps keep their momentum.
     */
    static double AccelerationRate(bool bHasInput, bool bOnFloor, double Accel, double Decel, double AirControl);

    /**
     * Whether a jump should fire this frame. Combines coyote time (a late press
     * shortly after walking off a ledge still counts) with jump buffering (an
     * early press shortly before landing still counts). bJumpSpent guards against
     * double-firing until the character touches the floor again.
     */
    static bool ShouldJump(
        double TimeSinceGrounded,
        double CoyoteTime,
        double TimeSinceJumpPressed,
        double BufferTime,
        bool bJumpSpent);

    /**
     * Fall damage from a landing's downward speed (m/s): nothing at or below
     * SafeSpeed, ramping linearly to MaxDamage at LethalSpeed (clamped there for
     * harder hits). A degenerate range (Lethal <= Safe) deals nothing.
     */
    static double FallDamage(double ImpactSpeed, double SafeSpeed, double LethalSpeed, double MaxDamage);

    /**
     * Downhill slide acceleration (m/s^2, horizontal) on a floor too steep to
     * stand on cleanly. FloorNormal is the contact normal; once its Y drops below
     * MaxWalkNormalY (= cos of the steepest stable angle) the character is pushed
     * down the fall line, scaled by how far past the threshold the slope is.
     * Returns the zero vector on flat-enough or degenerate ground. The fall line
     * is the horizontal part of the normal (steepest-descent direction).
     */
    static FVector SlopeSlide(const FVector& FloorNormal, double MaxWalkNormalY, double SlideAccel);

    /**
     * Velocity while latched to a ladder: forward input climbs, back input
     * descends, and the world-space move Direction is kept at half speed so the
     * player can steer off the ladder sideways or over the top lip.
     */
    static FVector ClimbVelocity(const FVector2D& InputDir, const FVector& Direction, double ClimbSpeed);
};
