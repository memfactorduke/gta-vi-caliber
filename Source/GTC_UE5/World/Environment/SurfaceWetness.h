// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure, runtime-free surface-wetness integrator: how wet the streets are given the
 * rain over time. Wetness *lags* rain — puddles build quickly when it starts and
 * dry off slowly after it stops — so the wet-reflective night look (the whole
 * reason the MPC carries world_wetness) eases in and out instead of snapping with
 * the rain parameter. It is the slow companion to FWeatherFront, which supplies
 * the instantaneous rain/target; this core turns that into the lagged wetness the
 * material actually shows.
 *
 * Scene-free pure core (static methods, no UObject, double precision), the
 * FSkyModel / FWeatherSystem pattern; the weather controller holds one wetness
 * scalar, Step()s it each frame from the current rain, and pushes it onto the MPC
 * (world_wetness). Unit-tested headless in Tests/SurfaceWetnessTest.cpp (prefix
 * GTC.World.Wetness).
 */
struct GTC_UE5_API FSurfaceWetness
{
	/** Default approach rates (per second): wets fast, dries slow. */
	static constexpr double DefaultWetRate = 0.5;
	static constexpr double DefaultDryRate = 0.08;

	/** Steady-state wetness for sustained rain of this intensity: just clamp[0,1]. */
	static double Equilibrium(double RainIntensity);

	/**
	 * Advance wetness one step toward the rain-driven target (= RainIntensity):
	 * rising uses WetRate, falling uses the slower DryRate, so the response is
	 * asymmetric — quick to wet, slow to dry. Exponential approach, clamped to
	 * [0,1], so a single frame never snaps and the value never overshoots.
	 */
	static double Step(double Wetness, double RainIntensity, double DeltaSeconds,
		double WetRate = DefaultWetRate, double DryRate = DefaultDryRate);
};
