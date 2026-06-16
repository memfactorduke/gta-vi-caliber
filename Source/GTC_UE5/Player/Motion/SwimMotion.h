// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure swim math for the player when in water.
 *
 * Direct port of the Godot `SwimMotion` (RefCounted) static helpers at
 * `game/scripts/player/swim_motion.gd`. Static methods only, no scene access —
 * the same testable-core pattern as FPlayerMotion. This owns submersion, the
 * enter/leave hysteresis, stroke input and passive buoyancy.
 *
 * Double precision throughout, to match the GDScript float math. Work stays in
 * the Godot Y-up frame (vertical is +Y); `Direction` is the planar unit vector
 * from FPlayerMotion. NO Godot->UE Z-up axis remap is baked in here — wiring
 * these outputs onto a UE CharacterMovement rig is a DEFERRED Wave-3 adapter and
 * is NOT parity-covered.
 *
 * Parity coverage: Tests/SwimMotionTest.cpp, prefix GTC.Player.Motion,
 * mirroring game/tests/unit/test_swim_motion.gd 1:1.
 */
struct GTC_UE5_API FSwimMotion
{
    /**
     * How far the body is submerged, as a fraction of its height: 0.0 when the
     * feet just touch the surface, 1.0 when the head goes under. OriginY is the
     * body's feet; the body spans up to OriginY + BodyHeight. Clamped, and safe
     * for a degenerate (<= 0) height.
     */
    static double Submersion(double OriginY, double WaterY, double BodyHeight);

    /**
     * Swim state with hysteresis so the shoreline doesn't flicker. Start swimming
     * once submerged past EnterFraction (chest-deep); keep swimming until back
     * down past ExitFraction (wading depth). EnterFraction should exceed
     * ExitFraction.
     */
    static bool IsSwimming(double SubmersionFraction, bool bCurrently, double EnterFraction, double ExitFraction);

    /**
     * Vertical stroke axis from the surface/dive keys: +1 kick up, -1 dive down,
     * 0 when neither or both are held.
     */
    static double VerticalAxis(bool bSurfacePressed, bool bDivePressed);

    /**
     * Target swim velocity: the horizontal component follows the camera-relative
     * move Direction at SwimSpeed, the vertical component is the stroke Axis at
     * VerticalSpeed. Direction is the planar unit vector from FPlayerMotion.
     */
    static FVector TargetVelocity(const FVector& Direction, double SwimSpeed, double Axis, double VerticalSpeed);

    /**
     * Passive buoyancy used when no vertical stroke is held: the body eases toward
     * floating at Neutral submersion. Deeper than neutral pushes up, shallower
     * lets it settle; result is a vertical speed clamped to +/-MaxSpeed.
     */
    static double Buoyancy(double SubmersionFraction, double Neutral, double Strength, double MaxSpeed);

    /**
     * Whether the head is under: submerged at or past HeadFraction (near 1.0).
     * Below it the player can still breathe even while wading deep.
     */
    static bool HeadUnderwater(double SubmersionFraction, double HeadFraction);

    /**
     * Next breath reserve in [0, 1]: underwater it drains over BreathSeconds; at
     * the surface it refills at RecoverRate per second. A degenerate BreathSeconds
     * is treated as instant.
     */
    static double NextOxygen(double Oxygen, bool bUnderwater, double BreathSeconds, double RecoverRate, double Delta);
};
