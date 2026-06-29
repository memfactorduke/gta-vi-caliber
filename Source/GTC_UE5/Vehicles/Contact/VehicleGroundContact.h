// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Ground/landing contact for KINEMATIC vehicles — the floor-clamp + touchdown brain a
 * pawn needs once it integrates a pure-core velocity itself (the Wings & Waves stack
 * is Chaos-free, like AGTCPoliceHelicopter). The flight cores (FHelicopterFlight /
 * FFixedWingFlight) own the velocity and FBoatHandling owns planar motion, but nothing
 * decides "have we reached the ground, should the gear stop sinking, and how hard did
 * we hit". FVehicleGroundContact is that decision, and nothing about the engine: the
 * adapter casts a downward trace, hands the floor Z + the body's current Z/vertical
 * speed in, and gets back whether we're airborne or grounded, where to clamp so the
 * gear/keel never penetrates, and — on the airborne->grounded edge — how soft the
 * landing was (so a gentle set-down is fine, a fast one jolts, a slammed-in one wrecks).
 *
 * It serves both halves of the feature: a helicopter/plane landing on terrain, and a
 * boat beaching on the shore (a hull point finding solid ground above the water line).
 *
 * Hysteresis keeps it stable: once grounded the body is held at its resting height and
 * only counts as airborne again after it climbs clear by LiftoffMargin, so a hovering
 * craft sitting on its skids doesn't flicker between states frame to frame.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject —
 * stateless static helpers (the FVehicleChaseCamera shape; the caller owns the prior
 * EContact for the edge). The downward trace, the SetActorLocation clamp, and any
 * crash FX are the DEFERRED adapter and are NOT covered by the tests.
 *
 * Frame / units: Z is up (these are the Z-up player cores — NO Godot Y<->Z remap, unlike
 * AI/Police/GTCPoliceHelicopter.cpp). Positions/heights are cm, vertical speed cm/s
 * (negative = descending). Unit-tested headless
 * (Vehicles/Contact/Tests/VehicleGroundContactTest.cpp, prefix GTC.Vehicles.Contact).
 */
struct GTC_UE5_API FVehicleGroundContact
{
    /** Per-airframe / per-hull tuning the adapter passes in. */
    struct FParams
    {
        /** Resting height of the body origin above the ground when settled (cm) — the
         *  landing-gear / keel clearance. */
        double GearHeightCm = 150.0;
        /** Descent speed (cm/s) at/below which a touchdown is Soft (a clean set-down). */
        double SoftLandingSpeedCm = 300.0;
        /** Descent speed (cm/s) at/above which a touchdown is a Crash; between Soft and
         *  this it is Hard (jolting but survivable). Clamped to >= SoftLandingSpeedCm. */
        double CrashLandingSpeedCm = 800.0;
        /** How far (cm) above the resting height the body must climb before it counts as
         *  airborne again — hysteresis so a craft on its skids doesn't flicker. */
        double LiftoffMarginCm = 25.0;
    };

    /** How the body is meeting the ground this tick. */
    enum class EContact : uint8
    {
        Airborne, // clear of the ground
        Grounded, // resting / taxiing on the ground (held at the resting height)
    };

    /** Quality of a touchdown — only ever set on the airborne->grounded edge. */
    enum class ETouchdown : uint8
    {
        None,  // not a touchdown this tick
        Soft,  // gentle — a clean landing
        Hard,  // jolting — survivable, take some damage
        Crash, // slammed in — wreck it
    };

    /** Outcome of one contact evaluation. */
    struct FResult
    {
        EContact Contact = EContact::Airborne;
        ETouchdown Touchdown = ETouchdown::None;
        /** True when the body should be clamped onto the ground (gear not penetrating). */
        bool bClampToGround = false;
        /** World Z (cm) to clamp the body origin to when bClampToGround. */
        double ClampZCm = 0.0;
        /** 0..1 touchdown hardness: 0 at/below the soft speed, 1 at/above the crash speed.
         *  0 when this tick is not a touchdown. */
        double ImpactHardness01 = 0.0;
    };

    /** Height of the body origin above the traced floor (cm). */
    static double AltitudeAGL(double BodyZCm, double GroundZCm);

    /** World Z (cm) the body origin rests at when sitting on GroundZCm. */
    static double RestZCm(double GroundZCm, const FParams& Params);

    /**
     * Classify a touchdown purely from how fast the body was coming down. `DescentSpeedCm`
     * is the downward speed magnitude (>= 0; pass max(0, -VerticalSpeed)). A non-positive
     * speed is the gentlest possible Soft.
     */
    static ETouchdown ClassifyTouchdown(double DescentSpeedCm, const FParams& Params);

    /** 0..1 hardness of a touchdown at `DescentSpeedCm` (the FResult::ImpactHardness01 curve). */
    static double ImpactHardness01(double DescentSpeedCm, const FParams& Params);

    /**
     * Resolve ground contact for this tick.
     *  - `BodyZCm`         : the body origin's current world Z (cm), after this frame's move.
     *  - `bGroundHit`      : did a downward trace find a floor? (false => nothing below => airborne).
     *  - `GroundZCm`       : that floor's world Z (cm); ignored when !bGroundHit.
     *  - `VerticalSpeedCm` : signed vertical velocity (cm/s); negative = descending.
     *  - `Prev`            : last tick's EContact, for landing/takeoff hysteresis.
     *
     * Returns the contact state, the clamp (set whenever grounded, so the gear/keel rests
     * on the floor instead of sinking — this also lifts a body that spawned below ground),
     * and, on the airborne->grounded edge only, the touchdown quality/hardness from the
     * descent speed.
     */
    static FResult Evaluate(double BodyZCm, bool bGroundHit, double GroundZCm,
        double VerticalSpeedCm, EContact Prev, const FParams& Params);
};
