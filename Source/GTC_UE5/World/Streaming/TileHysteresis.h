// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure load/unload hysteresis for the M3 streaming world.
 *
 * A single load/unload radius makes tiles thrash: a camera hovering right on the
 * boundary loads and unloads the same tile every frame, churning the streamer.
 * The fix is a two-radius band — load a tile once it comes within the inner
 * `LoadRadius`, but only unload it once it passes the wider outer `UnloadRadius`.
 * Between the two the tile keeps whatever state it already has, so crossing the
 * edge has to be decisive before anything streams.
 *
 * Optionally pair the spatial band with a temporal dwell: require the unload
 * condition to hold for a minimum time before acting, so a brief excursion past
 * the outer radius (a fast strafe, a camera shake) doesn't evict a tile that is
 * about to be needed again. `AdvanceDwell` is the pure accumulator for that.
 *
 * Plain non-UObject static helpers (the FStreamingGrid / FTileStreamPriority
 * pattern), double precision. Distance-metric agnostic: pass plain radial
 * distance, or the anticipatory FTileStreamPriority::EffectiveDistance, whichever
 * the caller streams on. Radii are clamped to >= 0 and the outer radius is
 * treated as max(Load, Unload) so a misconfigured pair degrades to a single
 * threshold instead of misbehaving.
 *
 * Coverage: Tests/TileHysteresisTest.cpp, prefix GTC.World.Streaming.Hysteresis.
 */
struct GTC_UE5_API FTileHysteresis
{
	/**
	 * Next residency state for a tile given its distance and current state:
	 * load when Distance <= LoadRadius, unload when Distance >= UnloadRadius,
	 * otherwise hold the current state (the sticky band). Inclusive load edge
	 * wins ties so a tile exactly on LoadRadius is resident.
	 */
	static bool ShouldBeResident(bool bCurrentlyResident, double Distance, double LoadRadius, double UnloadRadius);

	/**
	 * True when Distance is strictly inside the hysteresis band
	 * (LoadRadius < Distance < UnloadRadius) — the zone where ShouldBeResident
	 * holds the current state. False on or outside either edge, or if the band
	 * is empty (Unload <= Load).
	 */
	static bool InBand(double Distance, double LoadRadius, double UnloadRadius);

	/**
	 * Advance a dwell timer for temporal hysteresis: accumulate DeltaSeconds
	 * while bConditionHeld (e.g. "beyond the unload radius"), reset to 0 the
	 * moment it stops holding. Negative delta is ignored. Pair with a
	 * `dwell >= MinDwellSeconds` check before acting on an unload.
	 */
	static double AdvanceDwell(double CurrentDwell, bool bConditionHeld, double DeltaSeconds);
};
