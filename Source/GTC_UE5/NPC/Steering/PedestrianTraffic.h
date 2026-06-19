// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure traffic-awareness math for pedestrians — the "look before you cross, and
 * jump when a car runs the light" instinct that keeps crowds from walking blindly
 * into the road. Complements FNpcSteering (which handles sidewalk flow) by turning
 * nearby car positions+velocities into a threat score, a curb go/no-go decision,
 * and a lateral dodge impulse.
 *
 * Direct port of the Godot `PedestrianTraffic` (RefCounted) at
 * `game/scripts/npc/pedestrian_traffic.gd`. All static, FVector-in /
 * scalar-or-FVector-out, no UObject — unit-tested headless via the parity oracle
 * (Tests/PedestrianTrafficTest.cpp, prefix GTC.NPC.Steering.PedestrianTraffic).
 *
 * Double precision throughout, to match the GDScript float math. Work happens in
 * the XZ plane (Godot Y-up); inputs are flattened with FNpcSteering::Ground().
 *
 * NOTE: no Godot->UE Z-up axis remap is baked in here — the model stays in the
 * Godot XZ frame so the ported unit tests match the oracle bit-for-bit. Porting
 * the axis convention to UE's Z-up space is a DEFERRED Wave-3 concern.
 *
 * PURE-CORE boundary: this is ONLY the per-agent traffic math, computed from
 * caller-supplied pedestrian + car positions / velocities. The Godot `cars`
 * Array of `{pos, vel}` dictionaries maps to a TArray<FCar>. Wiring this into a
 * live pedestrian agent (gathering nearby cars, blending dodge into combine,
 * gating curb-stepping) is the deferred Wave-3 adapter — NOT implemented here and
 * NOT covered by the parity tests.
 */
struct GTC_UE5_API FPedestrianTraffic
{
    /** Planar tolerance mirroring the Godot `EPSILON` constant. */
    static constexpr double Epsilon = 0.0001;

    /** One car's planar state — the UE analogue of the Godot {pos, vel} dictionary. */
    struct FCar
    {
        FVector Pos = FVector::ZeroVector;
        FVector Vel = FVector::ZeroVector;
    };

    /**
     * The most threatening car around the pedestrian. Index is -1 and Threat 0.0
     * when nothing qualifies. Mirrors the Godot {threat, index, pos, vel}
     * dictionary.
     */
    struct FThreat
    {
        double Threat = 0.0;
        int32 Index = -1;
        FVector Pos = FVector::ZeroVector;
        FVector Vel = FVector::ZeroVector;
    };

    /**
     * True when the gap between pedestrian and car is shrinking (the car is closing
     * in), rather than the car already moving away. Uses the sign of d/dt |r|^2.
     */
    static bool IsClosing(
        const FVector& PPos, const FVector& PVel, const FVector& CPos, const FVector& CVel);

    /**
     * Seconds until pedestrian and car are at their nearest point, assuming both
     * hold velocity. 0 when they are not closing (already past, or parallel), so
     * callers never react to a car that is leaving.
     */
    static double TimeToClosestApproach(
        const FVector& PPos, const FVector& PVel, const FVector& CPos, const FVector& CVel);

    /**
     * How close the car and pedestrian come (planar) if both hold velocity from now.
     */
    static double ClosestApproachDistance(
        const FVector& PPos, const FVector& PVel, const FVector& CPos, const FVector& CVel);

    /**
     * Danger of one car, 0 (safe) .. 1 (imminent broadside). Non-zero only when the
     * car is closing, will miss by less than ReactRadius, and arrives within
     * HorizonSec. Blends proximity and urgency.
     */
    static double CarThreat(
        const FVector& PPos,
        const FVector& PVel,
        const FVector& CPos,
        const FVector& CVel,
        double ReactRadius,
        double HorizonSec);

    /**
     * The most threatening car around the pedestrian. Index is -1 and Threat 0.0
     * when nothing qualifies. Order-preserving: a strict > keeps the first car on a
     * tie (mirrors the Godot loop).
     */
    static FThreat NearestThreat(
        const FVector& PPos,
        const FVector& PVel,
        const TArray<FCar>& Cars,
        double ReactRadius,
        double HorizonSec);

    /**
     * A lateral escape velocity to clear the car's path: perpendicular to the car's
     * heading, toward whichever side the pedestrian is already on (shortest way out
     * of the lane). Falls back to straight-away-from-car when the car is stopped.
     */
    static FVector DodgeVelocity(
        const FVector& PPos, const FVector& CPos, const FVector& CVel, double MaxSpeed);

    /**
     * Curb go/no-go: false when any car is closing on the pedestrian's spot, will
     * pass within DangerRadius, and gets there within SafeGapSec. The pedestrian is
     * treated as stationary (deciding whether to step off).
     */
    static bool SafeToCross(
        const FVector& PPos, const TArray<FCar>& Cars, double DangerRadius, double SafeGapSec);
};
