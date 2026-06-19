// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * How a citizen FEELS about the current sky — the layer that lets weather and the
 * hour actually change behaviour instead of just recolouring it. A clear noon is
 * nothing; a storm sends the skittish scurrying for cover, hurries everyone's
 * step, and earns a grumble. Tie it to the existing day/night + weather system
 * (FWeatherSystem) and a crowd stops looking like it's strolling through a photo
 * and starts looking like it's living in real weather.
 *
 * Decoupled from the weather enum by design: the adapter passes a SEVERITY (0
 * clear .. 4 storm — exactly FWeatherSystem::SeverityIndex) plus the clock hour
 * and the citizen's bravery, so this model stays pure-core with no dependency on
 * World/Environment. Braver people tolerate more before seeking shelter; night
 * adds a little extra discomfort/caution.
 *
 * PURE-CORE: scene-free, deterministic, no engine coupling. Unit-tested headless
 * (Tests/NpcWeatherReactionTest.cpp, prefix GTC.NPC.Decision.NpcWeatherReaction).
 * GTC-original content.
 */
struct GTC_UE5_API FNpcWeatherResponse
{
	/** Head for cover (indoors / under an awning) — the rain/storm + skittish case. */
	bool bSeekShelter = false;

	/** Pick up the pace to get out of it — anything past a light drizzle. */
	bool bHurry = false;

	/** How unpleasant it feels, 0 (fine) .. 1 (miserable). Drives comment odds,
	 *  posture (hunch/umbrella), and the shelter/hurry decisions. */
	float Discomfort = 0.0f;
};

struct GTC_UE5_API FNpcWeatherReaction
{
	/**
	 * Assess the weather for one citizen. `Severity` is the weather severity index
	 * (0 clear .. 4 storm); `Hour` is the clock hour [0,24); `Bravery` is [0,1]
	 * (skittish .. bold). Deterministic. Higher bravery resists seeking shelter;
	 * night nudges discomfort up.
	 */
	static FNpcWeatherResponse Assess(int32 Severity, double Hour, double Bravery);

	/** Night-ish hours (before dawn / after dusk), when the city reads darker. */
	static bool IsNight(double Hour);

	/** Base discomfort for a severity index alone (no hour/bravery), 0..1. */
	static float BaseDiscomfort(int32 Severity);
};
