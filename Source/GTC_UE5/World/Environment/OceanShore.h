// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure, runtime-free shoreline math: how the ocean meets the land. From the
 * water-column depth (the instantaneous wave surface minus the seabed/terrain
 * height — the caller reads the surface from FOceanSurface, so the shoreline
 * *moves with the swell*) it produces the water-material shore blend, the
 * breaking-wave foam band that sits just offshore, the wet-sand run-up line that
 * follows the swash, and a depth colour fade for shallow->deep water tint.
 *
 * Scene-free pure core (static methods, no UObject, double precision), the
 * FSkyModel / FOceanSurface pattern; the water/landscape material adapter feeds
 * these into the shore blend, foam mask and a beach wetness parameter. Because
 * every factor is a function of the *instantaneous* depth, raising the wave
 * surface sweeps the foam and wet line up the beach automatically — real
 * shoreline interaction, no extra bookkeeping. Unit-tested headless in
 * Tests/OceanShoreTest.cpp (prefix GTC.World.Ocean.Shore).
 *
 * Conventions: Z up, metres. Depth > 0 is water over the point, Depth <= 0 is dry
 * land standing above the water.
 */
struct GTC_UE5_API FOceanShore
{
	/** Water-column depth at a point: instantaneous water surface Z minus seabed Z. */
	static double WaterDepth(double WaterSurfaceZ, double SeabedZ);

	/**
	 * Water-material shore blend in [0,1]: 0 right at the waterline, rising
	 * smoothly to 1 once the water is ShallowDepth deep. Drives the depth-based
	 * opacity/edge-softening so the water doesn't end in a hard line on the sand.
	 */
	static double ShoreBlend01(double Depth, double ShallowDepth);

	/**
	 * Breaking-wave foam coverage in [0,1]: a band that sits just offshore — zero
	 * exactly at the waterline and in deep water, peaking in the shallow break
	 * zone within FoamDepth of shore. Multiply by a wave-energy factor in the
	 * adapter so calm water foams less than a heavy swell.
	 */
	static double ShoreFoam01(double Depth, double FoamDepth);

	/**
	 * Wet-sand factor in [0,1] for a point of dry land at height TerrainZ: 1 where
	 * the terrain is at/below the swash top (just been washed), fading to 0 over
	 * DryBand above it. Feed SwashTopZ = mean sea level + the wave run-up so the
	 * wet line chases each wave up and down the beach.
	 */
	static double WetSand01(double TerrainZ, double SwashTopZ, double DryBand);

	/**
	 * Shallow->deep colour fade in [0,1]: 0 at the shoreline, approaching 1 in
	 * deep water as 1 - exp(-Depth/FadeDistance). Drives the turquoise-shallows to
	 * deep-blue tint of the water material.
	 */
	static double DepthColorFade01(double Depth, double FadeDistance);
};
