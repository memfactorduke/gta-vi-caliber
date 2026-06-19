// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NavGrid.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FNavGrid — pedestrian grid A*. GTC-original. Pins straight-line
 * routing, detour around a wall (never entering a blocked cell), unreachability,
 * corner-cut prevention, and the cell<->world mapping the steerer consumes.
 */

namespace
{
    TArray<uint8> AllWalkable(int32 Count)
    {
        TArray<uint8> W;
        W.Reserve(Count);
        for (int32 I = 0; I < Count; ++I) { W.Add(1); }
        return W;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNavGridTest,
    "GTC.NPC.NavGrid",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNavGridTest::RunTest(const FString& Parameters)
{
    FNavGrid::FGrid G;
    G.Width = 5;
    G.Height = 5;

    // Open field: a clean diagonal of 5 cells from (0,0) to (4,4).
    {
        const TArray<uint8> W = AllWalkable(25);
        const TArray<int32> P = FNavGrid::FindPath(G, W, G.Index(0, 0), G.Index(4, 4));
        TestEqual(TEXT("diagonal path is 5 cells"), P.Num(), 5);
        TestEqual(TEXT("path starts at start"), P[0], G.Index(0, 0));
        TestEqual(TEXT("path ends at goal"), P.Last(), G.Index(4, 4));
    }

    // Wall column forces a detour; the path must avoid every blocked cell.
    {
        TArray<uint8> W = AllWalkable(25);
        W[G.Index(2, 0)] = 0; W[G.Index(2, 1)] = 0; W[G.Index(2, 2)] = 0; W[G.Index(2, 3)] = 0; // gap at (2,4)
        const TArray<int32> P = FNavGrid::FindPath(G, W, G.Index(0, 0), G.Index(4, 0));
        TestTrue(TEXT("detour path exists"), P.Num() > 0);
        TestEqual(TEXT("detour reaches goal"), P.Last(), G.Index(4, 0));
        TestTrue(TEXT("detour is longer than the blocked straight line"), P.Num() > 5);
        bool bAllWalkable = true;
        for (int32 I = 0; I < P.Num(); ++I) { if (W[P[I]] == 0) { bAllWalkable = false; } }
        TestTrue(TEXT("detour never enters a wall"), bAllWalkable);
    }

    // Goal walled off on all approaches -> no path.
    {
        FNavGrid::FGrid G3; G3.Width = 3; G3.Height = 3;
        TArray<uint8> W = AllWalkable(9);
        W[G3.Index(1, 2)] = 0; W[G3.Index(2, 1)] = 0; W[G3.Index(1, 1)] = 0;
        const TArray<int32> P = FNavGrid::FindPath(G3, W, G3.Index(0, 0), G3.Index(2, 2));
        TestTrue(TEXT("unreachable goal -> empty"), P.IsEmpty());
    }

    // Degenerate / invalid requests.
    {
        const TArray<uint8> W = AllWalkable(25);
        TestEqual(TEXT("start == goal -> single cell"), FNavGrid::FindPath(G, W, 7, 7).Num(), 1);
        TArray<uint8> Blocked = AllWalkable(25);
        Blocked[G.Index(0, 0)] = 0;
        TestTrue(TEXT("blocked start -> empty"), FNavGrid::FindPath(G, Blocked, G.Index(0, 0), G.Index(4, 4)).IsEmpty());
    }

    // Corner-cut prevention.
    {
        FNavGrid::FGrid G3; G3.Width = 3; G3.Height = 3;
        TArray<uint8> W = AllWalkable(9);
        W[G3.Index(1, 0)] = 0; W[G3.Index(0, 1)] = 0; // the two cells the (0,0)->(1,1) diagonal passes between
        TestFalse(TEXT("diagonal blocked by a corner"), FNavGrid::DiagonalAllowed(G3, W, 0, 0, 1, 1));
        const TArray<uint8> Open = AllWalkable(9);
        TestTrue(TEXT("diagonal allowed when corner is clear"), FNavGrid::DiagonalAllowed(G3, Open, 0, 0, 1, 1));
    }

    // Cell <-> world mapping.
    {
        const FVector Origin(100, 0, 200);
        const double Cell = 50.0;
        const FVector C = FNavGrid::CellToWorld(2, 3, Origin, Cell);
        TestTrue(TEXT("cell centre X"), FMath::Abs(C.X - 225.0) <= Eps);
        TestTrue(TEXT("cell centre Z"), FMath::Abs(C.Z - 375.0) <= Eps);
        TestEqual(TEXT("world maps back to its cell"), FNavGrid::WorldToCell(G, C, Origin, Cell), G.Index(2, 3));

        const TArray<int32> CellPath({G.Index(0, 0), G.Index(1, 1)});
        const TArray<FVector> WP = FNavGrid::ToWorld(G, CellPath, Origin, Cell);
        TestEqual(TEXT("waypoint count matches cell path"), WP.Num(), 2);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
