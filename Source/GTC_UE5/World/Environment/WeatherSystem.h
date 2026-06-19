// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure, runtime-free weather model: the set of visual weather parameters, the
 * per-kind presets, and the *transition state machine* that blends from one
 * preset to the next. Like FSkyModel, smoothness is structural — a weather
 * change is never a swap of parameters, it is a timed blend from the parameters
 * the sky had when the change began to the new target, eased over
 * TransitionSeconds. At any instant the live parameters are a convex blend of
 * two presets, so every value moves monotonically and is bounded per frame by
 * roughly DeltaSeconds / TransitionSeconds — it is mathematically impossible for
 * it to snap (unless TransitionSeconds is driven to ~0, which Begin* clamps away).
 *
 * Scene-free pure-core (no UObject, double precision), same pattern as
 * FAmbientEvents: the director's randomness is injected as a [0,1) Roll so the
 * whole thing is deterministic and unit-tested headless (Tests/WeatherSystemTest.cpp,
 * prefix GTC.World.Weather). The actor adapter (AGTCWeatherController) owns an
 * instance, ticks it, supplies rolls from its FRandomStream when a new spell is
 * due, and pushes Current() onto the sky/cloud/fog/rain visuals.
 */

/** Weather archetypes, ordered by severity for the gradual director walk. */
enum class EWeatherKind : uint8
{
	Clear = 0,
	PartlyCloudy,
	Overcast,
	Rain,
	Storm,
	// Fog is a special alternate (not on the severity chain) reachable manually
	// or as an occasional calm-weather variant.
	Fog,
	Count
};

/**
 * The visual weather parameters, all normalised to [0,1] so a single blend
 * scalar drives every channel coherently. The adapter maps these onto real
 * units (cloud material coverage, ExponentialHeightFog density, rain spawn rate,
 * a wetness material-parameter, a sun-light multiplier, etc.).
 */
struct GTC_UE5_API FWeatherParams
{
	double CloudCoverage = 0.1;  // sky fraction covered by cloud
	double CloudDensity = 0.3;   // opacity/thickness of that cloud
	double FogDensity = 0.05;    // ground haze amount
	double RainIntensity = 0.0;  // 0 = dry, 1 = downpour
	double Wetness = 0.0;        // surface wetness (lags rain in the adapter if desired)
	double Wind = 0.15;          // drives cloud drift + rain slant
	double SunLightScale = 1.0;  // multiplies sun brightness (storms darken the day)
	double SkyDesaturation = 0.0;// grey-out of the sky under heavy overcast

	/** Component-wise linear blend A->B by Alpha (Alpha clamped to [0,1]). */
	static FWeatherParams Lerp(const FWeatherParams& A, const FWeatherParams& B, double Alpha);

	/** The authored target parameters for a weather kind. */
	static FWeatherParams Preset(EWeatherKind Kind);

	/** Largest absolute per-component difference (used by tests/asserts). */
	static double MaxAbsDelta(const FWeatherParams& A, const FWeatherParams& B);
};

/**
 * The live weather state + timed transition. Hold one, BeginTransition() to a
 * new kind, and Tick(dt) every frame; Current() is always a smooth blend.
 */
struct GTC_UE5_API FWeatherSystem
{
	/** Floor on transition length so a change can never be made instant. */
	static constexpr double MinTransitionSeconds = 2.0;

	FWeatherSystem();

	/** Snap immediately to a kind with no blend — for initial spawn only. */
	void InitializeTo(EWeatherKind Kind);

	/**
	 * Start easing from the current live parameters toward Kind's preset over
	 * Seconds (clamped to >= MinTransitionSeconds). Starting from the *current*
	 * blended params (not the previous kind's preset) means re-targeting mid-
	 * transition is still continuous — no snap.
	 */
	void BeginTransition(EWeatherKind Kind, double Seconds);

	/**
	 * Advance the blend and the dwell clock by DeltaSeconds. Returns true exactly
	 * once when the current spell has lasted DwellSeconds AND the last transition
	 * has finished — the adapter takes that as "time to roll the next spell".
	 */
	bool Tick(double DeltaSeconds, double DwellSeconds);

	/** The live, blended parameters to apply this frame. */
	const FWeatherParams& Current() const { return CurrentParams; }

	EWeatherKind CurrentKind() const { return TargetKind; }
	bool IsTransitioning() const { return TransitionSeconds > 0.0 && Elapsed < TransitionSeconds; }
	/** Blend progress of the active transition in [0,1] (1 when settled). */
	double TransitionAlpha() const;

	/**
	 * Director: choose the next weather kind from Current, biased to adjacent
	 * severity so weather evolves gradually (clear->partly->overcast->rain->storm)
	 * rather than teleporting clear->storm. Roll in [0,1) selects the direction;
	 * deterministic, so it is unit-tested without an RNG. Ends of the chain reflect
	 * inward; a small slice of rolls from the calm end diverts to Fog.
	 */
	static EWeatherKind ChooseNextKind(EWeatherKind Current, double Roll);

	/** Position on the severity chain (Fog maps next to Overcast). */
	static int32 SeverityIndex(EWeatherKind Kind);

private:
	FWeatherParams StartParams;   // params captured when the current transition began
	FWeatherParams CurrentParams; // live blended params
	EWeatherKind TargetKind = EWeatherKind::Clear;
	double TransitionSeconds = 0.0;
	double Elapsed = 0.0;         // into the current transition
	double DwellElapsed = 0.0;    // since the current spell settled in
};
