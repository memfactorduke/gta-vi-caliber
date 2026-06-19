// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure, runtime-free buoyancy: given a floating body approximated by a handful of
 * hull sample points and the ocean surface height under each (the caller reads
 * those from FOceanSurface), compute the net hydrostatic buoyant force, the
 * righting torque about the body's centre, and the volume-weighted submerged
 * fraction. Plus simple water drag so an arcade boat settles instead of bobbing
 * forever. Scene-free pure core (static methods, no UObject, double precision),
 * the FSkyModel / FOceanSurface pattern; the actor adapter (a Chaos boat pawn)
 * samples the surface at its hull points, calls Compute(), and applies the
 * returned force/torque. Unit-tested headless in Tests/OceanBuoyancyTest.cpp
 * (prefix GTC.World.Ocean.Buoyancy).
 *
 * Model: each sample owns a displaced Volume and a vertical Span. Its submersion
 * ramps linearly from 0 (at/above the water) to 1 once it is a full Span below
 * the surface, so the displaced volume — and thus the upward force rho*g*V — is
 * bounded: a chunk that is already fully under does not push harder the deeper it
 * goes, exactly as a fixed displaced volume should. Buoyancy acts straight up;
 * the righting torque emerges from the *asymmetry* of which samples are deeper.
 *
 * Conventions: Z up, mean sea level Z=0, SI units (metres, m^3, newtons, seconds).
 */

/** Minimal double vector for forces/torques/velocities (no FVector dependency). */
struct GTC_UE5_API FBuoyVec3
{
	double X = 0.0;
	double Y = 0.0;
	double Z = 0.0;
};

/** One hull sample: where it is, how high the water is there, what it displaces. */
struct GTC_UE5_API FBuoyancyPoint
{
	double X = 0.0;          // world position of this hull sample
	double Y = 0.0;
	double Z = 0.0;
	double WaterHeight = 0.0;// ocean surface height at (X,Y) — from FOceanSurface
	double Volume = 1.0;     // displaced volume this sample represents (m^3)
	double Span = 0.5;       // vertical extent dry -> fully submerged (m)
};

/** Net buoyancy outputs. Force/Torque exclude gravity (the body's mass owns that). */
struct GTC_UE5_API FBuoyancyResult
{
	FBuoyVec3 Force;                 // net buoyant force (N), straight up if level
	FBuoyVec3 Torque;                // net righting torque about the centre (N*m)
	double SubmergedFraction = 0.0;  // volume-weighted share underwater, [0,1]
};

struct GTC_UE5_API FOceanBuoyancy
{
	/** Sea-water density (kg/m^3); fresh water is ~1000. */
	static constexpr double SeaWaterDensity = 1025.0;
	static constexpr double DefaultGravity = 9.81;

	/** Submersion of one sample in [0,1]: 0 at/above water, 1 a full Span under. */
	static double Submersion01(const FBuoyancyPoint& Point);

	/**
	 * Net buoyant force + righting torque about (CenterX,CenterY,CenterZ) and the
	 * volume-weighted submerged fraction. A body entirely out of the water returns
	 * all-zero; a level half-submerged body returns pure up-force and ~zero torque.
	 */
	static FBuoyancyResult Compute(const FBuoyancyPoint* Points, int32 Count,
		double CenterX, double CenterY, double CenterZ,
		double Gravity = DefaultGravity, double Density = SeaWaterDensity);

	/**
	 * Linear water drag: -Coeff * SubmergedFraction * Velocity. Opposes motion,
	 * scales with how much hull is wetted, and is zero out of the water.
	 */
	static FBuoyVec3 LinearDrag(const FBuoyVec3& Velocity, double SubmergedFraction, double Coeff);

	/** Angular water drag: -Coeff * SubmergedFraction * AngularVelocity. */
	static FBuoyVec3 AngularDrag(const FBuoyVec3& AngularVelocity, double SubmergedFraction, double Coeff);
};
