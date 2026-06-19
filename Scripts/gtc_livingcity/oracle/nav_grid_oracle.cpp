// Out-of-tree oracle for FNavGrid — compiles + runs the REAL NavGrid.cpp and
// re-asserts GTC.NPC.NavGrid.
#include "../../../Source/GTC_UE5/NPC/Navigation/NavGrid.h"
#include "harness.h"

static TArray<uint8> AllWalkable(int32 Count)
{
    TArray<uint8> W;
    W.Reserve(Count);
    for (int32 I = 0; I < Count; ++I) { W.Add(1); }
    return W;
}

int main()
{
    FNavGrid::FGrid G; G.Width = 5; G.Height = 5;

    {
        const TArray<uint8> W = AllWalkable(25);
        const TArray<int32> P = FNavGrid::FindPath(G, W, G.Index(0, 0), G.Index(4, 4));
        Check(P.Num() == 5, "diagonal path is 5 cells");
        Check(P[0] == G.Index(0, 0), "path starts at start");
        Check(P.Last() == G.Index(4, 4), "path ends at goal");
    }

    {
        TArray<uint8> W = AllWalkable(25);
        W[G.Index(2, 0)] = 0; W[G.Index(2, 1)] = 0; W[G.Index(2, 2)] = 0; W[G.Index(2, 3)] = 0;
        const TArray<int32> P = FNavGrid::FindPath(G, W, G.Index(0, 0), G.Index(4, 0));
        Check(P.Num() > 0, "detour path exists");
        Check(P.Last() == G.Index(4, 0), "detour reaches goal");
        Check(P.Num() > 5, "detour is longer than the blocked straight line");
        bool bAllWalkable = true;
        for (int32 I = 0; I < P.Num(); ++I) { if (W[P[I]] == 0) { bAllWalkable = false; } }
        Check(bAllWalkable, "detour never enters a wall");
    }

    {
        FNavGrid::FGrid G3; G3.Width = 3; G3.Height = 3;
        TArray<uint8> W = AllWalkable(9);
        W[G3.Index(1, 2)] = 0; W[G3.Index(2, 1)] = 0; W[G3.Index(1, 1)] = 0;
        Check(FNavGrid::FindPath(G3, W, G3.Index(0, 0), G3.Index(2, 2)).IsEmpty(), "unreachable goal -> empty");
    }

    {
        const TArray<uint8> W = AllWalkable(25);
        Check(FNavGrid::FindPath(G, W, 7, 7).Num() == 1, "start == goal -> single cell");
        TArray<uint8> Blocked = AllWalkable(25);
        Blocked[G.Index(0, 0)] = 0;
        Check(FNavGrid::FindPath(G, Blocked, G.Index(0, 0), G.Index(4, 4)).IsEmpty(), "blocked start -> empty");
    }

    {
        FNavGrid::FGrid G3; G3.Width = 3; G3.Height = 3;
        TArray<uint8> W = AllWalkable(9);
        W[G3.Index(1, 0)] = 0; W[G3.Index(0, 1)] = 0;
        Check(!FNavGrid::DiagonalAllowed(G3, W, 0, 0, 1, 1), "diagonal blocked by a corner");
        const TArray<uint8> Open = AllWalkable(9);
        Check(FNavGrid::DiagonalAllowed(G3, Open, 0, 0, 1, 1), "diagonal allowed when corner is clear");
    }

    {
        const FVector Origin(100, 0, 200);
        const double Cell = 50.0;
        const FVector C = FNavGrid::CellToWorld(2, 3, Origin, Cell);
        CheckNear(C.X, 225.0, "cell centre X");
        CheckNear(C.Z, 375.0, "cell centre Z");
        Check(FNavGrid::WorldToCell(G, C, Origin, Cell) == G.Index(2, 3), "world maps back to its cell");
        const TArray<int32> CellPath({G.Index(0, 0), G.Index(1, 1)});
        Check(FNavGrid::ToWorld(G, CellPath, Origin, Cell).Num() == 2, "waypoint count matches cell path");
    }

    return OracleSummary("FNavGrid");
}
