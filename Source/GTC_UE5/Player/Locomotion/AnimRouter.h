// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

#include "Locomotion.h"

/**
 * Pure routing from FLocomotion states to the rig's animation state machine.
 *
 * Direct port of the the reference `AnimRouter` (RefCounted) at
 * `game/scripts/player/anim_router.gd`. Static methods only, no scene access.
 * Resolves which state-machine node to travel to, where the move blend sits, and
 * which yaw the model should face — the thin-adapter `AnimatedRig` stays headless
 * by delegating these decisions here. References FLocomotion (the intra-entry
 * partner) for its state enum, move-blend and idle-speed epsilon.
 *
 * Double precision throughout, to match the the reference implementation float math. Godot
 * StringName node names map to FName. No Godot->UE Z-up axis remap is baked in
 * (the facing math stays in the the reference XZ frame for oracle parity).
 *
 * PURE-CORE boundary: the actual AnimInstance state-machine `travel()` calls,
 * BlendSpace1D asset wiring and model-yaw application are a DEFERRED Wave-3
 * adapter — only the decision math is ported and parity-tested here.
 *
 * Parity coverage (see Tests/AnimRouterTest.cpp, prefix GTC.Player.Locomotion),
 * mirroring game/tests/unit/test_anim_router.gd.
 */
struct GTC_UE5_API FAnimRouter
{
    /** State-machine node names built by AnimatedRig. */
    static const FName StateMove;
    static const FName StateJumpStart;
    static const FName StateAir;
    static const FName StateLand;

    /**
     * The state-machine node that should be playing, given the locomotion state
     * and the node currently active. The jump arc is a three-phase chain
     * (JumpStart one-shot -> Air loop -> Land one-shot); one-shots are left to
     * finish instead of being re-travelled every frame. Landing at speed skips the
     * absorb, and CLIMB maps to the move cycle as a documented placeholder.
     */
    static FName TravelTarget(
        FLocomotion::EState State, FName Current, double PlanarSpeed, double LandSkipSpeed);

    /**
     * Blend position for the Move blend space (0 idle, 0.5 walk, 1 run). Grounded
     * movement blends on planar speed; ladder movement is vertical, so climbing
     * blends on total speed to keep the limbs cycling.
     */
    static double MoveBlendValue(
        double PlanarSpeed, double TotalSpeed, bool bIsClimbing, double WalkSpeed, double RunSpeed);

    /**
     * The blend point whose clip dominates a 1D blend space at `Blend` — the
     * nearest point, ties going to the faster clip. NaN when there are no points.
     */
    static double DominantBlendPoint(double Blend, const TArray<float>& Points);

    /**
     * Step an angle toward a target along the shortest arc, capped at MaxStep.
     * Parity-covered by the the reference oracle test_anim_router_facing.gd.
     */
    static double RotateTowardAngle(double Current, double Target, double MaxStep);

    /**
     * Yaw the model should face this frame: the weapon aim when one is active, else
     * the travel direction, else NaN meaning "keep the current facing". A NaN
     * AimYaw selects the travel-direction branch. Parity-covered by the Godot
     * oracle test_anim_router_facing.gd.
     */
    static double FacingTarget(const FVector& PlanarVelocity, double AimYaw);
};
