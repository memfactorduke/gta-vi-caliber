// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure, runtime-free crowd flow-field for worldcore navigation at scale. A flow
 * field is computed ONCE per goal (Dijkstra from the goal over a cost grid -> an
 * integration/distance field -> a per-cell "downhill" direction), then hundreds
 * of agents sample it for free to route around obstacles toward the shared goal.
 *
 * Direct C++->C++ port of the worldcore core
 * engine/src/worldcore/flow_field_core.h (namespace worldcore_flow). All static,
 * no UObject — unit-tested headless via the parity oracle
 * (Tests/FlowFieldTest.cpp, prefix GTC.Worldcore.FlowField).
 *
 * Vector type: the core's plain Vec2{x, z} maps to FVector2D with X<-x, Y<-z.
 * Double precision throughout, matching the C++ oracle's double math.
 * Unreachable cells are +infinity (FMath::IsFinite asserts the guard in tests).
 *
 * PURE-CORE boundary: this is the pure Dijkstra integration + flow-direction math
 * only. Mass / actor integration (agents sampling the field and applying it each
 * tick) is a DEFERRED Wave-3 adapter — the core stays pure, operating on
 * caller-supplied grid/cost state, and is NOT covered by the parity tests.
 */
struct GTC_UE5_API FFlowField
{
    /** Row-major grid descriptor (cells indexed cz * Width + cx). */
    struct FGrid
    {
        int32 Width = 0;
        int32 Height = 0;
        int32 Index(int32 Cx, int32 Cz) const { return Cz * Width + Cx; }
        bool InBounds(int32 Cx, int32 Cz) const
        {
            return Cx >= 0 && Cx < Width && Cz >= 0 && Cz < Height;
        }
    };

    /**
     * Dijkstra integration field from GoalCell. Costs[i] < 0 marks an impassable
     * cell (wall); otherwise Costs[i] is the per-cell movement cost (>=1 typical),
     * charged for the cell being ENTERED toward the goal. Returns per-cell
     * distance-to-goal; unreachable cells are +inf.
     */
    static TArray<double> Integrate(const FGrid& G, const TArray<double>& Costs, int32 GoalCell);

    /**
     * Per-cell flow direction: a unit vector toward the 8-neighbour with the lowest
     * integration distance (downhill toward the goal), skipping diagonals that cut
     * a wall corner. Zero at the goal and on unreachable cells.
     */
    static TArray<FVector2D> FlowFrom(const FGrid& G, const TArray<double>& Costs, const TArray<double>& Dist);

    /**
     * A diagonal step (K >= 4) "cuts a corner" if either of the two orthogonal cells
     * it passes between is a wall. Forbidding it stops agents clipping through wall
     * corners. Returns true when the diagonal from (Cx,Cz) in direction K is blocked.
     */
    static bool DiagonalCutsCorner(const FGrid& G, const TArray<double>& Costs, int32 Cx, int32 Cz, int32 K);
};
