// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure pedestrian AI: wander between points, flee from threats.
 *
 * Direct port of the the reference `NpcBrain` (RefCounted) at
 * `game/scripts/npc/npc_brain.gd`. Static helpers only — no scene access, RNG
 * injected by the caller (the two [0,1) wander samples) — so behaviour is
 * deterministic and unit-tested (Tests/NpcBrainTest.cpp, prefix
 * GTC.NPC.Decision.NpcBrain). Spatial math is on the XZ plane (Y ignored),
 * matching the the reference source; NO Z-up remap.
 *
 * PURE-CORE boundary: the moment-to-moment locomotion wiring (the Pedestrian
 * node owning the mutable target/timers, calling these each frame) is the
 * DEFERRED Wave-3 adapter and is NOT covered by the parity tests. Computed
 * precision is `double` to match the reference implementation.
 */
struct GTC_UE5_API FNpcBrain
{
    /** Behaviour states. the reference enum ordinal order preserved. */
    enum class EState : uint8
    {
        Idle,
        Wander,
        Flee,
    };

    /**
     * A random planar point within `Radius` of `Origin`, area-uniform from two
     * independent [0, 1) samples (`URadius`, `UAngle`) so wander targets don't
     * clump at the centre.
     */
    static FVector WanderTarget(const FVector& Origin, double Radius, double URadius, double UAngle);

    /** Planar (XZ) distance between two points. */
    static double PlanarDistance(const FVector& A, const FVector& B);

    /** True once within `Tolerance` (planar) of the target. */
    static bool Arrived(const FVector& Pos, const FVector& Target, double Tolerance);

    /** Planar unit direction from A to B, or ZeroVector if effectively coincident. */
    static FVector PlanarDir(const FVector& A, const FVector& B);

    /** Direction that runs directly away from a threat (planar, unit). */
    static FVector FleeDir(const FVector& SelfPos, const FVector& ThreatPos);

    /**
     * Direction that chases a target down (planar, unit) — the inverse of
     * FleeDir. Used by police pursuit.
     */
    static FVector PursueDir(const FVector& SelfPos, const FVector& TargetPos);

    /**
     * Next behaviour state with hysteresis: a nearby active threat forces Flee;
     * once fleeing, keep running until the threat is gone or beyond CalmRadius,
     * so the pedestrian doesn't flip-flop at the boundary. Idle/Wander
     * alternation itself is left to the node's timers.
     */
    static EState NextState(
        EState Current,
        bool bThreatActive,
        double ThreatDistance,
        double FleeRadius,
        double CalmRadius);

    /** Target move speed for a state, given this pedestrian's walk/run speeds. */
    static double SpeedFor(EState State, double WalkSpeed, double RunSpeed);
};
