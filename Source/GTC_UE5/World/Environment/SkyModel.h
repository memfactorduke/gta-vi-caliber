// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure, runtime-free time-of-day -> sky/sun math. The whole day<->night cycle is
 * expressed as continuous functions of a single continuously-advancing clock
 * (TimeOfDay in [0,24) hours), so the transition is *physically incapable* of
 * jumping: nothing here is a state switch, every output is C0-continuous (and
 * C1-smooth) in the hour. Dawn and dusk are gradual ramps, never a flip.
 *
 * Same scene-free pure-core pattern as FTrafficModel / FPlayerMotion: static
 * methods only, no UObject, double precision. The actor adapter
 * (AGTCWeatherController) advances TimeOfDay each tick with AdvanceHours() and
 * feeds these scalars onto the GTC_Sun directional light, SkyLight, fog and
 * post-process. Unit-tested headless in Tests/SkyModelTest.cpp (prefix
 * GTC.World.Sky).
 *
 * Conventions:
 *  - Hours run 0..24, 6 = sunrise, 12 = solar noon, 18 = sunset, 0/24 = midnight.
 *  - SunElevation01 is the normalised solar altitude in [-1,+1] (1 = straight
 *    overhead, 0 = on the horizon, -1 = deepest midnight), a single sinusoid so
 *    it is smooth and seamless across the midnight wrap.
 *  - Sun *pitch* is the UE directional-light pitch (degrees): the light points
 *    down (pitch -90) when the sun is overhead, along the horizon (pitch 0) at
 *    sunrise/sunset.
 */
struct GTC_UE5_API FSkyModel
{
	/** Wrap any hour value into the canonical [0,24) range (handles negatives). */
	static double WrapHours(double Hours);

	/**
	 * Advance the clock by real DeltaSeconds. A full 0->24 cycle takes
	 * DayLengthSeconds of real time; the result is wrapped into [0,24). A
	 * non-positive DayLengthSeconds freezes the clock (returns Hours unchanged)
	 * so a misconfigured length can never produce a jump or div-by-zero.
	 */
	static double AdvanceHours(double Hours, double DeltaSeconds, double DayLengthSeconds);

	/**
	 * Normalised solar altitude in [-1,+1]: sin over one 24h period, zero at the
	 * 6h/18h horizons, +1 at noon, -1 at midnight. Continuous and seamless across
	 * the 24->0 wrap (a single sine of the hour).
	 */
	static double SunElevation01(double Hours);

	/** UE directional-light pitch in degrees: -90*elevation (overhead sun -> -90). */
	static double SunPitchDegrees(double Hours);

	/**
	 * A slow azimuth sweep so shadows rotate through the day rather than only
	 * tilting. BaseYaw is the south-facing reference; the sun swings +/-SwingDeg
	 * around it across the day. Continuous in the hour.
	 */
	static double SunYawDegrees(double Hours, double BaseYaw, double SwingDeg = 35.0);

	/**
	 * Daylight strength in [0,1]: 0 at full night, 1 in full day, with a smooth
	 * (smoothstep) twilight ramp centred on the horizon crossing. This is the
	 * master "how lit is the world" factor that dimming/colour/stars derive from,
	 * so dawn and dusk fade in/out over real minutes, never a switch.
	 */
	static double DaylightFactor(double Hours);

	/**
	 * Directional-light brightness (lux, UE's physical sun unit) for the hour:
	 * lerps NightLux..DayLux by DaylightFactor. Defaults are a bright midday sun
	 * and a dim (moonlight-ish) night floor so it never reaches pure black.
	 */
	static double SunBrightnessLux(double Hours, double DayLux = 75000.0, double NightLux = 0.0);

	/** Moon/ambient directional brightness (lux) for the unlit half, fades with night. */
	static double MoonBrightnessLux(double Hours, double MoonLux = 0.6);

	/** SkyLight (ambient bounce) intensity scale in [NightFloor,1] over the day. */
	static double SkyLightFactor(double Hours, double NightFloor = 0.04);

	/**
	 * Sun colour "warmth" in [0,1]: 0 = neutral white (high midday sun), 1 = deep
	 * orange/red at the horizon. Peaks at sunrise/sunset, smooth either side, so
	 * the golden hour eases in and out. Drives the directional light colour in the
	 * adapter (lerp white->sunset-orange by this factor).
	 */
	static double SunWarmth(double Hours);

	/** Star/night-sky opacity in [0,1]: simply 1-DaylightFactor (stars fade in at dusk). */
	static double StarOpacity(double Hours);
};
