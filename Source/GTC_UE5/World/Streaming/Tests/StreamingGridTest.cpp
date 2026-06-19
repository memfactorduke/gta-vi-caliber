// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../StreamingGrid.h"
#include "../../../Tests/GtcTestTolerances.h"

/**
 * Unit tests for FStreamingGrid (the streaming tile-coordinate substrate).
 * Prefix GTC.World.Streaming.Grid. Distances are world units; FVector2D
 * components are doubles, so numeric comparisons use double literals and the
 * shared Eps tolerance.
 */

using GtcTest::Eps;

// --- WorldToTile ----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStreamingGridWorldToTileTest,
	"GTC.World.Streaming.Grid.WorldToTile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStreamingGridWorldToTileTest::RunTest(const FString& Parameters)
{
	const double Size = 128.0;

	TestEqual(TEXT("origin -> tile 0,0"), FStreamingGrid::WorldToTile(FVector2D(0.0, 0.0), Size), FIntPoint(0, 0));
	TestEqual(TEXT("inside first tile"), FStreamingGrid::WorldToTile(FVector2D(127.9, 1.0), Size), FIntPoint(0, 0));
	TestEqual(TEXT("exact boundary rolls up"), FStreamingGrid::WorldToTile(FVector2D(128.0, 0.0), Size), FIntPoint(1, 0));
	// Negative coordinates must floor, not truncate toward zero.
	TestEqual(TEXT("just below origin"), FStreamingGrid::WorldToTile(FVector2D(-1.0, 0.0), Size), FIntPoint(-1, 0));
	TestEqual(TEXT("neg boundary"), FStreamingGrid::WorldToTile(FVector2D(-128.0, 0.0), Size), FIntPoint(-1, 0));
	TestEqual(TEXT("just past neg boundary"), FStreamingGrid::WorldToTile(FVector2D(-129.0, 0.0), Size), FIntPoint(-2, 0));
	TestEqual(TEXT("xy independent"), FStreamingGrid::WorldToTile(FVector2D(300.0, -300.0), Size), FIntPoint(2, -3));
	// Degenerate tile size returns the origin tile rather than dividing by zero.
	TestEqual(TEXT("zero size safe"), FStreamingGrid::WorldToTile(FVector2D(500.0, 500.0), 0.0), FIntPoint(0, 0));

	return true;
}

// --- TileMin / TileCenter -------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStreamingGridTileGeomTest,
	"GTC.World.Streaming.Grid.TileGeom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStreamingGridTileGeomTest::RunTest(const FString& Parameters)
{
	const double Size = 128.0;

	const FVector2D Min = FStreamingGrid::TileMin(FIntPoint(1, 0), Size);
	TestEqual(TEXT("min.x"), Min.X, 128.0, Eps);
	TestEqual(TEXT("min.y"), Min.Y, 0.0, Eps);

	const FVector2D C0 = FStreamingGrid::TileCenter(FIntPoint(0, 0), Size);
	TestEqual(TEXT("center0.x"), C0.X, 64.0, Eps);
	TestEqual(TEXT("center0.y"), C0.Y, 64.0, Eps);

	const FVector2D Cn = FStreamingGrid::TileCenter(FIntPoint(-1, 2), Size);
	TestEqual(TEXT("center(-1,2).x"), Cn.X, -64.0, Eps);
	TestEqual(TEXT("center(-1,2).y"), Cn.Y, 320.0, Eps);

	// WorldToTile round-trips: the center of a tile maps back to that tile.
	TestEqual(TEXT("center round-trips"),
		FStreamingGrid::WorldToTile(FStreamingGrid::TileCenter(FIntPoint(3, -5), Size), Size), FIntPoint(3, -5));

	return true;
}

// --- TileRing (Chebyshev) -------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStreamingGridTileRingTest,
	"GTC.World.Streaming.Grid.TileRing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStreamingGridTileRingTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("same tile ring 0"), FStreamingGrid::TileRing(FIntPoint(4, 4), FIntPoint(4, 4)), 0);
	TestEqual(TEXT("adjacent ring 1"), FStreamingGrid::TileRing(FIntPoint(0, 0), FIntPoint(1, 0)), 1);
	TestEqual(TEXT("diagonal still ring 1"), FStreamingGrid::TileRing(FIntPoint(0, 0), FIntPoint(1, 1)), 1);
	// Chebyshev = max of the two axis deltas, sign-independent.
	TestEqual(TEXT("max axis dominates"), FStreamingGrid::TileRing(FIntPoint(0, 0), FIntPoint(2, -3)), 3);

	return true;
}

// --- CenterDistance -------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStreamingGridCenterDistanceTest,
	"GTC.World.Streaming.Grid.CenterDistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStreamingGridCenterDistanceTest::RunTest(const FString& Parameters)
{
	const double Size = 128.0;

	// Standing on a tile's center -> distance 0.
	TestEqual(TEXT("on center is zero"),
		FStreamingGrid::CenterDistance(FIntPoint(0, 0), FVector2D(64.0, 64.0), Size), 0.0, Eps);

	// Center of tile (1,0) is (192,64); from (64,64) that is 128 along +X.
	TestEqual(TEXT("one tile east"),
		FStreamingGrid::CenterDistance(FIntPoint(1, 0), FVector2D(64.0, 64.0), Size), 128.0, Eps);

	// 3-4-5: center of (1,1) is (192,192); from (64,64) -> dx=128, dy=128.
	TestEqual(TEXT("diagonal magnitude"),
		FStreamingGrid::CenterDistance(FIntPoint(1, 1), FVector2D(64.0, 64.0), Size),
		FMath::Sqrt(128.0 * 128.0 + 128.0 * 128.0), Eps);

	return true;
}

// --- TilesInWindow --------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStreamingGridTilesInWindowTest,
	"GTC.World.Streaming.Grid.TilesInWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStreamingGridTilesInWindowTest::RunTest(const FString& Parameters)
{
	// Radius 0 -> just the center tile.
	{
		const TArray<FIntPoint> W = FStreamingGrid::TilesInWindow(FIntPoint(5, 5), 0);
		TestEqual(TEXT("r0 count"), W.Num(), 1);
		TestEqual(TEXT("r0 is center"), W[0], FIntPoint(5, 5));
	}

	// Radius 1 -> 3x3 = 9 tiles, center included, deterministic row-major order.
	{
		const TArray<FIntPoint> W = FStreamingGrid::TilesInWindow(FIntPoint(0, 0), 1);
		TestEqual(TEXT("r1 count"), W.Num(), 9);
		TestTrue(TEXT("r1 includes center"), W.Contains(FIntPoint(0, 0)));
		TestTrue(TEXT("r1 includes corner"), W.Contains(FIntPoint(-1, -1)));
		TestTrue(TEXT("r1 includes far corner"), W.Contains(FIntPoint(1, 1)));
		// Row-major: first entry is the lower-left corner, last is the upper-right.
		TestEqual(TEXT("r1 first is min corner"), W[0], FIntPoint(-1, -1));
		TestEqual(TEXT("r1 last is max corner"), W.Last(), FIntPoint(1, 1));
		// No duplicates.
		TestEqual(TEXT("r1 unique"), TSet<FIntPoint>(W).Num(), 9);
	}

	// Radius 2 -> 5x5 = 25 tiles.
	TestEqual(TEXT("r2 count"), FStreamingGrid::TilesInWindow(FIntPoint(0, 0), 2).Num(), 25);

	// Negative radius -> empty (degenerate guard).
	TestEqual(TEXT("neg radius empty"), FStreamingGrid::TilesInWindow(FIntPoint(0, 0), -1).Num(), 0);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
