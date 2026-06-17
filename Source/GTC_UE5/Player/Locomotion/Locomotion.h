// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure locomotion-animation math for the character.
 *
 * Direct port of the Godot `Locomotion` (RefCounted) at
 * `game/scripts/player/locomotion.gd`. Static methods only, no scene access —
 * the same testable-core pattern as FCameraFeel / FGpsNavigation. Turns raw
 * motion (planar speed, grounded flag, vertical velocity) into an animation
 * state plus the procedural parameters a greybox rig needs to *look* like it is
 * walking: a 1D move blend, a stride phase that advances with distance, and the
 * limb-swing / vertical-bob / lean derived from it.
 *
 * Double precision throughout, to match the GDScript float math (GDScript floats
 * are 64-bit). Godot constants `TAU` / `PI` map to the UE double constants. No
 * Godot->UE Z-up axis remap is baked in here — the model stays in the Godot
 * frame so the ported unit tests match the oracle. Angle units are radians.
 *
 * PURE-CORE boundary: this is the pure blend/gait/procedural-pose decision math
 * only. Wiring these outputs onto an actual UE AnimInstance / AnimBP (pin
 * routing, BlendSpace assets, control-rig drivers) is a DEFERRED Wave-3 adapter
 * and is NOT implemented or parity-tested here.
 *
 * Parity coverage (see Tests/LocomotionTest.cpp, prefix GTC.Player.Locomotion),
 * mirroring game/tests/unit/test_locomotion.gd.
 */
struct GTC_UE5_API FLocomotion
{
    /** Animation state classification. Order mirrors the Godot enum. */
    enum class EState : uint8
    {
        Idle,
        Walk,
        Run,
        Jump,
        Fall,
        Climb,
    };

    /** Below this planar speed (m/s) the character is considered standing still. */
    static constexpr double IdleSpeedEpsilon = 0.15;

    /**
     * Stride cycles per metre travelled at a walk; scaled up toward a run so feet
     * visibly quicken with speed. Tuned for a 1.8 m capsule.
     */
    static constexpr double StrideCyclesPerMetre = 0.45;

    /**
     * Classify the current motion. Climbing and airborne states take priority over
     * ground locomotion; on the ground the planar speed picks idle/walk/run with a
     * small tolerance band around walk_speed so a steady walk doesn't flicker into
     * run on minor overshoot.
     */
    static EState StateFor(
        double PlanarSpeed,
        bool bOnFloor,
        double VerticalVelocity,
        bool bIsClimbing,
        double WalkSpeed,
        double RunSpeed);

    /**
     * Position on a 1D idle->walk->run blend axis: 0.0 standing, 0.5 at walk_speed,
     * 1.0 at run_speed, linear and clamped between.
     */
    static double MoveBlend(double PlanarSpeed, double WalkSpeed, double RunSpeed);

    /**
     * Advance the stride phase (radians, wrapped to TAU). Phase accumulates with
     * distance travelled — not raw time — so steps stay locked to the ground at any
     * speed. One full TAU cycle = a left+right stride pair.
     */
    static double AdvancePhase(double Phase, double PlanarSpeed, double Delta);

    /** Swing angle (radians) for a limb at the given stride phase. */
    static double LimbSwing(double Phase, double Amplitude);

    /** Vertical bob (metres) of the torso; oscillates at twice the stride frequency. */
    static double VerticalBob(double Phase, double Amplitude);

    /** Small side-to-side pelvis travel that follows the planted foot. */
    static double LateralSway(double Phase, double Amplitude);

    /** Roll the pelvis opposite the planted side. */
    static double PelvisRoll(double Phase, double Amplitude);

    /** Counter-roll shoulders against the pelvis. */
    static double ShoulderCounterRoll(double Phase, double Amplitude);

    /** Yaw the upper body against the stepping leg. */
    static double TorsoTwist(double Phase, double Amplitude);

    /** Tiny head pitch on each step. */
    static double HeadStepPitch(double Phase, double Amplitude);

    /** Counter-roll the head against the pelvis/shoulders. */
    static double HeadCounterRoll(double Phase, double Amplitude);

    /** Slow breathing lift for the idle pose. */
    static double IdleBreath(double IdleTime, double Amplitude);

    /** Tiny idle weight shift across the hips (slower than breathing). */
    static double IdleWeightShift(double IdleTime, double Amplitude);

    /** Neck compensation during idle breathing. */
    static double IdleHeadPitch(double IdleTime, double Amplitude);

    /** Foot yaw: opens the toe slightly outward plus a small extra swing-phase turn. */
    static double FootToeOut(double Side, double SwingNorm, double Base, double Swing);

    /** Roll the foot subtly as weight moves across it (sign per side). */
    static double FootBank(double Side, double SwingNorm, double Amplitude);

    /** Lean into a change in facing direction. */
    static double TurnLean(double AngularVelocity, double Reference, double MaxLean);

    /** Landing compression from the downward speed at floor contact. */
    static double LandingCompression(double PreviousVerticalVelocity, double Reference, double MaxCompression);

    /** Forward lean (radians) into signed planar acceleration. */
    static double LeanAngle(double Accel, double AccelReference, double MaxLean);

    // --- Articulated gait: two-bone limb flexion -----------------------------

    /** Knee flexion magnitude (always >= 0); peaks near phase = 3*PI/2. */
    static double KneeFlex(double Phase, double Amplitude, double StanceFlex = 0.12);

    /** Ankle pitch (signed), a quarter-cycle ahead of the hip. */
    static double AnklePitch(double Phase, double Amplitude);

    /** Elbow flexion for a swinging arm (always >= 0). arm_phase is legs' + PI. */
    static double ElbowFlex(double ArmPhase, double Amplitude, double BaseBend = 0.35);

    /** Knee flex from the thigh's current normalised swing (hip_angle / hip_amp). */
    static double KneeFlexFromSwing(double SwingNorm, double Amplitude, double StanceFlex = 0.12);

    /** Elbow flex from the upper arm's current normalised swing (shoulder_angle / arm_amp). */
    static double ElbowFlexFromSwing(double SwingNorm, double Amplitude, double BaseBend = 0.35);
};
