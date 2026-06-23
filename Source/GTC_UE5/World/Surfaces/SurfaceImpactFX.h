// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SurfaceImpact.h"

struct FHitResult;
class UWorld;

/**
 * Engine-coupled adapter for the pure-core surface impact registry (SurfaceImpact.h). It is the
 * bridge the registry header promised: it turns a live FHitResult into the right burst at the
 * impact point.
 *
 * Classification priority (matches SurfaceImpact.h):
 *   1. Authored tags — the hit component's tags, then the actor's tags. A "Creature" tag sprays
 *      blood; "Surface.Wood" / .Metal / .Glass / .Concrete pick the material burst. Tags win
 *      because they need no physical material on every mesh.
 *   2. The trace's physical-material surface index (EPhysicalSurface -> SurfaceTypeN), used when
 *      nothing is tagged. Requires the trace to set bReturnPhysicalMaterial.
 *   3. Default — a generic puff when neither resolves.
 *
 * Assets (the Cascade UParticleSystem and any decal material) resolve lazily on first use and are
 * cached for the lifetime of the module, so the registry's literal /Game paths stay allocation-free.
 */
struct GTC_UE5_API FGTCImpactFX
{
	/** Resolve a hit to a surface category: component tags, then actor tags, then physical material. */
	static EGTCSurface ResolveSurface(const FHitResult& Hit);

	/**
	 * Resolve the surface and spawn its burst (and a wear decal on static geometry) at
	 * Hit.ImpactPoint, fanned along Hit.ImpactNormal. No-op when World is null. Safe to call for
	 * every pellet of a shotgun and for AI-fired weapons.
	 */
	static void PlayImpact(UWorld* World, const FHitResult& Hit);
};
