// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VehicleHandling.h"      // FVehicleHandling — same folder
#include "../Traction/RoadGrip.h" // FRoadGrip

/**
 * FVehicleGripResolver — the per-frame "car handling brain" that finally joins three
 * vehicle cores the codebase built but never wired together:
 *
 *     surface wetness + speed   --FRoadGrip-->     the road's grip
 *     road grip x tyre grip                    =   the GripFactor handling wants
 *     heading vs velocity       --FVehicleHandling--> slip -> drift -> lateral retention
 *     retention applied to velocity            =   the gripped / sliding velocity
 *
 * Both cores describe exactly this seam and both left it deferred: FRoadGrip.h calls
 * "sourcing Wetness..., the car's speed..., multiplying in tyre grip, and feeding the
 * product to FVehicleHandling" the "DEFERRED adapter"; FVehicleHandling.h says "feed
 * GripFactor into LateralRetention". This is that adapter's PURE half — one deterministic
 * Resolve() call, FVector-in / plain-out, no engine coupling and NO new tuning of its own
 * (it only orchestrates the two cores' constants). So the whole grip chain stays
 * headless-unit-tested (Tests/VehicleGripResolverTest.cpp, prefix GTC.Vehicles.GripResolver)
 * and re-run out-of-tree by the oracle.
 *
 * The UObject shell UGTCVehicleHandlingComponent only GATHERS the inputs (the pawn's
 * velocity + a wetness value fed by the car BP) and PUBLISHES the outputs; it holds no
 * handling math itself.
 *
 * Units: Velocity/Forward/Right are UE world vectors (cm, cm/s). The slip/drift/apply math
 * is scale-free; only the wet-grip-vs-speed curve needs real units, so the resolver derives
 * the GROUND speed (velocity projected onto the Forward/Right plane, so a falling/airborne
 * car doesn't count its vertical drop as speed) and converts it to km/h — the units FRoadGrip
 * wants — with the fixed UE cm/s factor. Wetness / TyreGrip / grip factors are 0..1; angles
 * are radians.
 */
struct GTC_UE5_API FVehicleGripResolver
{
    /** cm/s -> km/h (1 cm/s = 0.036 km/h). Velocity arrives in UE cm/s; FRoadGrip wants km/h. */
    static constexpr double CmsToKmh = 0.036;

    /** What the chassis + world look like this frame. */
    struct FInput
    {
        FVector Velocity = FVector::ZeroVector; // chassis world velocity (cm/s)
        FVector Forward = FVector::ZeroVector;  // chassis forward, ground-plane (any length)
        FVector Right = FVector::ZeroVector;    // chassis right, ground-plane (any length)
        double Wetness = 0.0;                   // 0 dry .. 1 soaked (fed from the weather seam)
        double TyreGrip = 1.0;                  // 0..1 tyre/wear grip (FVehicleCondition), 1 = fresh
        bool bHandbrake = false;                // handbrake held this frame
    };

    /** The composed handling state the shell publishes and the chassis/FX/HUD read. */
    struct FOutput
    {
        double GroundSpeedKmh = 0.0;   // ground-plane speed in km/h (what the grip curve saw)
        double RoadGrip = 1.0;         // FRoadGrip's road-only grip, 0..1
        double GripFactor = 1.0;       // road grip x tyre grip, 0..1 — what handling consumes
        double SlipAngleRad = 0.0;     // angle between heading and ground velocity
        double DriftFactor = 0.0;      // 0..1, how hard the car is sliding FORWARD (0 in reverse)
        double LateralRetention = 0.0; // 0..1 fraction of sideways velocity kept this frame
        double AquaplaneAmount = 0.0;  // 0..1 how developed aquaplaning is
        bool bAquaplaning = false;     // water present AND over the aquaplane speed
        FVector GrippedVelocity = FVector::ZeroVector; // velocity after ApplyGrip (vertical preserved)
    };

    /**
     * Compose the whole grip chain for one frame. Pure + deterministic; degenerate input
     * (parked car, zero heading) falls through the cores' own guards to a sane no-slide
     * result, and driving straight in reverse is NOT treated as a drift.
     */
    static FOutput Resolve(const FInput& In, const FRoadGrip::FParams& RoadParams);
};
