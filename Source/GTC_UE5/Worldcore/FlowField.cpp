// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlowField.h"

#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

namespace
{
    // 8-connected neighbour offsets and their step lengths (diagonals cost √2).
    // Function-local statics keep these out of file/anon-namespace scope under
    // unity builds while preserving the oracle's exact ordering and step values.
    const int32* NeighbourDx()
    {
        static const int32 Dx[8] = {1, -1, 0, 0, 1, 1, -1, -1};
        return Dx;
    }
    const int32* NeighbourDz()
    {
        static const int32 Dz[8] = {0, 0, 1, -1, 1, -1, 1, -1};
        return Dz;
    }
    const double* NeighbourStep()
    {
        static const double S[8] = {1.0, 1.0, 1.0, 1.0, 1.4142135624, 1.4142135624, 1.4142135624,
            1.4142135624};
        return S;
    }
}

bool FFlowField::DiagonalCutsCorner(const FGrid& G, const TArray<double>& Costs, int32 Cx, int32 Cz, int32 K)
{
    if (K < 4)
    {
        return false;
    }
    const int32* Dx = NeighbourDx();
    const int32* Dz = NeighbourDz();
    const int32 Ax = Cx + Dx[K];
    const int32 Bz = Cz + Dz[K];
    if (!G.InBounds(Ax, Cz) || Costs[G.Index(Ax, Cz)] < 0.0)
    {
        return true;
    }
    if (!G.InBounds(Cx, Bz) || Costs[G.Index(Cx, Bz)] < 0.0)
    {
        return true;
    }
    return false;
}

TArray<double> FFlowField::Integrate(const FGrid& G, const TArray<double>& Costs, int32 GoalCell)
{
    const double Inf = std::numeric_limits<double>::infinity();
    const int32 CellCount = G.Width * G.Height;
    TArray<double> Dist;
    Dist.Init(Inf, CellCount);
    if (GoalCell < 0 || GoalCell >= CellCount || Costs[GoalCell] < 0.0)
    {
        return Dist;
    }
    using Item = std::pair<double, int32>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> Pq;
    Dist[GoalCell] = 0.0;
    Pq.push(std::make_pair(0.0, GoalCell));
    const int32* Dx = NeighbourDx();
    const int32* Dz = NeighbourDz();
    const double* Step = NeighbourStep();
    while (!Pq.empty())
    {
        const double D = Pq.top().first;
        const int32 C = Pq.top().second;
        Pq.pop();
        if (D > Dist[C])
        {
            continue;
        }
        const int32 Cx = C % G.Width;
        const int32 Cz = C / G.Width;
        for (int32 K = 0; K < 8; ++K)
        {
            const int32 Nx = Cx + Dx[K];
            const int32 Nz = Cz + Dz[K];
            if (!G.InBounds(Nx, Nz))
            {
                continue;
            }
            const int32 Nc = G.Index(Nx, Nz);
            if (Costs[Nc] < 0.0)
            {
                continue; // wall
            }
            if (DiagonalCutsCorner(G, Costs, Cx, Cz, K))
            {
                continue; // don't let the field flow diagonally through a corner
            }
            // Charge the cost of the cell entered on the forward move (Nc -> C
            // heads toward the goal, so the agent enters C).
            const double Nd = Dist[C] + Step[K] * Costs[C];
            if (Nd < Dist[Nc])
            {
                Dist[Nc] = Nd;
                Pq.push(std::make_pair(Nd, Nc));
            }
        }
    }
    return Dist;
}

TArray<FVector2D> FFlowField::FlowFrom(const FGrid& G, const TArray<double>& Costs, const TArray<double>& Dist)
{
    TArray<FVector2D> Flow;
    Flow.Init(FVector2D(0.0, 0.0), G.Width * G.Height);
    const int32* Dx = NeighbourDx();
    const int32* Dz = NeighbourDz();
    for (int32 Cz = 0; Cz < G.Height; ++Cz)
    {
        for (int32 Cx = 0; Cx < G.Width; ++Cx)
        {
            const int32 C = G.Index(Cx, Cz);
            if (!std::isfinite(Dist[C]))
            {
                continue;
            }
            double Best = Dist[C];
            int32 BestK = -1;
            for (int32 K = 0; K < 8; ++K)
            {
                const int32 Nx = Cx + Dx[K];
                const int32 Nz = Cz + Dz[K];
                if (!G.InBounds(Nx, Nz))
                {
                    continue;
                }
                if (DiagonalCutsCorner(G, Costs, Cx, Cz, K))
                {
                    continue;
                }
                const double Nd = Dist[G.Index(Nx, Nz)];
                if (Nd < Best)
                {
                    Best = Nd;
                    BestK = K;
                }
            }
            if (BestK >= 0)
            {
                const double Len = std::sqrt(static_cast<double>(
                    Dx[BestK] * Dx[BestK] + Dz[BestK] * Dz[BestK]));
                Flow[C] = FVector2D(Dx[BestK] / Len, Dz[BestK] / Len);
            }
        }
    }
    return Flow;
}
