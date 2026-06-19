// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure full-body rig / IK math layered on top of FLocomotion.
 *
 * FLocomotion turns motion into the procedural *gait* (stride swing, bob, the
 * per-limb hip/knee/ankle/shoulder/elbow articulation of a walk or run cycle).
 * FPoseRig is the partner layer that the deferred Control-Rig / AnimInstance
 * adapter consumes to:
 *
 *   - hold *postures* that are not part of the locomotion cycle (sitting, a
 *     standing relieving stance) as a set of named joint targets, and
 *   - cross-fade smoothly between any posture and the moving gait, so a sit or a
 *     stand-up never snaps — the "smooth" the player feels comes from the
 *     frame-rate-independent ApproachWeight / SpringDamp primitives here, and
 *   - solve the two-bone IK for the legs (foot planting) and the arms (hand IK
 *     targets), plus the head/neck look-at that aims the face at a target.
 *
 * Same testable-core contract as FLocomotion / FAnimRouter / FCameraFeel: static
 * methods only, no scene access, no UObject, double precision throughout. Angle
 * units are radians; lengths are metres. The middle-joint IK convention is the
 * interior angle between the two bones (0 = fully folded, PI = straight).
 *
 * PURE-CORE boundary: wiring these outputs onto an actual UE Control Rig / AnimBP
 * (bone transforms, IK targets, posture blend pins) is a DEFERRED editor-side
 * adapter, NOT implemented here — only the decision/IK math is, and it is fully
 * automation-tested.
 *
 * Test coverage: Tests/PoseRigTest.cpp, prefix GTC.Player.PoseRig.
 */
struct GTC_UE5_API FPoseRig
{
    /** A posture layered over the locomotion gait. Order is stable for blending. */
    enum class EPosture : uint8
    {
        Locomotion, // defer to FLocomotion's moving gait / idle
        Sit,        // seated: hips + knees flexed ~90, pelvis dropped, hands on thighs
        Piss,       // standing relieving stance: knees soft, slight forward lean, hands to front
    };

    // --- Smoothing primitives (the "smooth" in smooth transitions) --------------

    /**
     * Frame-rate-independent exponential approach of Current toward Target. Rate is
     * the responsiveness per second (larger = snappier). Equivalent to an
     * exponential decay of the remaining error: at Rate*Delta == ln(2) exactly half
     * the gap is closed. Monotonic and never overshoots; Delta <= 0 returns Current
     * unchanged. This is the cheap default for blending a posture weight in [0,1].
     */
    static double ApproachWeight(double Current, double Target, double Rate, double Delta);

    /**
     * Exact critically-damped spring step toward Target, parameterised by HalfLife
     * (seconds for the error to halve). Updates Velocity by reference and returns
     * the new value. Critically damped => eases in and out with no overshoot, the
     * smoothest natural-feeling settle for a sit-down / stand-up blend or a
     * pelvis/hand target that should glide rather than snap. Delta <= 0 leaves both
     * value and velocity untouched.
     */
    static double SpringDamp(double Current, double Target, double& Velocity, double HalfLife, double Delta);

    // --- Stance poses: named joint targets --------------------------------------

    /**
     * A static full-body posture as named joint targets. Sign convention: pitch is
     * forward-positive flexion; knee/elbow flex are magnitudes (>= 0); PelvisDrop is
     * a vertical offset in metres (<= 0 lowers the pelvis); HandToFront is a 0..1
     * weight gathering the hands toward the body's front centre.
     */
    struct FStancePose
    {
        double HipPitch = 0.0;      // thigh flexion (forward +)
        double KneeFlex = 0.0;      // knee bend magnitude
        double AnklePitch = 0.0;    // foot pitch
        double SpinePitch = 0.0;    // torso lean (forward +, recline -)
        double NeckPitch = 0.0;     // head pitch (look down +)
        double ShoulderPitch = 0.0; // upper-arm raise
        double ElbowFlex = 0.0;     // elbow bend magnitude
        double PelvisDrop = 0.0;    // pelvis vertical offset, metres (<= 0 lowers)
        double HandToFront = 0.0;   // 0..1 hands gathered to front centre
    };

    /** The target joint set for a posture. Locomotion returns the neutral base the
     *  gait articulates around (elbows carry the same base bend FLocomotion uses). */
    static FStancePose StancePose(EPosture Posture);

    /** Per-field linear blend A->B at weight W (clamped to [0,1]); W=0 => A, W=1 => B. */
    static FStancePose BlendPose(const FStancePose& A, const FStancePose& B, double W);

    // --- Two-bone IK (legs and arms) --------------------------------------------

    struct FTwoBoneSolution
    {
        /** Interior bend at the middle joint (knee/elbow): 0 folded, PI straight. */
        double JointAngle = 0.0;
        /** False when the target was beyond full extension or inside the dead zone
         *  and had to be clamped onto the reachable shell. */
        bool bReachable = true;
    };

    /**
     * Law-of-cosines two-bone IK. Given the upper and lower bone lengths and the
     * straight-line distance from the root to the IK target, return the middle-joint
     * angle that lands the effector on the target. TargetDist is clamped to the
     * reachable range [|Upper-Lower|, Upper+Lower]; bReachable reports whether it had
     * to be clamped. Degenerate (non-positive) bone lengths return a straight,
     * unreachable joint.
     */
    static FTwoBoneSolution SolveTwoBoneIK(double UpperLen, double LowerLen, double TargetDist);

    // --- Foot ground plant ------------------------------------------------------

    /**
     * Vertical foot-IK offset that keeps a planted foot on detected ground. Positive
     * raises the foot toward ground that sits above the hip's nominal foot drop;
     * negative lowers it into a dip. The raw offset (GroundHeight - nominal foot
     * height, where nominal = HipHeight - LegLength) is clamped to +/- MaxLift so a
     * bad trace can't hyperextend the leg. Non-positive LegLength returns 0.
     */
    static double FootPlantOffset(double HipHeight, double GroundHeight, double LegLength, double MaxLift);

    // --- Head / face look-at ----------------------------------------------------

    struct FAimAngles
    {
        double Yaw = 0.0;   // horizontal turn toward target, radians (clamped)
        double Pitch = 0.0; // vertical tilt toward target, radians (clamped)
    };

    /**
     * Yaw and pitch the head must add to rotate from Forward toward TargetDir, each
     * independently clamped to +/- MaxYaw / +/- MaxPitch so the neck never breaks.
     * Vectors are in the same Godot-frame convention FAnimRouter uses (yaw from
     * atan2(X, Z)); they need not be normalised. A zero-length Forward or TargetDir
     * yields a neutral (0,0) aim.
     */
    static FAimAngles LookAtAngles(const FVector& Forward, const FVector& TargetDir, double MaxYaw, double MaxPitch);
};
