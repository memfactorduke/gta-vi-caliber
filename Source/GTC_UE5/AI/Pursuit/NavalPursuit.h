// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Naval pursuit geometry — the Coast Guard's brain for running down a fleeing boat on the
 * water plane. It is the waterborne sibling of FPursuitTactics (the road-cop tactics) and
 * deliberately REUSES it for the hard part (lead-intercept, the ram decision) rather than
 * re-deriving the math — but FPursuitTactics works in the legacy Godot XZ frame (it
 * flattens Y), so this core remaps UE Z-up <-> that frame at the boundary (swap Y<->Z) and
 * keeps its OWN public API in clean UE Z-up like the rest of Wings & Waves. On top of the
 * reuse it adds the two naval-specific moves: a broadside firing run abeam the target, and
 * staying in navigable water (riding the surface, grounding on the shore rather than
 * driving through it).
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject —
 * stateless static helpers. Frame: PUBLIC api is UE Z-up (water plane = XY); the Godot
 * remap is internal only. Positions/distances cm, stars integers. Unit-tested headless
 * (AI/Pursuit/Tests/NavalPursuitTest.cpp, prefix GTC.AI.Pursuit.NavalPursuit).
 */
struct GTC_UE5_API FNavalPursuit
{
    /** Star at which the Coast Guard engages (matches FAirSeaEscalation::CoastGuardStars). */
    static constexpr int32 DeployStars = 4;

    /**
     * Lead-intercept aim point on the water (UE Z-up). Delegates to
     * FPursuitTactics::InterceptPoint with the Godot Y<->Z remap; falls back to the
     * target's position when there's no useful solution (handled by the delegate).
     */
    static FVector InterceptPoint(const FVector& TargetPos, const FVector& TargetVel,
        const FVector& PursuerPos, double PursuerSpeed);

    /**
     * A firing-run point abeam the target: directly to one `Side` (-1 port, +1 starboard
     * of the target's heading) at `StandoffCm`, so the interceptor runs parallel and the
     * gunner gets a side shot. Keeps the target's water Z. With no target heading it steps
     * out along world +/-Y.
     */
    static FVector BroadsideOffset(const FVector& TargetPos, const FVector& TargetVel,
        double Side, double StandoffCm);

    /**
     * Keep a desired point navigable: a boat rides the water surface (SeaLevelZ) unless the
     * shore/seabed there pokes above it, in which case it grounds on the shore. Returns the
     * point with Z = max(SeaLevelZ, ShoreHeightAtDesiredCm); XY unchanged.
     */
    static FVector ClampToWater(const FVector& Desired, double ShoreHeightAtDesiredCm, double SeaLevelZCm);

    /**
     * Whether to ram the fleeing hull: close, lined up, and the heat high enough. Delegates
     * to FPursuitTactics::ShouldRam with the Godot remap.
     */
    static bool ShouldRamHull(const FVector& PursuerPos, const FVector& PursuerHeading,
        const FVector& TargetPos, double RamRangeCm, int32 Stars);
};
