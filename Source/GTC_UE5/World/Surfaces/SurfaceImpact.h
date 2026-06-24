// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Surface impact taxonomy — what a bullet, blade, or thrown object hits, and the burst it kicks
 * up. A round into a plank throws splinters; into a car panel, sparks; through a window, shards;
 * into a wall, dust and chips; into a person, blood. This is the lookup that turns "I hit
 * something" into "spawn THIS effect here".
 *
 * Two ways a hit is classified, in priority order (the live FX adapter applies them):
 *   1. Actor tag — an NPC carries the Creature tag (see GTCSurfaceTags::Creature), a destructible
 *      prop or building face carries Surface.Wood / .Metal / .Glass / .Concrete. Tags win because
 *      they are authored intent and need no physical material on every mesh.
 *   2. Physical material surface index — the EPhysicalSurface the trace returns (the SurfaceTypeN
 *      slots defined in Config/DefaultEngine.ini, mirrored by PhysicalSurfaceToSurface below).
 *
 * PURE-CORE: scene-free, deterministic, no engine spawn, no asset loads. The classification and
 * the effect registry are plain functions over an enum and literal asset paths, so they unit-test
 * headless (World/Surfaces/Tests/SurfaceImpactTest.cpp, prefix GTC.World.Surfaces). The engine
 * coupling — loading the UParticleSystem and spawning it at FHitResult.ImpactPoint — lives in the
 * FGTCImpactFX adapter (SurfaceImpactFX.*), which consumes this registry.
 */

/**
 * The material categories an impact resolves to. Mirrors the SurfaceTypeN order in
 * DefaultEngine.ini. ORDER IS LOAD-BEARING: the enum value equals the SurfaceTypeN index, and
 * PhysicalSurfaceToSurface relies on that. Append new surfaces before Count; never reorder.
 */
enum class EGTCSurface : uint8
{
	Default = 0, // SurfaceType_Default — unclassified geometry; generic puff.
	Wood,        // SurfaceType1  — planks, crates, fences, furniture. Splinters.
	Metal,       // SurfaceType2  — vehicle panels, railings, signs, sheet steel. Sparks.
	Glass,       // SurfaceType3  — windows, bottles, screens. Shards.
	Concrete,    // SurfaceType4  — walls, kerbs, building faces. Dust + chips.
	Creature,    // SurfaceType5  — anything alive: NPCs, the player, animals. Blood.
	Asphalt,     // SurfaceType6  — roads, tarmac. Dark grit.
	Brick,       // SurfaceType7  — brick walls, masonry. Red dust + chips.
	Ceramic,     // SurfaceType8  — tiles, pottery, sanitary ware. Sharp white shards.
	Rubber,      // SurfaceType9  — tyres, mats, hoses. Dull scuff, no decal.
	Vegetation,  // SurfaceType10 — grass, hedges, foliage, palms. Leaf burst, no decal.
	Ice,         // SurfaceType11 — frozen surfaces, freezers. Crystalline chips.
	Leather,     // SurfaceType12 — car seats, sofas, luggage. Soft puff.
	Paper,       // SurfaceType13 — posters, cardboard, books, trash. Confetti shred.
	Water,       // SurfaceType14 — ocean, pools, puddles. Splash, no decal.
	Count
};

/**
 * Canonical tag names. World geometry and destructibles carry the Surface.* tag for their
 * material; every NPC pawn carries Creature so a shot resolves to blood even when the skeletal
 * mesh has no physical material assigned. Accessors (not namespace-scope FName globals) so the
 * name table is always live when they're first read.
 */
namespace GTCSurfaceTags
{
	/** The tag for a material surface, e.g. EGTCSurface::Wood -> "Surface.Wood". None for Default. */
	GTC_UE5_API FName SurfaceTag(EGTCSurface Surface);

	/** The flesh tag placed on every living pawn — "Creature". Shorthand for SurfaceTag(Creature). */
	GTC_UE5_API FName CreatureTag();
}

/**
 * The burst to play when something of a given surface is hit: which Cascade system, an optional
 * decal, how big, and a debug colour for the headless draw path. Literal paths keep it allocation-
 * free and constexpr-friendly for tests; the adapter resolves them to assets lazily and caches.
 */
struct GTC_UE5_API FGTCImpactEffect
{
	/** /Game path of the Cascade UParticleSystem to spawn at the impact point. Never null. */
	const TCHAR* ParticlePath = nullptr;
	/** /Game path of a decal material to stamp on the surface, or null for none (e.g. Creature). */
	const TCHAR* DecalPath = nullptr;
	/** Uniform spawn scale for the particle system. */
	double Scale = 1.0;
	/** Decal quad size in centimetres; 0 means no decal regardless of DecalPath. */
	double DecalSizeCm = 0.0;
	/** Colour for the headless/debug marker at the impact point. */
	FColor DebugColor = FColor::White;
};

/** Surface classification + the effect registry. All pure; see file header. */
struct GTC_UE5_API FGTCSurfaceImpact
{
	/**
	 * Map a physical-material surface index (an EPhysicalSurface value, 0..62, as returned by a
	 * trace with bReturnPhysicalMaterial) to our category. Mirrors the SurfaceTypeN ordering in
	 * Config/DefaultEngine.ini. Unknown / unmapped indices fall back to Default.
	 */
	static EGTCSurface PhysicalSurfaceToSurface(uint8 PhysicalSurfaceIndex);

	/**
	 * Classify an actor/component tag. "Creature" -> Creature; "Surface.Wood" -> Wood, etc.
	 * Anything unrecognised -> Default. Case-sensitive on the canonical names above.
	 */
	static EGTCSurface SurfaceFromTag(FName Tag);

	/** The effect descriptor for a surface. Always returns a valid ParticlePath (Default if unknown). */
	static FGTCImpactEffect ImpactEffectFor(EGTCSurface Surface);

	/** Human-readable name for logging/debug, e.g. EGTCSurface::Glass -> "Glass". */
	static const TCHAR* SurfaceName(EGTCSurface Surface);
};
