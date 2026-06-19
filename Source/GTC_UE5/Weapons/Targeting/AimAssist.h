// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Console-style aim assist — the thing that makes third-person shooting FEEL good
 * with a stick (and forgiving with a mouse). When the player raises a weapon, the
 * game softly acquires the most plausible target near the crosshair and gently bends
 * the aim toward it; the pull is strongest dead-centre and fades to nothing at the
 * edge of an acquisition cone, so it helps without yanking the camera off whatever
 * the player actually meant to shoot.
 *
 * Two halves, both pure:
 *   1. ACQUISITION — score every candidate by how close it sits to the crosshair,
 *      how near it is, and its priority (a charging hostile outranks a fleeing
 *      civilian), then pick the best one inside the cone (SelectBestTarget).
 *   2. ADHESION — once a soft target is held, Magnetism() says how hard to pull and
 *      ApplyAdhesion() rotates the aim a fraction of the way toward it.
 *
 * PURE-CORE: scene-free, deterministic, double precision, FVector-in / FVector-out,
 * no engine coupling. The weapon/camera adapter (GTCWeaponComponent / the player
 * look layer) supplies the aim origin + direction and a candidate list it has already
 * LOS- and faction-filtered (set bTargetable accordingly), converts the chosen
 * adhesion delta into a look/rotation nudge, and re-runs acquisition on a cadence.
 * Angles are radians, distances metres, directions need not be pre-normalised. Unit-
 * tested headless (Weapons/Targeting/Tests/AimAssistTest.cpp, prefix
 * GTC.Weapons.Targeting.AimAssist).
 */
struct GTC_UE5_API FAimCandidate
{
    /** World-space aim point on the target (chest/head as the adapter prefers). */
    FVector Position = FVector::ZeroVector;
    /** Selection weight: hostility/threat. Higher is preferred; <= 0 is never chosen. */
    double Priority = 1.0;
    /** The adapter clears this for the dead, the obscured (failed LOS), and friendlies. */
    bool bTargetable = true;
};

struct GTC_UE5_API FAimAssist
{
    /** Suggested half-angle of the acquisition cone (degrees). Adapter converts to radians. */
    static constexpr double DefaultConeHalfAngleDeg = 12.0;
    /** Suggested max acquisition range (metres). */
    static constexpr double DefaultRangeM = 60.0;
    /** Default cone half-angle in radians, for ergonomic default arguments. */
    static constexpr double DefaultConeHalfAngleRad =
        DefaultConeHalfAngleDeg * 3.14159265358979323846 / 180.0;

    /** Crosshair proximity dominates the score; range is the tie-breaker. (Sum to 1.) */
    static constexpr double AngleWeight = 0.7;
    static constexpr double RangeWeight = 0.3;

    /**
     * Angle (radians, 0..pi) between the aim direction and the direction from the aim
     * origin to TargetPos. 0 means the target is dead on the crosshair. Returns 0 for a
     * degenerate aim or a target sitting on the origin.
     */
    static double AngleToTargetRad(const FVector& AimOrigin, const FVector& AimDir, const FVector& TargetPos);

    /**
     * Desirability of a candidate, 0 if it can't be acquired (not targetable, beyond
     * MaxRangeM, or outside the cone). Otherwise blends crosshair-closeness and
     * range-closeness (each 1 at best, 0 at the limit) by Angle/RangeWeight, scaled by
     * the candidate's Priority. Higher is better; only relative values matter.
     */
    static double Score(const FVector& AimOrigin, const FVector& AimDir, const FAimCandidate& Candidate,
        double ConeHalfAngleRad = DefaultConeHalfAngleRad, double MaxRangeM = DefaultRangeM);

    /**
     * Index of the best candidate to soft-lock, or INDEX_NONE if none is acquirable.
     * Ties (exactly equal score) resolve to the earlier index, so the result is stable.
     */
    static int32 SelectBestTarget(const FVector& AimOrigin, const FVector& AimDir,
        const TArray<FAimCandidate>& Candidates,
        double ConeHalfAngleRad = DefaultConeHalfAngleRad, double MaxRangeM = DefaultRangeM);

    /**
     * How hard to pull the aim toward a target currently AngleRad off the crosshair:
     * 1 dead-centre, easing to 0 at (and beyond) the cone edge. Multiply by a per-weapon
     * adhesion gain at the call site. Monotonically decreasing in AngleRad.
     */
    static double Magnetism(double AngleRad, double ConeHalfAngleRad = DefaultConeHalfAngleRad);

    /**
     * Rotate AimDir a fraction Strength (0..1) of the way toward the target via a
     * normalised blend: 0 leaves the aim untouched, 1 points it straight at the target.
     * Returns a unit vector; falls back to the normalised aim for a degenerate target or
     * a near-antiparallel blend.
     */
    static FVector ApplyAdhesion(const FVector& AimDir, const FVector& AimOrigin,
        const FVector& TargetPos, double Strength);
};
