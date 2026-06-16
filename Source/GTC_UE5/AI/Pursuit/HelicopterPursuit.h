// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure air-support math for the police helicopter that joins the chase at high
 * heat (FPoliceResponse::UsesHelicopter(stars) — 3+ stars). It decides where the
 * chopper orbits above the target, how wide its searchlight footprint is, and
 * whether the target is currently lit.
 *
 * Direct port of the Godot `HelicopterPursuit` (RefCounted) at
 * `game/scripts/ai/helicopter_pursuit.gd`. All static, FVector-in,
 * FVector/scalar/bool-out, no UObject — unit-tested headless via the parity
 * oracle (Tests/HelicopterPursuitTest.cpp, prefix
 * GTC.AI.Pursuit.HelicopterPursuit).
 *
 * Double precision throughout, to match the GDScript float math. "Planar" is the
 * XZ ground plane (Godot Y-up); altitude is the +Y component. No Godot->UE Z-up
 * axis remap — the model stays in the Godot XZ frame so tests match the oracle
 * bit-for-bit; the axis convention is a DEFERRED Wave-3 concern.
 *
 * Depends on the merged FPoliceResponse (World/Police) for the deploy threshold;
 * not re-ported here.
 *
 * PURE-CORE boundary: the pure geometry/altitude math only — caller supplies
 * center, time, radii, altitude and the focus/target positions. Spotlight
 * actors, perception and Behavior Tree wiring are the deferred Wave-3 adapter,
 * NOT implemented here and NOT covered by the parity tests.
 */

struct GTC_UE5_API FHelicopterPursuit
{
    static constexpr double DefaultOrbitRadius = 28.0;
    static constexpr double DefaultAltitude = 32.0;
    static constexpr double DefaultConeDegrees = 22.0;

    /** True once the wanted level calls in air support (3+ stars). */
    static bool ShouldDeploy(int32 Stars);

    /** Where the chopper should be: circling `Center` at `Radius`/`Altitude`. */
    static FVector OrbitPoint(
        const FVector& Center, double Time, double Radius, double Altitude, double AngularSpeed);

    /** Half-angle (radians) of the searchlight cone from degrees, clamped (0, 89deg]. */
    static double ConeHalfRadians(double Degrees);

    /** Radius of the lit ground circle, given chopper height and cone half-angle. */
    static double SpotlightGroundRadius(double Altitude, double ConeHalf);

    /** Whether the target ground position stands inside the searchlight footprint. */
    static bool TargetLit(
        const FVector& FocusGround, const FVector& TargetGround, double LitRadius);
};
