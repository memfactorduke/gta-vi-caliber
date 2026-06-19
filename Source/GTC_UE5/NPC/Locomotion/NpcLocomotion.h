// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../Decision/NpcBrain.h"
#include "../Steering/PedestrianTraffic.h"

/**
 * The per-tick locomotion glue: the pure-core layer that the rest of the NPC
 * stack always deferred. NpcSteering gives sidewalk flow, PedestrianTraffic gives
 * the look-before-you-cross instinct, NpcBrain gives the wander/flee state — but
 * nothing fused them into "given everything I know this frame, which way do I
 * actually step, and how fast". This does, and it does it as scene-free math so
 * the decision is unit-testable headless (Tests/NpcLocomotionTest.cpp, prefix
 * GTC.NPC.Locomotion.NpcLocomotion).
 *
 * It also closes the one axis-convention gap the steering/traffic headers flagged
 * as a "DEFERRED Wave-3 concern". The ported math lives in the Godot XZ plane
 * (Y-up, ground = X/Z). Unreal's ground plane is X/Y with Z up. Rather than
 * re-deriving the tested math in a new frame, we keep the math in its proven XZ
 * frame and remap at this boundary: ToPlane()/FromPlane() are the only place the
 * two conventions meet. The live AGTCCitizen adapter converts UE world vectors in,
 * composes here, and converts the desired velocity back out for movement input.
 *
 * Double precision throughout, matching the upstream GDScript float math.
 */

/**
 * Tuning for FNpcLocomotion::DesiredVelocity. Top-level (not nested) so it can be
 * used as a defaulted argument — a nested struct's default member initializers
 * aren't a complete-class context for a default argument in the same class.
 * Defaults are sensible city-pedestrian values (centimetres = UE world units);
 * the adapter may override per archetype.
 */
struct GTC_UE5_API FNpcLocomotionParams
{
    /** Ease-to-a-halt radius for Arrive at the goal. */
    double SlowRadius = 180.0;
    /** Inside this of the goal the NPC is "there" and stops. */
    double ArriveRadius = 40.0;
    /** Personal-space radius feeding NpcSteering::Separation. */
    double SeparationRadius = 120.0;
    /** Range within which a car is considered for the dodge reflex. */
    double CarReactRadius = 350.0;
    /** Look-ahead horizon (seconds) for car threat assessment. */
    double CarHorizonSec = 2.0;
    /** A car threat at/above this fully engages the dodge. */
    double DodgeThreshold = 0.15;
    /** Steering blend weights: goal seek/arrive, neighbour separation. */
    double GoalWeight = 1.0;
    double SeparationWeight = 1.3;
    /** Dodge weight is scaled by the live threat, up to this ceiling. */
    double DodgeWeight = 2.5;
};

struct GTC_UE5_API FNpcLocomotion
{
    /** Convenience alias so existing call sites can say FNpcLocomotion::FParams. */
    using FParams = FNpcLocomotionParams;

    /** Planar tolerance, consistent with FNpcSteering::Eps / FPedestrianTraffic::Epsilon. */
    static constexpr double Eps = 0.0001;

    // --- Axis convention bridge (UE Z-up ground <-> Godot XZ math plane) -------

    /**
     * Map an Unreal world vector (ground = X/Y, Z up) into the math plane the
     * steering/traffic/brain code operates in (ground = X/Z). The UE up axis (Z)
     * is dropped — pedestrians steer on the ground — and UE Y becomes plane Z.
     */
    static FVector ToPlane(const FVector& UEWorld)
    {
        return FVector(UEWorld.X, 0.0, UEWorld.Y);
    }

    /**
     * Inverse of ToPlane: map a math-plane vector (X/Z) back to a UE ground vector
     * (X/Y, Z = 0). Use on the composed desired velocity before handing it to
     * CharacterMovement.
     */
    static FVector FromPlane(const FVector& Plane)
    {
        return FVector(Plane.X, Plane.Z, 0.0);
    }

    // --- Queries ----------------------------------------------------------------

    /** Planar distance between two UE world points (drops Z, the UE up axis). */
    static double GroundDistance(const FVector& A, const FVector& B);

    /**
     * True once within Tolerance (planar) of Goal — the "I'm here, pick a new
     * destination" signal for the adapter. Inputs are UE world positions.
     */
    static bool HasArrived(const FVector& Pos, const FVector& Goal, double Tolerance);

    /**
     * A far-off point directly away from a threat, suitable as a transient Flee
     * goal so the same Seek/Arrive pipeline drives running away. Inputs/outputs in
     * UE world space; Distance is how far ahead to plant the point.
     */
    static FVector FleeGoal(const FVector& SelfPos, const FVector& ThreatPos, double Distance);

    /**
     * The whole point of the file: the desired ground velocity for this NPC this
     * frame, ready to feed movement input.
     *
     *   State        - brain FSM state (Idle => stand still, Wander/Flee => move)
     *   SelfPos/Goal - UE world positions (the adapter plants a flee goal for Flee)
     *   Neighbors    - UE world positions of nearby NPCs (for separation)
     *   Cars         - nearby vehicles in UE world space (pos + velocity)
     *   WalkSpeed/RunSpeed - this archetype's locomotion speeds (cm/s)
     *   Params       - tuning
     *
     * Returns a UE world-space velocity whose magnitude never exceeds the state's
     * target speed. Idle (or a zero-length result) yields the zero vector, which
     * the adapter treats as "no movement input this frame".
     *
     * All neighbour/car/self math is done in the XZ plane via ToPlane(); the result
     * is mapped back with FromPlane(), so this function — and only this function —
     * owns the axis bridge for the live path.
     */
    static FVector DesiredVelocity(
        FNpcBrain::EState State,
        const FVector& SelfPos,
        const FVector& Goal,
        const TArray<FVector>& Neighbors,
        const TArray<FPedestrianTraffic::FCar>& Cars,
        double WalkSpeed,
        double RunSpeed,
        const FNpcLocomotionParams& Params = FNpcLocomotionParams());
};
