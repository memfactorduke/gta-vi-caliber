// Out-of-tree oracle for FNavGrid — compiles + runs the REAL NavGrid.cpp and
// re-asserts the GTC.NPC.NavGrid cases: peds route through walkable space and
// around solid cells, never cutting building corners, with a deterministic path.
// Build (standalone): clang++ -std=c++17 -I ue_shim navgrid_oracle.cpp \
//   ../../../Source/GTC_UE5/NPC/Steering/NavGrid.cpp -o /tmp/navgrid && /tmp/navgrid
// In run.sh:  build_run navgrid_oracle.cpp NPC/Steering/NavGrid.cpp
#include "../../../Source/GTC_UE5/NPC/Steering/NavGrid.h"
#include "harness.h"

// Are all cells in the path walkable, and is it a connected 8-neighbour chain
// from start to goal?
static bool PathValid(const FNavGrid& Grid, const TArray<FNavGridCell>& P, int32 SX, int32 SY, int32 GX, int32 GY)
{
	if (P.Num() == 0) { return false; }
	if (!(P[0] == FNavGridCell(SX, SY)) || !(P[P.Num() - 1] == FNavGridCell(GX, GY))) { return false; }
	for (int32 i = 0; i < P.Num(); ++i)
	{
		if (!Grid.IsWalkable(P[i].X, P[i].Y)) { return false; }
		if (i > 0)
		{
			const int32 dx = (int32)std::abs(P[i].X - P[i - 1].X);
			const int32 dy = (int32)std::abs(P[i].Y - P[i - 1].Y);
			if (dx > 1 || dy > 1 || (dx == 0 && dy == 0)) { return false; }
		}
	}
	return true;
}

int main()
{
	// --- Open grid: straight and diagonal shortest paths -------------------
	{
		FNavGrid Grid;
		Grid.Init(8, 8, 1.0, FVector2D(0.0, 0.0));

		const TArray<FNavGridCell> Straight = Grid.FindPath(0, 0, 5, 0);
		Check(Straight.Num() == 6, "open straight path is 6 cells");
		Check(PathValid(Grid, Straight, 0, 0, 5, 0), "open straight path valid");
		bool flat = true;
		for (const FNavGridCell& C : Straight) { if (C.Y != 0) { flat = false; } }
		Check(flat, "straight path stays on its row");

		const TArray<FNavGridCell> Diag = Grid.FindPath(0, 0, 4, 4);
		Check(Diag.Num() == 5, "open diagonal path is 5 cells");
		Check(PathValid(Grid, Diag, 0, 0, 4, 4), "open diagonal path valid");

		// Start == goal degenerates to a single cell.
		const TArray<FNavGridCell> Same = Grid.FindPath(3, 3, 3, 3);
		Check(Same.Num() == 1 && Same[0] == FNavGridCell(3, 3), "start==goal single cell");

		// Deterministic: same request, same path.
		const TArray<FNavGridCell> Again = Grid.FindPath(0, 0, 5, 0);
		bool identical = (Again.Num() == Straight.Num());
		for (int32 i = 0; identical && i < Again.Num(); ++i) { if (!(Again[i] == Straight[i])) { identical = false; } }
		Check(identical, "path is deterministic across calls");
	}

	// --- Wall forces a detour around solid cells ---------------------------
	{
		FNavGrid Grid;
		Grid.Init(7, 7, 1.0, FVector2D(0.0, 0.0));
		// Vertical wall at x=3 spanning y=0..5, leaving a gap at y=6 to go around.
		for (int32 y = 0; y <= 5; ++y) { Grid.SetWalkable(3, y, false); }

		const TArray<FNavGridCell> P = Grid.FindPath(0, 0, 6, 0);
		Check(PathValid(Grid, P, 0, 0, 6, 0), "detour path valid + walkable");
		bool avoidsWall = true;
		int32 maxY = 0;
		for (const FNavGridCell& C : P)
		{
			if (C.X == 3 && C.Y <= 5) { avoidsWall = false; }
			if (C.Y > maxY) { maxY = C.Y; }
		}
		Check(avoidsWall, "detour never steps on the wall");
		Check(maxY >= 6, "detour routes around the wall's open end");
		// The detour must be longer than the blocked straight shot of 7 cells.
		Check(P.Num() > 7, "detour is longer than the blocked straight line");
	}

	// --- Enclosed goal is unreachable -> empty -----------------------------
	{
		FNavGrid Grid;
		Grid.Init(7, 7, 1.0, FVector2D(0.0, 0.0));
		// Box the goal (3,3) in solid cells on all 8 sides.
		for (int32 dx = -1; dx <= 1; ++dx)
		{
			for (int32 dy = -1; dy <= 1; ++dy)
			{
				if (dx == 0 && dy == 0) { continue; }
				Grid.SetWalkable(3 + dx, 3 + dy, false);
			}
		}
		const TArray<FNavGridCell> P = Grid.FindPath(0, 0, 3, 3);
		Check(P.Num() == 0, "enclosed goal yields no path");

		// A solid start/goal also yields no path.
		Grid.SetWalkable(1, 1, false);
		Check(Grid.FindPath(1, 1, 6, 6).Num() == 0, "solid start yields no path");
	}

	// --- No corner cutting through a solid diagonal block ------------------
	{
		FNavGrid Grid;
		Grid.Init(3, 3, 1.0, FVector2D(0.0, 0.0));
		// Block the two orthogonal cells between (0,0) and (1,1) so the only way
		// across would be to clip the corner — which must be forbidden.
		Grid.SetWalkable(1, 0, false);
		Grid.SetWalkable(0, 1, false);
		const TArray<FNavGridCell> P = Grid.FindPath(0, 0, 1, 1);
		// (1,1) is now only reachable via (2,..) cells; never a direct corner clip.
		bool noClip = true;
		for (int32 i = 1; i < P.Num(); ++i)
		{
			const int32 dx = P[i].X - P[i - 1].X;
			const int32 dy = P[i].Y - P[i - 1].Y;
			if (dx != 0 && dy != 0)
			{
				// diagonal step: both orthogonals must have been open
				if (!Grid.IsWalkable(P[i - 1].X + dx, P[i - 1].Y) || !Grid.IsWalkable(P[i - 1].X, P[i - 1].Y + dy))
				{
					noClip = false;
				}
			}
		}
		Check(noClip, "no diagonal step cuts a solid corner");
		// With (1,0) and (0,1) solid in a 3x3, (1,1) is reachable via (2,*).
		Check(P.Num() == 0 || PathValid(Grid, P, 0, 0, 1, 1), "corner case path valid if any");
	}

	// --- Simplify keeps only the bends; world path feeds the steerer -------
	{
		FNavGrid Grid;
		Grid.Init(10, 10, 2.0, FVector2D(100.0, 200.0));
		const TArray<FNavGridCell> Raw = Grid.FindPath(0, 0, 6, 0);
		const TArray<FNavGridCell> Simp = FNavGrid::Simplify(Raw);
		Check(Raw.Num() == 7, "raw straight run is 7 cells");
		Check(Simp.Num() == 2, "straight run simplifies to 2 endpoints");

		// World round-trip: cell (3,4) center maps back to cell (3,4).
		const FVector2D W = Grid.CellToWorld(3, 4);
		int32 rx, ry; Grid.WorldToCell(W, rx, ry);
		Check(rx == 3 && ry == 4, "world<->cell round-trips");

		const TArray<FVector> WP = Grid.FindWorldPath(Grid.CellToWorld(0, 0), Grid.CellToWorld(6, 0));
		Check(WP.Num() == 2, "world path returns the 2 simplified waypoints");
		Check(WP[0].Y == 0.0, "waypoints sit on the XZ ground plane");
		CheckNear(WP[0].X, 100.0, "first waypoint at grid origin X");
		CheckNear(WP[WP.Num() - 1].X, 100.0 + 6 * 2.0, "last waypoint at goal X");
	}

	// An L-shaped route bends once -> simplifies to 3 points (start, corner, end).
	{
		FNavGrid Grid;
		Grid.Init(8, 8, 1.0, FVector2D(0.0, 0.0));
		// Wall makes the path go right then up: force an L by blocking a column.
		for (int32 y = 1; y <= 7; ++y) { Grid.SetWalkable(2, y, false); }
		const TArray<FNavGridCell> Raw = Grid.FindPath(0, 5, 4, 5);
		const TArray<FNavGridCell> Simp = FNavGrid::Simplify(Raw);
		Check(Raw.Num() >= 2, "L route found");
		Check(Simp.Num() <= Raw.Num(), "simplify never grows the path");
		Check(Simp.Num() >= 2, "simplify keeps both endpoints");
	}

	return OracleSummary("navgrid_oracle");
}
