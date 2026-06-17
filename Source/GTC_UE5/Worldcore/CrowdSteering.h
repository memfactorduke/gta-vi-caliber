// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure, runtime-free boids steering for the worldcore crowd/traffic sim. Given an
 * agent and its neighbours, produce a steering force from the three classic
 * flocking behaviours — separation, alignment, cohesion — combined and clamped,
 * plus seek/arrive/obstacle-avoidance helpers.
 *
 * Direct C++->C++ port of the worldcore core
 * engine/src/worldcore/crowd_steering_core.h (namespace worldcore_crowd). All
 * static, FVector2D-in/FVector2D-out, no UObject — unit-tested headless via the
 * parity oracle (Tests/CrowdSteeringTest.cpp, prefix GTC.Worldcore.CrowdSteering).
 *
 * Vector type: the core's plain Vec2{x, z} maps to FVector2D with X<-x, Y<-z.
 * The original works in a planar XZ frame; no axis remap is baked in — the model
 * stays in the core's frame so the ported tests match the oracle. Double
 * precision throughout (FVector2D is double under UE_LARGE_WORLD_COORDINATES).
 *
 * PURE-CORE boundary: this is the pure steering-force math only. Mass / actor
 * integration (applying these forces to agent velocity/position each tick) is a
 * DEFERRED Wave-3 adapter — the core stays pure, operating on caller-supplied
 * agent state, and is NOT covered by the parity tests.
 */
struct GTC_UE5_API FCrowdSteering
{
    /** Length-clamp: zero force when MaxLen <= 0 (no budget — never flip direction). */
    static FVector2D ClampLen(const FVector2D& A, double MaxLen);

    /**
     * Steer away from neighbours within Radius, weighted by 1/distance so closer
     * agents push harder. Averaged over the contributing neighbours.
     */
    static FVector2D Separation(const FVector2D& SelfPos, const TArray<FVector2D>& Neighbors, double Radius);

    /** Steer toward the average position (centroid) of neighbours. */
    static FVector2D Cohesion(const FVector2D& SelfPos, const TArray<FVector2D>& Neighbors);

    /** Steer to match the average heading (velocity) of neighbours. */
    static FVector2D Alignment(const TArray<FVector2D>& NeighborVels);

    /** Weighted sum of the three behaviours, clamped to MaxForce. */
    static FVector2D Combine(const FVector2D& Sep, const FVector2D& Ali, const FVector2D& Coh,
        double WSep, double WAli, double WCoh, double MaxForce);

    /** Seek: steer toward Target at full speed (desired velocity minus current). */
    static FVector2D Seek(const FVector2D& SelfPos, const FVector2D& SelfVel, const FVector2D& Target, double MaxSpeed);

    /**
     * Arrive: like seek, but ramp speed down linearly inside SlowRadius so the
     * agent settles at the target instead of orbiting it. Brakes when on top of it.
     */
    static FVector2D Arrive(const FVector2D& SelfPos, const FVector2D& SelfVel, const FVector2D& Target,
        double SlowRadius, double MaxSpeed);

    /**
     * Avoid circular obstacles: outward push from each obstacle the agent is inside
     * (radius + margin), weighted by penetration depth. Positions and Radii are
     * parallel; the shorter length wins so a malformed pair can't read past the end.
     */
    static FVector2D AvoidObstacles(const FVector2D& SelfPos, const TArray<FVector2D>& Positions,
        const TArray<double>& Radii, double Margin);
};
