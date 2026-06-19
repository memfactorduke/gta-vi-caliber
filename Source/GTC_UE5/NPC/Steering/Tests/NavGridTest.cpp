// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NavGrid.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FNavGrid pedestrian routing. Proves peds get the shortest path over
 * open ground, detour around solid cells, never cut a building corner, report no
 * path when boxed in, and that Simplify/FindWorldPath hand the steerer a tidy
 * waypoint list. Prefix GTC.NPC.NavGrid.
 */

namespace
{
	bool PathWalkableChain(const FNavGrid& Grid, const TArray<FNavGridCell>& P, int32 SX, int32 SY, int32 GX, int32 GY)
	{
		if (P.Num() == 0)
		{
			return false;
		}
		if (!(P[0] == FNavGridCell(SX, SY)) || !(P.Last() == FNavGridCell(GX, GY)))
		{
			return false;
		}
		for (int32 i = 0; i < P.Num(); ++i)
		{
			if (!Grid.IsWalkable(P[i].X, P[i].Y))
			{
				return false;
			}
			if (i > 0)
			{
				const int32 dx = FMath::Abs(P[i].X - P[i - 1].X);
				const int32 dy = FMath::Abs(P[i].Y - P[i - 1].Y);
				if (dx > 1 || dy > 1 || (dx == 0 && dy == 0))
				{
					return false;
				}
			}
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNavGridPathTest,
	"GTC.NPC.NavGrid.Path",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNavGridPathTest::RunTest(const FString& Parameters)
{
	FNavGrid Grid;
	Grid.Init(8, 8, 1.0, FVector2D(0.0, 0.0));

	const TArray<FNavGridCell> Straight = Grid.FindPath(0, 0, 5, 0);
	TestEqual(TEXT("open straight path is 6 cells"), Straight.Num(), 6);
	TestTrue(TEXT("open straight path valid"), PathWalkableChain(Grid, Straight, 0, 0, 5, 0));

	const TArray<FNavGridCell> Diag = Grid.FindPath(0, 0, 4, 4);
	TestEqual(TEXT("open diagonal path is 5 cells"), Diag.Num(), 5);
	TestTrue(TEXT("open diagonal path valid"), PathWalkableChain(Grid, Diag, 0, 0, 4, 4));

	const TArray<FNavGridCell> Same = Grid.FindPath(3, 3, 3, 3);
	TestEqual(TEXT("start==goal single cell"), Same.Num(), 1);

	// Deterministic across calls.
	const TArray<FNavGridCell> Again = Grid.FindPath(0, 0, 5, 0);
	bool bIdentical = (Again.Num() == Straight.Num());
	for (int32 i = 0; bIdentical && i < Again.Num(); ++i)
	{
		bIdentical = (Again[i] == Straight[i]);
	}
	TestTrue(TEXT("path is deterministic"), bIdentical);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNavGridDetourTest,
	"GTC.NPC.NavGrid.Detour",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNavGridDetourTest::RunTest(const FString& Parameters)
{
	FNavGrid Grid;
	Grid.Init(7, 7, 1.0, FVector2D(0.0, 0.0));
	for (int32 y = 0; y <= 5; ++y)
	{
		Grid.SetWalkable(3, y, false); // wall with an open end at y=6
	}

	const TArray<FNavGridCell> P = Grid.FindPath(0, 0, 6, 0);
	TestTrue(TEXT("detour valid + walkable"), PathWalkableChain(Grid, P, 0, 0, 6, 0));
	bool bAvoidsWall = true;
	int32 MaxY = 0;
	for (const FNavGridCell& C : P)
	{
		if (C.X == 3 && C.Y <= 5)
		{
			bAvoidsWall = false;
		}
		MaxY = FMath::Max(MaxY, C.Y);
	}
	TestTrue(TEXT("detour never steps on the wall"), bAvoidsWall);
	TestTrue(TEXT("detour routes around the open end"), MaxY >= 6);
	TestTrue(TEXT("detour longer than blocked straight line"), P.Num() > 7);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNavGridUnreachableTest,
	"GTC.NPC.NavGrid.Unreachable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNavGridUnreachableTest::RunTest(const FString& Parameters)
{
	FNavGrid Grid;
	Grid.Init(7, 7, 1.0, FVector2D(0.0, 0.0));
	for (int32 dx = -1; dx <= 1; ++dx)
	{
		for (int32 dy = -1; dy <= 1; ++dy)
		{
			if (dx == 0 && dy == 0)
			{
				continue;
			}
			Grid.SetWalkable(3 + dx, 3 + dy, false);
		}
	}
	TestEqual(TEXT("enclosed goal yields no path"), Grid.FindPath(0, 0, 3, 3).Num(), 0);

	Grid.SetWalkable(1, 1, false);
	TestEqual(TEXT("solid start yields no path"), Grid.FindPath(1, 1, 6, 6).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNavGridCornerCutTest,
	"GTC.NPC.NavGrid.CornerCut",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNavGridCornerCutTest::RunTest(const FString& Parameters)
{
	FNavGrid Grid;
	Grid.Init(3, 3, 1.0, FVector2D(0.0, 0.0));
	Grid.SetWalkable(1, 0, false);
	Grid.SetWalkable(0, 1, false);
	// Both orthogonals between (0,0) and (1,1) are solid: the diagonal would clip
	// a corner, so it must be forbidden — isolating (0,0) entirely here.
	const TArray<FNavGridCell> P = Grid.FindPath(0, 0, 1, 1);
	bool bNoClip = true;
	for (int32 i = 1; i < P.Num(); ++i)
	{
		const int32 dx = P[i].X - P[i - 1].X;
		const int32 dy = P[i].Y - P[i - 1].Y;
		if (dx != 0 && dy != 0)
		{
			if (!Grid.IsWalkable(P[i - 1].X + dx, P[i - 1].Y) || !Grid.IsWalkable(P[i - 1].X, P[i - 1].Y + dy))
			{
				bNoClip = false;
			}
		}
	}
	TestTrue(TEXT("no diagonal step cuts a solid corner"), bNoClip);
	TestEqual(TEXT("corner-blocked start is isolated"), P.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNavGridSimplifyTest,
	"GTC.NPC.NavGrid.Simplify",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNavGridSimplifyTest::RunTest(const FString& Parameters)
{
	FNavGrid Grid;
	Grid.Init(10, 10, 2.0, FVector2D(100.0, 200.0));

	const TArray<FNavGridCell> Raw = Grid.FindPath(0, 0, 6, 0);
	const TArray<FNavGridCell> Simp = FNavGrid::Simplify(Raw);
	TestEqual(TEXT("raw straight run is 7 cells"), Raw.Num(), 7);
	TestEqual(TEXT("straight run simplifies to 2 endpoints"), Simp.Num(), 2);

	int32 rx, ry;
	Grid.WorldToCell(Grid.CellToWorld(3, 4), rx, ry);
	TestTrue(TEXT("world<->cell round-trips"), rx == 3 && ry == 4);

	const TArray<FVector> WP = Grid.FindWorldPath(Grid.CellToWorld(0, 0), Grid.CellToWorld(6, 0));
	TestEqual(TEXT("world path returns 2 simplified waypoints"), WP.Num(), 2);
	TestTrue(TEXT("waypoints sit on the XZ ground plane"), FMath::Abs(WP[0].Y) < Eps);
	TestTrue(TEXT("first waypoint at grid origin X"), FMath::Abs(WP[0].X - 100.0) < Eps);
	TestTrue(TEXT("last waypoint at goal X"), FMath::Abs(WP.Last().X - (100.0 + 6 * 2.0)) < Eps);
	return true;
}

#endif // WITH_AUTOMATION_TESTS
