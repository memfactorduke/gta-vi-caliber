// Copyright Epic Games, Inc. All Rights Reserved.

#include "NavGrid.h"

#include "Algo/Reverse.h"
#include "Math/UnrealMathUtility.h"

#include <limits>

namespace
{
    constexpr double Sqrt2 = 1.4142135623730951;

    // 8 neighbour steps in a FIXED order so the search is deterministic. The four
    // orthogonals first (cost 1), then the four diagonals (cost sqrt2).
    struct FStep { int32 Dx; int32 Dz; double Cost; };
    const FStep Steps[8] = {
        {1, 0, 1.0}, {-1, 0, 1.0}, {0, 1, 1.0}, {0, -1, 1.0},
        {1, 1, Sqrt2}, {1, -1, Sqrt2}, {-1, 1, Sqrt2}, {-1, -1, Sqrt2},
    };

    // Octile heuristic: straight + diagonal blend, admissible on an 8-grid.
    double Octile(int32 Cx, int32 Cz, int32 Gx, int32 Gz)
    {
        const int32 Dx = FMath::Abs(Cx - Gx);
        const int32 Dz = FMath::Abs(Cz - Gz);
        const int32 Lo = FMath::Min(Dx, Dz);
        const int32 Hi = FMath::Max(Dx, Dz);
        return (Hi - Lo) + Sqrt2 * Lo;
    }
}

bool FNavGrid::DiagonalAllowed(
    const FGrid& G, const TArray<uint8>& Walkable, int32 Cx, int32 Cz, int32 Dx, int32 Dz)
{
    if (Dx == 0 || Dz == 0)
    {
        return true; // orthogonal step never cuts a corner
    }
    // Both orthogonal cells the diagonal passes between must be walkable.
    const bool bSideX = G.InBounds(Cx + Dx, Cz) && Walkable[G.Index(Cx + Dx, Cz)] != 0;
    const bool bSideZ = G.InBounds(Cx, Cz + Dz) && Walkable[G.Index(Cx, Cz + Dz)] != 0;
    return bSideX && bSideZ;
}

TArray<int32> FNavGrid::FindPath(
    const FGrid& G, const TArray<uint8>& Walkable, int32 StartCell, int32 GoalCell)
{
    TArray<int32> Empty;
    const int32 N = G.Count();
    if (N <= 0 || Walkable.Num() < N)
    {
        return Empty;
    }
    if (StartCell < 0 || StartCell >= N || GoalCell < 0 || GoalCell >= N)
    {
        return Empty;
    }
    if (Walkable[StartCell] == 0 || Walkable[GoalCell] == 0)
    {
        return Empty;
    }
    if (StartCell == GoalCell)
    {
        return TArray<int32>({StartCell});
    }

    const int32 Gx = GoalCell % G.Width;
    const int32 Gz = GoalCell / G.Width;

    const double Inf = std::numeric_limits<double>::infinity();
    TArray<double> GScore;
    TArray<double> FScore;
    TArray<int32> Came;
    TArray<uint8> InOpen;
    GScore.Reserve(N); FScore.Reserve(N); Came.Reserve(N); InOpen.Reserve(N);
    for (int32 I = 0; I < N; ++I)
    {
        GScore.Add(Inf); FScore.Add(Inf); Came.Add(-1); InOpen.Add(0);
    }

    GScore[StartCell] = 0.0;
    FScore[StartCell] = Octile(StartCell % G.Width, StartCell / G.Width, Gx, Gz);
    // Ordered open set: first-inserted wins ties (deterministic, like FRoadNetwork).
    TArray<int32> Open;
    Open.Add(StartCell);
    InOpen[StartCell] = 1;

    int32 Guard = 0;
    const int32 Budget = N + 1;
    while (!Open.IsEmpty() && Guard < Budget)
    {
        ++Guard;
        // Pick the open cell with the lowest f (strict <, first wins).
        int32 CurIdx = 0;
        double Best = FScore[Open[0]];
        for (int32 K = 1; K < Open.Num(); ++K)
        {
            if (FScore[Open[K]] < Best)
            {
                Best = FScore[Open[K]];
                CurIdx = K;
            }
        }
        const int32 Cur = Open[CurIdx];
        if (Cur == GoalCell)
        {
            break;
        }
        Open.RemoveAtSwap(CurIdx);
        InOpen[Cur] = 0;

        const int32 Cx = Cur % G.Width;
        const int32 Cz = Cur / G.Width;
        for (const FStep& S : Steps)
        {
            const int32 Nx = Cx + S.Dx;
            const int32 Nz = Cz + S.Dz;
            if (!G.InBounds(Nx, Nz))
            {
                continue;
            }
            const int32 Nb = G.Index(Nx, Nz);
            if (Walkable[Nb] == 0)
            {
                continue;
            }
            if (!DiagonalAllowed(G, Walkable, Cx, Cz, S.Dx, S.Dz))
            {
                continue;
            }
            const double Tentative = GScore[Cur] + S.Cost;
            if (Tentative < GScore[Nb])
            {
                Came[Nb] = Cur;
                GScore[Nb] = Tentative;
                FScore[Nb] = Tentative + Octile(Nx, Nz, Gx, Gz);
                if (InOpen[Nb] == 0)
                {
                    Open.Add(Nb);
                    InOpen[Nb] = 1;
                }
            }
        }
    }

    if (Came[GoalCell] == -1 && StartCell != GoalCell)
    {
        return Empty; // unreachable
    }

    TArray<int32> Path;
    Path.Add(GoalCell);
    int32 C = GoalCell;
    while (C != StartCell)
    {
        C = Came[C];
        Path.Add(C);
    }
    Algo::Reverse(Path);
    return Path;
}

FVector FNavGrid::CellToWorld(int32 Cx, int32 Cz, const FVector& Origin, double CellSize)
{
    return FVector(
        Origin.X + (static_cast<double>(Cx) + 0.5) * CellSize,
        Origin.Y,
        Origin.Z + (static_cast<double>(Cz) + 0.5) * CellSize);
}

int32 FNavGrid::WorldToCell(const FGrid& G, const FVector& World, const FVector& Origin, double CellSize)
{
    const int32 Cx = static_cast<int32>(FMath::FloorToDouble((World.X - Origin.X) / CellSize));
    const int32 Cz = static_cast<int32>(FMath::FloorToDouble((World.Z - Origin.Z) / CellSize));
    return G.Index(Cx, Cz);
}

TArray<FVector> FNavGrid::ToWorld(
    const FGrid& G, const TArray<int32>& CellPath, const FVector& Origin, double CellSize)
{
    TArray<FVector> Out;
    Out.Reserve(CellPath.Num());
    for (int32 I = 0; I < CellPath.Num(); ++I)
    {
        const int32 Cell = CellPath[I];
        Out.Add(CellToWorld(Cell % G.Width, Cell / G.Width, Origin, CellSize));
    }
    return Out;
}
