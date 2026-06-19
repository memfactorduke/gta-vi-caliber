// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure, runtime-free ocean surface math: a sum of Gerstner (trochoidal) waves
 * sampled analytically at a world (X,Y) and time. This is the foundation the
 * rest of Ocean v2 stands on — buoyancy samples the height, the wake/foam core
 * reads the folding Jacobian, the shoreline core reads the height against the
 * terrain — so it lives as a scene-free pure core (static methods, no UObject,
 * double precision), the same pattern as FSkyModel / FWeatherSystem. The actor /
 * material adapter feeds the same FGerstnerWave set to the water material so the
 * physics surface and the rendered surface agree.
 *
 * Gerstner waves move surface points in a circle, so unlike a plain sine they
 * also displace *horizontally* — sharpening crests and flattening troughs — which
 * is what makes the analytic height usable for buoyancy and the Jacobian usable
 * for whitecap foam. Unit-tested headless in Tests/OceanSurfaceTest.cpp
 * (prefix GTC.World.Ocean).
 *
 * Conventions:
 *  - Z is up; mean sea level is Z = 0. Distances are metres, time is seconds.
 *  - Each wave's amplitude is the crest height above mean; |total height| is
 *    bounded by the sum of amplitudes.
 *  - Steepness in [0,1] is the Gerstner pinch: 0 = a round sine wave (no
 *    horizontal motion), 1 = a sharp trochoidal crest. It is the ratio of
 *    horizontal displacement to amplitude, not a frequency, so it never changes
 *    where a crest is, only its shape.
 */

/** One Gerstner wave component. Direction need not be normalised (it is, inside). */
struct GTC_UE5_API FGerstnerWave
{
	double DirX = 1.0;        // horizontal travel direction (normalised internally)
	double DirY = 0.0;
	double Amplitude = 0.5;   // crest height above mean sea level, metres
	double Wavelength = 12.0; // crest-to-crest distance, metres
	double Steepness = 0.5;   // [0,1] Gerstner pinch (0 = sine, 1 = sharp crest)
	double Speed = 0.0;       // phase speed m/s; <= 0 => deep-water dispersion sqrt(g/k)
};

/** Everything the surface yields at one point: where it is and which way it faces. */
struct GTC_UE5_API FOceanSample
{
	double Height = 0.0;   // vertical displacement above mean sea level (Z up)
	double DispX = 0.0;    // horizontal Gerstner displacement of this surface point
	double DispY = 0.0;
	double NormalX = 0.0;  // unit surface normal (Z up for a calm/gentle sea)
	double NormalY = 0.0;
	double NormalZ = 1.0;
	double Jacobian = 1.0; // horizontal-map determinant; 1 = calm, < 0 = folded crest (foam)
};

struct GTC_UE5_API FOceanSurface
{
	/** Default gravity used when a wave leaves Speed <= 0 (deep-water dispersion). */
	static constexpr double DefaultGravity = 9.81;

	/** Angular wavenumber k = 2*pi / Wavelength. Returns 0 for a non-positive length. */
	static double Wavenumber(double Wavelength);

	/**
	 * Angular frequency omega for one wave: Speed*k when Speed > 0, otherwise the
	 * deep-water dispersion relation sqrt(g*k). Longer waves therefore travel
	 * faster, exactly as real swell does.
	 */
	static double AngularFrequency(const FGerstnerWave& Wave, double Gravity = DefaultGravity);

	/**
	 * Sample the full surface (height, horizontal displacement, unit normal and
	 * folding Jacobian) from a set of waves at horizontal position (X,Y) and Time.
	 * A null/empty set, zero amplitudes, or degenerate waves yield a flat calm
	 * sample (height 0, normal +Z, Jacobian 1) — never a NaN.
	 */
	static FOceanSample Sample(const FGerstnerWave* Waves, int32 Count,
		double X, double Y, double Time, double Gravity = DefaultGravity);

	/** Convenience: just the surface height at a point (buoyancy's hot path). */
	static double HeightAt(const FGerstnerWave* Waves, int32 Count,
		double X, double Y, double Time, double Gravity = DefaultGravity);
};
