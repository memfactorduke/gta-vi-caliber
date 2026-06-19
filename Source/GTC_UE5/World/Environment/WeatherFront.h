// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure, runtime-free weather *front*: a line of weather sweeping across the map,
 * so a storm rolls in from one side instead of the whole world switching at once.
 * Where FWeatherSystem owns the global "what kind of weather" blend over time,
 * this adds the *spatial* gradient — at a given point and time it reports how far
 * that point is ahead of / behind the front and the local clear->overcast->rain
 * intensity, then maps that to the shared world material globals (MPC_GTCGlobals:
 * world_wetness, world_night_amount) plus cloud/rain.
 *
 * Scene-free pure core (static methods, no UObject, double precision), the
 * FSkyModel / FWeatherSystem pattern; the weather controller projects each shaded
 * point onto the front axis, calls Intensity01(), and pushes Globals() onto the
 * MPC. Unit-tested headless in Tests/WeatherFrontTest.cpp (prefix
 * GTC.World.WeatherFront).
 *
 * Conventions: "Along" is a point's coordinate along the front's sweep axis
 * (the caller dots the world position with the unit axis), metres. The front
 * advances along that axis at Speed (m/s). Ahead of the line (Along > line) is
 * still clear; behind it (Along < line) the weather has arrived.
 */

/** The shared world material globals a front drives (all normalised [0,1]). */
struct GTC_UE5_API FWorldGlobals
{
	double CloudCoverage = 0.0; // sky cover — builds first as the front nears
	double RainIntensity = 0.0; // rain — onsets only in the heavy half of the front
	double Wetness = 0.0;       // target wet-surface amount (MPC world_wetness)
	double NightAmount = 0.0;   // darkening (MPC world_night_amount): night + storm gloom
};

struct GTC_UE5_API FWeatherFront
{
	/** Position of the front line along its axis at Time: FrontStart + Speed*Time. */
	static double FrontLineAt(double FrontStart, double Speed, double Time);

	/**
	 * Signed distance from a point (at coordinate Along) to the front line: > 0 the
	 * front has not reached it yet (clear), < 0 the front has passed (weathered).
	 */
	static double SignedDistance(double Along, double FrontStart, double Speed, double Time);

	/**
	 * Local weather intensity in [0,1] from the signed distance: 0 far ahead of the
	 * front (clear), 1 far behind (full weather), 0.5 on the line, ramped smoothly
	 * over Width. Monotone decreasing in signed distance — closer/behind = worse.
	 */
	static double Intensity01(double SignedDist, double Width);

	/**
	 * Map a local intensity (and the time-of-day daylight factor) to the shared
	 * world globals. Clouds build with intensity; rain onsets only past the midpoint
	 * (overcast precedes rain); wetness tracks rain; night-amount is the night
	 * darkness lifted by storm gloom so a heavy front dims even midday. All [0,1].
	 */
	static FWorldGlobals Globals(double Intensity01, double DaylightFactor, double StormDarkening = 0.5);
};
