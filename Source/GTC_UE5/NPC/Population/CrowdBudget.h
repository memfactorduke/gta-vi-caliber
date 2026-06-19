// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The streaming arithmetic behind UGTCCrowdSubsystem: how many citizens to spawn
 * this pass, where they may appear, which to retire, and how often each ticks —
 * the trick that makes a district feel densely peopled without simulating a whole
 * city, kept invisible to the player. The subsystem ran this inline; FCrowdBudget
 * lifts it into pure-core so the budgeting is unit-testable (ROADMAP M4: crowd
 * spawn/despawn around the camera).
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject
 * — unit-tested headless (Tests/CrowdBudgetTest.cpp, prefix GTC.NPC.CrowdBudget).
 * Distances are whatever unit the caller uses (cm in the live subsystem); the
 * model only compares them, so it is unit-agnostic.
 *
 * PURE-CORE boundary: actually spawning/retiring AGTCCitizen pawns, projecting to
 * the navmesh, and applying the tick interval is the subsystem's job (the DEFERRED
 * adapter) and is NOT covered by the tests.
 */
struct GTC_UE5_API FCrowdBudget
{
    /**
     * How many to spawn this pass: the deficit (Target - Live) clamped to
     * [0, MaxSpawnsPerPass], so the headcount eases up to target without a spike
     * and never over-fills. 0 once at/over target.
     */
    static int32 SpawnCount(int32 LiveCount, int32 TargetPopulation, int32 MaxSpawnsPerPass);

    /** A citizen this far from the camera should be retired (strictly beyond the radius). */
    static bool ShouldDespawn(double DistanceFromCamera, double DespawnRadius);

    /** Is a candidate spawn point in the just-out-of-view ring [RingMin, RingMax]? */
    static bool InSpawnRing(double DistanceFromCamera, double RingMin, double RingMax);

    /**
     * Distance-LOD tick interval: 0 (every frame) within NearRadius, else
     * FarInterval, so a distant crowd stays affordable.
     */
    static double TickInterval(double DistanceFromCamera, double NearRadius, double FarInterval);

    /**
     * Index of the best retire candidate — the FARTHEST citizen beyond
     * DespawnRadius — so a per-pass retire budget sheds the most-distant bodies
     * first. -1 when none is beyond the radius. Ties keep the lower index.
     */
    static int32 FarthestBeyond(const TArray<double>& Distances, double DespawnRadius);
};
