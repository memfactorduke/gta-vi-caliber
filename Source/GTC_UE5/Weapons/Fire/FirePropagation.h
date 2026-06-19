// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Spreading fire — a molotov that splashes flame across the asphalt, a torched car that
 * catches the one parked beside it, brush that goes up and races outward. FExplosionModel
 * handles the instant blast; this is the slow, creeping aftermath: fires grow while they have
 * fuel, jump to nearby flammables, deal damage over time, and burn themselves out.
 *
 * Pairs with FThrowable (a molotov's DetonationPoint seeds a fire) and the weapon/vehicle
 * damage layers (DamagePerSecond ticks anyone standing in it).
 *
 * Two halves:
 *   - Static spread math: SpreadIntensity (distance falloff to neighbours), CanIgnite
 *     (does a neighbour catch, given its flammability), DamagePerSecond.
 *   - A stateful FFireCell: one burning thing — Ignite it, then Tick it as it grows, consumes
 *     fuel, and dies out.
 *
 * PURE-CORE: scene-free, deterministic, double precision, no engine coupling, no RNG. The fire
 * director owns a grid/list of FFireCell, each tick advances them, and for any cell that
 * CanSpread() it asks SpreadIntensity/CanIgnite against nearby flammables and lights new cells;
 * DamagePerSecond hits actors overlapping a burning cell. Distances are metres; times seconds.
 * Unit-tested headless (Weapons/Fire/Tests/FirePropagationTest.cpp, prefix GTC.Weapons.Fire).
 */
struct GTC_UE5_API FFirePropagation
{
    /** Fire jumps to flammables within this range (metres). */
    static constexpr double SpreadRadiusM = 4.0;
    /** Below this imparted intensity a neighbour is too far to catch. */
    static constexpr double MinSpreadIntensity = 0.15;
    /** Imparted intensity x flammability at/above this ignites the neighbour. */
    static constexpr double IgnitionThreshold = 0.2;
    /** Damage per second a full-intensity fire deals. */
    static constexpr double MaxDamagePerSec = 12.0;

    /**
     * Intensity (0..1) a neighbour at DistanceM catches from a fire of SourceIntensity,
     * with linear falloff to 0 at RadiusM. Returns 0 beyond the radius or below
     * MinSpreadIntensity.
     */
    static double SpreadIntensity(double SourceIntensity, double DistanceM, double RadiusM = SpreadRadiusM);

    /**
     * Does a neighbour of the given Flammability (0..1) catch fire from a SourceIntensity fire
     * DistanceM away: SpreadIntensity * Flammability >= IgnitionThreshold.
     */
    static bool CanIgnite(double SourceIntensity, double DistanceM, double Flammability,
        double RadiusM = SpreadRadiusM);

    /** Damage per second from a fire of the given intensity (0..1). */
    static double DamagePerSecond(double Intensity);

    /** One burning thing: a fire with an intensity that grows on fuel and dies without it. */
    struct GTC_UE5_API FFireCell
    {
        /** How fast intensity climbs toward 1 while fuelled (per second). */
        static constexpr double GrowthRate = 0.8;
        /** How fast intensity falls once the fuel is gone (per second). */
        static constexpr double DecayRate = 0.5;
        /** Fuel consumed per second at full intensity. */
        static constexpr double BurnPerSec = 0.2;

        double Intensity = 0.0; // 0..1
        double Fuel = 0.0;      // remaining fuel (seconds-ish of burn at full intensity)

        /** Light this cell with a starting intensity and a fuel load. */
        void Ignite(double StartIntensity, double FuelLoad);

        /** Advance: grow + burn fuel while fuelled, decay once spent. */
        void Tick(double Dt);

        bool IsBurning() const { return Intensity > 0.01; }
        /** Hot enough and still fuelled to light neighbours. */
        bool CanSpread() const { return Intensity >= MinSpreadIntensity && Fuel > 0.0; }
    };
};
