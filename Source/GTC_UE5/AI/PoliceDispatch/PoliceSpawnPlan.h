// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"
#include "PoliceEscalation.h"

/**
 * One concrete officer to spawn this wave: which KIND of unit, and WHERE it
 * appears relative to the player (a planar ring offset). The composition of a
 * whole wave is a TArray of these.
 *
 * `PlanarOffset` is in the pure-core frame the dispatch math is written in —
 * METRES, planar XZ with Y carried as 0 (the Godot frame, NO Z-up remap), an
 * offset FROM the player centre. The Wave-3 actor adapter (AGTCPoliceDirector)
 * owns the conversion to Unreal world space (X->X, Z->Y, metres*100 -> cm) and
 * the NavMesh projection; keeping the plan frame-native is what lets it stay
 * unit-tested headless against the same oracle the dispatch helpers use.
 */
struct FPoliceSpawnSlot
{
    EPoliceUnitType UnitType = EPoliceUnitType::BeatCop;
    FVector PlanarOffset = FVector::ZeroVector;
};

/**
 * Pure-core composition that turns "how wanted is the player" into a concrete
 * spawn list for one reinforcement wave, by stitching together the existing,
 * already-tested escalation primitives so the spawner actor stays a thin shell:
 *   - FPoliceDispatch::SpawnCount   -- HOW MANY to add this wave (deficit, capped)
 *   - FPoliceEscalation::ResponseUnits -- the unit-TYPE mix at this heat
 *   - FPoliceDispatch::RingAngle / RingPosition -- WHERE on the response ring
 *
 * All-static, no UObject, no scene access; RNG is injected (an FRandomStream the
 * caller owns) exactly like the dispatch helpers, so a seeded run is
 * deterministic and the composition is unit-tested headless
 * (Tests/PoliceSpawnPlanTest.cpp, prefix GTC.AI.PoliceDispatch.PoliceSpawnPlan).
 *
 * PURE-CORE boundary: instancing prefabs at the slot positions, frame/unit remap
 * to Unreal world space, NavMesh placement and recall are the Wave-3 adapter --
 * NOT implemented here and NOT covered by the parity test.
 */
class GTC_UE5_API FPoliceSpawnPlan
{
public:
    /** Default radial wobble (metres) so officers don't appear on a perfect circle. */
    static constexpr double DefaultRadialJitterMeters = 6.0;
    /** Default fraction of a ring slice each spawn may wobble within (keeps the fan even). */
    static constexpr double DefaultJitterFrac = 0.8;

    /**
     * Build the concrete spawn list for one wave. Returns an empty array when the
     * deficit is zero or the player is not wanted (ResponseUnits is empty at 0
     * stars). `SpawnRadiusMeters` is the heat's response radius (FPoliceResponse::
     * SpawnRadius). Unit types are dealt round-robin from the escalation mix so a
     * wave reflects the heat's composition even when the wave is smaller than the
     * full mix.
     */
    static TArray<FPoliceSpawnSlot> Build(
        int32 Stars,
        int32 Alive,
        int32 MaxAlive,
        int32 MaxPerWave,
        double SpawnRadiusMeters,
        FRandomStream& Rng,
        double RadialJitterMeters = DefaultRadialJitterMeters,
        double JitterFrac = DefaultJitterFrac);
};
