// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure steering math for pedestrians — the locomotion side of NPC life. Turns a
 * goal position and a handful of neighbours into a desired velocity, so crowds
 * flow along sidewalks, ease to a stop at destinations, and don't telescope into
 * each other. Boids-lite: seek, arrive, separation, path following.
 *
 * Direct port of the Godot `NpcSteering` (RefCounted) at
 * `game/scripts/npc/npc_steering.gd`. All static, FVector-in / FVector-out, no
 * UObject — unit-tested headless via the parity oracle
 * (Tests/NpcSteeringTest.cpp, prefix GTC.NPC.Steering.NpcSteering).
 *
 * Double precision throughout, to match the GDScript float math. Work happens in
 * the XZ plane (Godot Y-up); `Ground(V)` flattens to `FVector(V.X, 0, V.Z)`.
 *
 * NOTE: no Godot->UE Z-up axis remap is baked in here — the model stays in the
 * Godot XZ frame so the ported unit tests match the oracle bit-for-bit. Porting
 * the axis convention to UE's Z-up space is a DEFERRED Wave-3 concern.
 *
 * PURE-CORE boundary: this is ONLY the per-agent steering math, computed from
 * caller-supplied positions / velocities / neighbour lists. Mass crowd-steering
 * integration (feeding the result to a CharacterMovementComponent, gravity,
 * collision, neighbour gathering) is the deferred Wave-3 adapter — NOT
 * implemented here and NOT covered by the parity tests.
 */
struct GTC_UE5_API FNpcSteering
{
    /** Planar tolerance mirroring the Godot 0.0001 zero-vector / radius guards. */
    static constexpr double Eps = 0.0001;

    /** Drop the vertical component — pedestrians steer on the ground (XZ) plane. */
    static FVector Ground(const FVector& V);

    /**
     * Desired velocity heading straight at Target at full speed. Zero when already
     * on top of it (avoids a NaN from normalising a zero vector).
     */
    static FVector Seek(const FVector& Pos, const FVector& Target, double MaxSpeed);

    /**
     * Like Seek, but ramps speed down inside SlowRadius so the NPC eases to a halt
     * on its mark instead of jittering across it. Stops within ArriveRadius.
     */
    static FVector Arrive(
        const FVector& Pos,
        const FVector& Target,
        double MaxSpeed,
        double SlowRadius,
        double ArriveRadius = 0.25);

    /**
     * Repulsion from nearby NPCs, weighted by closeness (closer = stronger push),
     * so personal space is respected without anyone phasing through anyone.
     * Inverse-falloff magnitude is preserved, only clamped (limit_length) when a
     * dense crowd's combined push would exceed MaxSpeed.
     */
    static FVector Separation(
        const FVector& Pos, const TArray<FVector>& Neighbors, double Radius, double MaxSpeed);

    /**
     * Blend weighted steering vectors and clamp the result to MaxSpeed, so no
     * combination of urges can launch an NPC faster than it can walk. Pairs are
     * taken up to the shorter of the two arrays.
     */
    static FVector Combine(
        const TArray<FVector>& Vectors, const TArray<double>& Weights, double MaxSpeed);

    /**
     * Which waypoint to head for: advance past any already reached within
     * AcceptRadius. Returns the new index (clamped to the last point), so a path
     * walker just calls this each tick and seeks Waypoints[Index].
     */
    static int32 AdvanceWaypoint(
        const FVector& Pos, const TArray<FVector>& Waypoints, int32 Index, double AcceptRadius);
};
