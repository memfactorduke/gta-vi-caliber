// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure, runtime-free night-lighting logic: when streetlights and lit windows come
 * on at dusk and go off at dawn, and how much of the city is lit overall (the
 * master world_night_amount the MPC carries to switch on neon / emissive windows).
 * Two details make it read as a real city instead of a switch: hysteresis (a hold
 * band between the dusk-on and dawn-off thresholds, so a light hovering near the
 * edge doesn't stutter on and off) and per-light jitter (each light's threshold is
 * nudged by its own seed, so the city lights up over a few minutes of dusk rather
 * than every bulb popping on the same frame).
 *
 * Scene-free pure core (static methods, no UObject, double precision), the
 * FSkyModel / FWeatherSystem pattern; the lighting adapter reads FSkyModel's
 * daylight factor, calls ShouldBeLit() per light (with its jittered thresholds)
 * and pushes LitFraction() onto the MPC (world_night_amount). Unit-tested headless
 * in Tests/NightLightsTest.cpp (prefix GTC.World.NightLights).
 *
 * Convention: Daylight is FSkyModel::DaylightFactor in [0,1] — 0 at full night, 1
 * in full day. Lights want to be on when it is dark (low daylight).
 */
struct GTC_UE5_API FNightLights
{
	/** Default dusk-on / dawn-off daylight thresholds (On < Off => a hold band). */
	static constexpr double DefaultOnThreshold = 0.25;  // turn on once daylight falls below this
	static constexpr double DefaultOffThreshold = 0.35; // turn off once daylight rises above this
	static constexpr double DefaultJitterSpread = 0.08; // per-light threshold spread

	/**
	 * Hysteretic on/off decision for one light. An off light turns on only once
	 * daylight drops below OnThreshold; a lit light stays on until daylight climbs
	 * above OffThreshold. Between the two it holds its current state, so a value
	 * dithering in the band never chatters. Thresholds are swapped if misordered.
	 */
	static bool ShouldBeLit(bool CurrentlyLit, double Daylight,
		double OnThreshold = DefaultOnThreshold, double OffThreshold = DefaultOffThreshold);

	/**
	 * A per-light threshold offset from its seed: Base + (Jitter01 - 0.5)*Spread,
	 * clamped to [0,1]. Jitter01 = 0.5 leaves the base unchanged; 0 / 1 are the
	 * earliest / latest switchers. Feed each light a stable Jitter01 (e.g. a hash
	 * of its position) so the city lights up over a spread of dusk, not in unison.
	 */
	static double JitteredThreshold(double BaseThreshold, double Jitter01, double Spread = DefaultJitterSpread);

	/**
	 * Aggregate lit fraction in [0,1] — the share of the city expected to be lit at
	 * this daylight, which is what the jittered population produces in the mean: 1
	 * below OnThreshold (all lit), 0 above OffThreshold (all dark), a smooth ramp
	 * across the band. Drives MPC world_night_amount.
	 */
	static double LitFraction(double Daylight,
		double OnThreshold = DefaultOnThreshold, double OffThreshold = DefaultOffThreshold);
};
