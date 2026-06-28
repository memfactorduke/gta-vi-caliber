// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Wet-road grip — the producer for a seam that already exists. FVehicleHandling
 * CONSUMES a GripFactor (0..1): LateralRetention(bHandbrake, GripFactor) and
 * ApplyGrip(..., GripFactor) already take "how much grip the tyres have", and that
 * header even calls the factor "tyre/wear/wet". But nothing turns the weather into
 * that number — the rain falls, SurfaceWetness/WeatherController report a wet road,
 * and the car drives exactly as it does in the dry. FRoadGrip closes the
 * ROADMAP/SYSTEMS "feed wetness into vehicle grip" TODO: it maps how wet the road
 * is and how fast you're going into the grip factor handling is waiting for.
 *
 * Two effects, matching how a wet road actually behaves:
 *   - a wet surface is simply LESS grippy at any speed — grip drops linearly with
 *     wetness by up to WetGripLoss.
 *   - AQUAPLANING only happens with water AND speed: standing water the tyres can't
 *     clear at speed floats the car, collapsing grip toward AquaplaneFloor. The
 *     amount ramps in with speed between two thresholds and scales with wetness, so
 *     a dry road never aquaplanes and a damp one aquaplanes less than a flooded one.
 *
 * The result is the single GripFactor the adapter multiplies by the car's own
 * tyre/wear grip (kept separate — this core is the ROAD's contribution only) and
 * hands down to FVehicleHandling. IsAquaplaning / AquaplaneAmount drive the
 * loss-of-control FX and a "slow down, it's flooding" tell.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject,
 * all static (like FVehicleHandling) — unit-tested headless (Tests/RoadGripTest.cpp,
 * prefix GTC.Vehicles.Traction).
 *
 * PURE-CORE boundary: sourcing Wetness from the weather/surface systems, the car's
 * speed from the movement component, multiplying in tyre/wear grip, and feeding the
 * product to FVehicleHandling is the DEFERRED adapter and is NOT covered by the
 * tests. Units: Wetness and the returned grip are 0..1; speed is km/h.
 */
struct GTC_UE5_API FRoadGrip
{
    /** Tuning for how a wet road bleeds grip. Defaults give a ~0.65 wet floor, aquaplane from 90 km/h. */
    struct FParams
    {
        /** Grip lost at full wetness, low speed (so wet low-speed grip = 1 - WetGripLoss). */
        double WetGripLoss = 0.35;
        /** Speed (km/h) at which aquaplaning begins on a fully wet road. */
        double AquaplaneSpeedKmh = 90.0;
        /** Speed (km/h) at which aquaplaning is fully developed. */
        double AquaplaneSpeedFullKmh = 160.0;
        /** Grip floor when fully aquaplaning (keep below 1 - WetGripLoss). */
        double AquaplaneFloor = 0.25;
    };

    /**
     * The road's grip factor (0..1) at the given Wetness (0..1, clamped) and SpeedKmh
     * (clamped >= 0). 1.0 on a dry road at any speed; on a wet road it drops with
     * wetness and, past the aquaplane speed, collapses toward AquaplaneFloor.
     */
    static double GripFactor(double Wetness, double SpeedKmh, const FParams& Params);

    /**
     * How developed aquaplaning is right now, 0..1 — 0 on a dry road or below the
     * aquaplane speed, ramping to 1 at full wetness and full aquaplane speed.
     */
    static double AquaplaneAmount(double Wetness, double SpeedKmh, const FParams& Params);

    /** True when the car is aquaplaning at all (water present and over the aquaplane speed). */
    static bool IsAquaplaning(double Wetness, double SpeedKmh, const FParams& Params);
};
