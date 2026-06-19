// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Arcade driving FEEL — the grip/slip/drift math that makes a car snap into corners,
 * step its tail out under the handbrake, and hold a satisfying slide. The Chaos vehicle
 * gives us a physical chassis; this is the thin arcade layer on top that decides how
 * much of the car's sideways velocity to KEEP (a drift) versus KILL (a grippy corner),
 * so the handling reads like a game and not a simulator.
 *
 * It closes a seam the rest of the vehicle stack was already built for: FVehicleCondition
 * hands out GripFactor() (tyre/wear/wet) and TopSpeedFactor() with a comment that they
 * "multiply into VehicleHandling" — this is that VehicleHandling. Feed GripFactor into
 * LateralRetention and TopSpeedFactor into EffectiveTopSpeed.
 *
 * Two halves:
 *   - Static handling math: SlipAngleRad, LateralRetention, DriftFactor, ApplyGrip,
 *     EffectiveTopSpeed. Scene-free, FVector-in / FVector-out, deterministic.
 *   - A stateful FDriftScorer (the GTA-style stunt reward): accumulate points while
 *     sliding, grow a combo multiplier the longer you hold it, Bank() on a clean exit or
 *     Wipe() on a spin-out / crash.
 *
 * PURE-CORE: double precision, no engine coupling. The Car adapter (a ChaosVehicle pawn)
 * supplies the chassis Forward/Right (ground-plane unit vectors) and Velocity each
 * physics frame, replaces the velocity with ApplyGrip(...), reads DriftFactor for the
 * camera/FX and the scorer, and caps speed at EffectiveTopSpeed. Distances/speeds are in
 * the adapter's own units (cm/s is fine — the math is scale-free); angles are radians.
 * Unit-tested headless (Vehicles/Handling/Tests/VehicleHandlingTest.cpp, prefix
 * GTC.Vehicles.Handling).
 */
struct GTC_UE5_API FVehicleHandling
{
    /** Below this slip angle the car is considered to be tracking straight (no drift). */
    static constexpr double SlipDeadzoneRad = 0.087;  // ~5 degrees
    /** At/above this slip angle the car is fully sideways (drift factor saturates). */
    static constexpr double FullDriftSlipRad = 0.611; // ~35 degrees
    /** Below this speed there is no meaningful drift, however the car is pointed. */
    static constexpr double MinDriftSpeed = 3.0;

    /** Lateral velocity retained in a normal grippy corner (most slide is killed). */
    static constexpr double GripRetention = 0.10;
    /** Lateral velocity retained under the handbrake (the tail steps right out). */
    static constexpr double HandbrakeRetention = 0.85;

    /**
     * Angle (radians, 0..pi) between where the car is POINTED (Forward) and where it is
     * actually MOVING (Velocity). 0 means tracking true; a big angle means it's sliding
     * sideways. Returns 0 for a parked car or a degenerate Forward.
     */
    static double SlipAngleRad(const FVector& Forward, const FVector& Velocity);

    /**
     * Fraction of sideways (lateral) velocity the tyres KEEP this frame: low when gripping
     * (the slide is scrubbed off), high under the handbrake (it slides). Worse GripFactor
     * (0..1; 1 = fresh tyres on dry road) lets the back end step out more. Result 0..1.
     */
    static double LateralRetention(bool bHandbrake, double GripFactor);

    /**
     * How hard the car is drifting, 0..1: 0 when tracking straight or below MinDriftSpeed,
     * ramping with slip from SlipDeadzoneRad to 1 at FullDriftSlipRad. Drives drift FX,
     * camera lean, and the scorer.
     */
    static double DriftFactor(double SlipAngleRadians, double Speed);

    /**
     * Re-split Velocity into forward + lateral (about the car's Forward/Right ground
     * basis), scale the lateral part by LateralRetention, and recombine — the actual act
     * of gripping or sliding. Any component outside the ground basis (e.g. vertical) is
     * preserved. Forward/Right should be unit and perpendicular; degenerate input returns
     * Velocity unchanged.
     */
    static FVector ApplyGrip(const FVector& Velocity, const FVector& Forward, const FVector& Right,
        double LateralRetentionFraction);

    /** Top speed after the per-vehicle TopSpeedFactor (wear/fuel) caps it. Both clamped >= 0. */
    static double EffectiveTopSpeed(double BaseTopSpeed, double TopSpeedFactor);

    /**
     * The drift-combo scorer — GTA-style "hold a slide for points". Stateful; the Car
     * adapter Update()s it each frame with the current DriftFactor + speed, then Bank()s
     * on a clean recovery or Wipe()s on a spin-out / impact.
     */
    struct GTC_UE5_API FDriftScorer
    {
        /** Below this drift factor the car isn't sliding enough to score. */
        static constexpr double MinDriftFactor = 0.2;
        /** Combo multiplier ceiling, however long the slide is held. */
        static constexpr double MaxMultiplier = 5.0;
        /** Combo multiplier growth per continuous second of drift. */
        static constexpr double MultiplierPerSec = 0.5;
        /** Raw points per unit of (driftFactor * speed * second) held. */
        static constexpr double ScoreRate = 0.1;

        double Score = 0.0;       // raw points accrued in the current run (pre-multiplier)
        double Multiplier = 1.0;  // current combo multiplier (1..MaxMultiplier)
        double DriftTime = 0.0;   // seconds of continuous drift in the current run

        /** Advance one frame. Accrues score + grows the multiplier only while actually
         *  drifting (DriftFactor >= MinDriftFactor, Speed > 0, Dt > 0); otherwise a no-op
         *  that holds the run so a momentary straighten doesn't cost the combo. */
        void Update(double DriftFactor, double Speed, double Dt);

        /** Clean exit: returns the banked payout (Score * Multiplier) and resets the run. */
        double Bank();

        /** Spin-out / crash: returns the score lost and resets the run to nothing. */
        double Wipe();

        /** Points the current run would pay if banked right now. */
        double PendingPayout() const { return Score * Multiplier; }

        bool IsActive() const { return Score > 0.0; }
    };
};
