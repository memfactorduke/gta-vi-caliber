// Copyright Epic Games, Inc. All Rights Reserved.

#include "NavGrid.h"
#include "Math/UnrealMathUtility.h"

namespace
{
	constexpr double kBig = 1e300;          // stand-in for +infinity g-score
	const double kSqrt2 = FMath::Sqrt(2.0); // diagonal step cost

	// Octile distance: the exact 8-connected cost over open ground, so A* expands
	// the minimum number of cells. Admissible + consistent.
	double Octile(int32 DX, int32 DY)
	{
		const int32 AX = (int32)FMath::Abs((double)DX);
		const int32 AY = (int32)FMath::Abs((double)DY);
		const int32 Lo = FMath::Min(AX, AY);
		const int32 Hi = FMath::Max(AX, AY);
		return (double)(Hi - Lo) + kSqrt2 * (double)Lo;
	}
}

void FNavGrid::Init(int32 W, int32 H, double InCellSize, const FVector2D& InOrigin)
{
	Width = FMath::Max(0, W);
	Height = FMath::Max(0, H);
	CellSize = InCellSize > 0.0 ? InCellSize : 1.0;
	Origin = InOrigin;
	Walkable.Reset();
	Walkable.Reserve(Width * Height);
	for (int32 i = 0; i < Width * Height; ++i)
	{
		Walkable.Add((uint8)1);
	}
}

bool FNavGrid::IsWalkable(int32 X, int32 Y) const
{
	if (!InBounds(X, Y))
	{
		return false;
	}
	return Walkable[CellIndex(X, Y)] != 0;
}

void FNavGrid::SetWalkable(int32 X, int32 Y, bool bWalkable)
{
	if (InBounds(X, Y))
	{
		Walkable[CellIndex(X, Y)] = bWalkable ? (uint8)1 : (uint8)0;
	}
}

FVector2D FNavGrid::CellToWorld(int32 X, int32 Y) const
{
	return FVector2D(Origin.X + (double)X * CellSize, Origin.Y + (double)Y * CellSize);
}

void FNavGrid::WorldToCell(const FVector2D& P, int32& OutX, int32& OutY) const
{
	const double FX = (P.X - Origin.X) / CellSize;
	const double FY = (P.Y - Origin.Y) / CellSize;
	OutX = FMath::Clamp(FMath::RoundToInt(FX), 0, FMath::Max(0, Width - 1));
	OutY = FMath::Clamp(FMath::RoundToInt(FY), 0, FMath::Max(0, Height - 1));
}

TArray<FNavGridCell> FNavGrid::FindPath(int32 SX, int32 SY, int32 GX, int32 GY) const
{
	TArray<FNavGridCell> Path;
	const int32 N = Width * Height;
	if (N <= 0 || !IsWalkable(SX, SY) || !IsWalkable(GX, GY))
	{
		return Path; // empty: degenerate grid or a solid endpoint
	}

	const int32 Start = CellIndex(SX, SY);
	const int32 Goal = CellIndex(GX, GY);
	if (Start == Goal)
	{
		Path.Add(FNavGridCell(SX, SY));
		return Path;
	}

	// Flat arrays indexed by cell. The open list is a plain index array with lazy
	// deletion: we never remove, just append candidates and skip Closed cells when
	// scanning for the min-f cell. Pedestrian routing is local and small, so the
	// simple O(open) pick is fine; a binary heap is a pure optimisation later.
	TArray<double> G;        // best cost from start
	TArray<double> F;        // G + heuristic
	TArray<int32> Came;      // predecessor cell, -1 = none
	TArray<uint8> Closed;    // finalised
	G.Reserve(N); F.Reserve(N); Came.Reserve(N); Closed.Reserve(N);
	for (int32 i = 0; i < N; ++i)
	{
		G.Add(kBig); F.Add(kBig); Came.Add(-1); Closed.Add((uint8)0);
	}

	TArray<int32> Open;
	G[Start] = 0.0;
	F[Start] = Octile(GX - SX, GY - SY);
	Open.Add(Start);

	// Fixed neighbour order: orthogonals first, then diagonals — a stable order so
	// equal-cost ties always resolve the same way and the path is deterministic.
	static const int32 DX[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
	static const int32 DY[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };

	while (!Open.IsEmpty())
	{
		// Pick the non-closed open cell with the lowest f (tie-break: lowest g,
		// then index). Stale entries of already-closed cells are skipped.
		int32 Current = -1;
		for (int32 s = 0; s < Open.Num(); ++s)
		{
			const int32 C = Open[s];
			if (Closed[C])
			{
				continue;
			}
			if (Current < 0 ||
				F[C] < F[Current] - 1e-9 ||
				(F[C] <= F[Current] + 1e-9 && (G[C] < G[Current] - 1e-9 ||
				(G[C] <= G[Current] + 1e-9 && C < Current))))
			{
				Current = C;
			}
		}
		if (Current < 0)
		{
			break; // open list exhausted (all stale/closed)
		}

		if (Current == Goal)
		{
			break;
		}
		Closed[Current] = 1;

		const int32 CX = Current % Width;
		const int32 CY = Current / Width;
		for (int32 d = 0; d < 8; ++d)
		{
			const int32 NX = CX + DX[d];
			const int32 NY = CY + DY[d];
			if (!IsWalkable(NX, NY))
			{
				continue;
			}
			const bool bDiagonal = (DX[d] != 0 && DY[d] != 0);
			if (bDiagonal)
			{
				// No corner-cutting: both orthogonal neighbours must be open, else
				// the step would clip the corner of a solid cell.
				if (!IsWalkable(CX + DX[d], CY) || !IsWalkable(CX, CY + DY[d]))
				{
					continue;
				}
			}
			const int32 NI = CellIndex(NX, NY);
			if (Closed[NI])
			{
				continue;
			}
			const double Step = bDiagonal ? kSqrt2 : 1.0;
			const double Tentative = G[Current] + Step;
			if (Tentative < G[NI] - 1e-9)
			{
				Came[NI] = Current;
				G[NI] = Tentative;
				F[NI] = Tentative + Octile(GX - NX, GY - NY);
				Open.Add(NI); // lazy: duplicates are skipped once Closed
			}
		}
	}

	if (Came[Goal] < 0 && Goal != Start)
	{
		return Path; // unreachable
	}

	// Walk predecessors back from the goal, then reverse into start->goal order.
	TArray<int32> Rev;
	for (int32 C = Goal; C != -1; C = Came[C])
	{
		Rev.Add(C);
		if (C == Start)
		{
			break;
		}
	}
	for (int32 i = Rev.Num() - 1; i >= 0; --i)
	{
		const int32 C = Rev[i];
		Path.Add(FNavGridCell(C % Width, C / Width));
	}
	return Path;
}

TArray<FVector> FNavGrid::FindWorldPath(const FVector2D& Start, const FVector2D& Goal) const
{
	TArray<FVector> Out;
	int32 SX, SY, GX, GY;
	WorldToCell(Start, SX, SY);
	WorldToCell(Goal, GX, GY);
	const TArray<FNavGridCell> Cells = Simplify(FindPath(SX, SY, GX, GY));
	for (const FNavGridCell& Cell : Cells)
	{
		const FVector2D W = CellToWorld(Cell.X, Cell.Y);
		Out.Add(FVector(W.X, 0.0, W.Y)); // XZ ground plane, height left at 0
	}
	return Out;
}

TArray<FNavGridCell> FNavGrid::Simplify(const TArray<FNavGridCell>& Path)
{
	TArray<FNavGridCell> Out;
	if (Path.Num() <= 2)
	{
		for (const FNavGridCell& C : Path)
		{
			Out.Add(C);
		}
		return Out;
	}
	Out.Add(Path[0]);
	for (int32 i = 1; i < Path.Num() - 1; ++i)
	{
		// Keep i only if the direction changes here. Equal step direction (the
		// grid moves in unit/diagonal steps) means i is mid-run and can be dropped.
		const int32 Pdx = Path[i].X - Path[i - 1].X;
		const int32 Pdy = Path[i].Y - Path[i - 1].Y;
		const int32 Ndx = Path[i + 1].X - Path[i].X;
		const int32 Ndy = Path[i + 1].Y - Path[i].Y;
		if (Pdx != Ndx || Pdy != Ndy)
		{
			Out.Add(Path[i]);
		}
	}
	Out.Add(Path[Path.Num() - 1]);
	return Out;
}
