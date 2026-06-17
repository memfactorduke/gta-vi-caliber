// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure spawn-plan model for the wanted-level police response: how many officers
 * should be in the field at a given heat, where to spawn reinforcements (rings
 * that widen with the stars), and which existing units to recall.
 *
 * Direct 1:1 port of the Godot RefCounted `PoliceDispatch`
 * (game/scripts/ai/police_dispatch.gd). All-static plain functions, no UObject,
 * no scene/RNG/node state -- RNG samples are injected by the caller -- so the
 * escalation math is deterministic and unit-tested
 * (Tests/PoliceDispatchTest.cpp, prefix GTC.AI.PoliceDispatch.PoliceDispatch).
 *
 * Depends on FPoliceResponse (stars -> head-count) which is already merged at
 * World/Police/PoliceResponse.h.
 *
 * "Planar" means the XZ plane (X = cos(angle)*r, Z = sin(angle)*r) with Y carried
 * from the spawn centre -- ported VERBATIM from the Godot source's Vector3 math
 * with NO Z-up remap, matching the bit-for-bit oracle.
 *
 * Double precision throughout (LWC) to match the Godot oracle's float arithmetic.
 *
 * PURE-CORE boundary: these helpers take positions/counts/RNG samples as plain
 * inputs. Actual unit spawning (instancing police prefabs at the returned
 * positions), freeing recalled units, and navmesh/world placement are the
 * DEFERRED Wave-3 adapter -- not implemented here and not covered by parity tests.
 */

class GTC_UE5_API FPoliceDispatch
{
public:
    /** How many officers should be live at this heat, capped by the scene budget. */
    static int32 DesiredUnits(int32 Stars, int32 MaxAlive);

    /**
     * How many to spawn this wave: the deficit toward DesiredUnits, never negative
     * and never more than MaxPerWave (so reinforcements trickle in as expanding
     * pressure rather than popping in all at once).
     */
    static int32 SpawnCount(int32 Stars, int32 Alive, int32 MaxAlive, int32 MaxPerWave);

    /**
     * Angle (radians) for the index-th of `Total` spawns: an even slice of the ring
     * plus jitter inside that slice, so a wave fans out around the player instead
     * of stacking on one bearing. `UJitter` in [0,1); `JitterFrac` scales the wobble.
     */
    static double RingAngle(int32 Index, int32 Total, double UJitter, double JitterFrac);

    /**
     * A spawn point on the response ring around `Center`. `Radius` is the heat's
     * spawn radius; `URadius` in [0,1) jitters the distance by up to `RadialJitter`
     * metres so officers don't appear on a perfect circle. Planar XZ, Y from Center.
     */
    static FVector RingPosition(
        const FVector& Center, double Radius, double Angle, double URadius, double RadialJitter);

    /**
     * Whether a live officer should be recalled (freed): once the player is no
     * longer wanted everyone stands down, and anyone who falls beyond
     * `DespawnRadius` is culled so the spawner can re-place fresh pressure closer in.
     */
    static bool ShouldDespawn(int32 Stars, double DistanceToPlayer, double DespawnRadius);
};
