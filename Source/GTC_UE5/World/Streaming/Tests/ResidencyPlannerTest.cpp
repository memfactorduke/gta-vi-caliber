// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../ResidencyPlanner.h"

/**
 * Unit tests for FResidencyPlanner (the per-frame load/unload plan that composes
 * grid + hysteresis + priority + budget). Prefix GTC.World.Streaming.Planner.
 *
 * Camera sits at the center of tile (0,0) (= (50,50) at TileSize 100) so tile
 * centers fall at clean offsets: tile (i,j) center is (i*100+50, j*100+50),
 * radial distance from the camera = 100*sqrt(i^2+j^2).
 */

namespace
{
	FResidencyPlanInput BaseInput()
	{
		FResidencyPlanInput In;
		In.CameraPos = FVector2D(50.0, 50.0); // center of tile (0,0)
		In.Velocity = FVector2D::ZeroVector;
		In.TileSize = 100.0;
		In.WindowRadius = 2;     // 5x5 candidate window
		In.LoadRadius = 150.0;   // loads tiles with i^2+j^2 <= 2.25
		In.UnloadRadius = 250.0;
		In.LookAheadSeconds = 0.0;
		In.MaxLoadsPerTick = 0;  // unbounded
		return In;
	}
}

// --- basic load set from a cold start -------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResidencyPlannerColdStartTest,
	"GTC.World.Streaming.Planner.ColdStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResidencyPlannerColdStartTest::RunTest(const FString& Parameters)
{
	const FResidencyPlanInput In = BaseInput();
	const FStreamPlan Plan = FResidencyPlanner::Plan(In, TArray<FIntPoint>());

	// Within LoadRadius 150: i^2+j^2 <= 2.25 -> the center + 4 edge + 4 diagonal
	// = 9 tiles. Nothing resident yet, so nothing to unload.
	TestEqual(TEXT("loads 9 tiles"), Plan.ToLoad.Num(), 9);
	TestEqual(TEXT("nothing to unload"), Plan.ToUnload.Num(), 0);

	// Most urgent first = the tile under the camera.
	TestEqual(TEXT("most urgent is center"), Plan.ToLoad[0], FIntPoint(0, 0));

	// A tile just outside the load radius (2,0) at distance 200 is NOT loaded.
	TestFalse(TEXT("banded tile not loaded"), Plan.ToLoad.Contains(FIntPoint(2, 0)));

	return true;
}

// --- per-frame budget cap -------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResidencyPlannerBudgetTest,
	"GTC.World.Streaming.Planner.Budget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResidencyPlannerBudgetTest::RunTest(const FString& Parameters)
{
	FResidencyPlanInput In = BaseInput();
	In.MaxLoadsPerTick = 3;

	const FStreamPlan Plan = FResidencyPlanner::Plan(In, TArray<FIntPoint>());
	TestEqual(TEXT("capped to 3"), Plan.ToLoad.Num(), 3);
	TestEqual(TEXT("still most urgent first"), Plan.ToLoad[0], FIntPoint(0, 0));

	return true;
}

// --- unload resident tiles outside the window -----------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResidencyPlannerUnloadOutsideTest,
	"GTC.World.Streaming.Planner.UnloadOutside",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResidencyPlannerUnloadOutsideTest::RunTest(const FString& Parameters)
{
	const FResidencyPlanInput In = BaseInput();
	// (0,0) is in-window and wanted; (10,10) is far outside the window.
	const TArray<FIntPoint> Resident = {FIntPoint(0, 0), FIntPoint(10, 10)};

	const FStreamPlan Plan = FResidencyPlanner::Plan(In, Resident);

	TestTrue(TEXT("far tile unloaded"), Plan.ToUnload.Contains(FIntPoint(10, 10)));
	TestFalse(TEXT("kept tile not unloaded"), Plan.ToUnload.Contains(FIntPoint(0, 0)));
	// (0,0) is already resident, so it is not re-loaded.
	TestFalse(TEXT("resident not reloaded"), Plan.ToLoad.Contains(FIntPoint(0, 0)));

	return true;
}

// --- hysteresis keeps a banded resident tile ------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResidencyPlannerBandKeepTest,
	"GTC.World.Streaming.Planner.BandKeep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResidencyPlannerBandKeepTest::RunTest(const FString& Parameters)
{
	const FResidencyPlanInput In = BaseInput();
	// (2,0) is at distance 200 -> inside the hysteresis band (150..250). Already
	// resident, so it must be kept: neither loaded nor unloaded.
	const TArray<FIntPoint> Resident = {FIntPoint(2, 0)};

	const FStreamPlan Plan = FResidencyPlanner::Plan(In, Resident);

	TestFalse(TEXT("banded resident not unloaded"), Plan.ToUnload.Contains(FIntPoint(2, 0)));
	TestFalse(TEXT("banded resident not loaded"), Plan.ToLoad.Contains(FIntPoint(2, 0)));

	// A resident tile pushed beyond the unload radius IS released. (3,0) is in a
	// wider window here, so widen and verify the eviction edge.
	FResidencyPlanInput Wide = BaseInput();
	Wide.WindowRadius = 3;
	const TArray<FIntPoint> FarResident = {FIntPoint(3, 0)}; // distance 300 >= 250
	const FStreamPlan WidePlan = FResidencyPlanner::Plan(Wide, FarResident);
	TestTrue(TEXT("beyond unload radius released"), WidePlan.ToUnload.Contains(FIntPoint(3, 0)));

	return true;
}

// --- velocity biases the load order ---------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResidencyPlannerVelocityOrderTest,
	"GTC.World.Streaming.Planner.VelocityOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResidencyPlannerVelocityOrderTest::RunTest(const FString& Parameters)
{
	FResidencyPlanInput In = BaseInput();
	In.Velocity = FVector2D(1000.0, 0.0); // racing +X
	In.LookAheadSeconds = 0.2;            // look-ahead point at (250,50)
	In.MaxLoadsPerTick = 1;               // only the single most urgent

	const FStreamPlan Plan = FResidencyPlanner::Plan(In, TArray<FIntPoint>());

	// With the camera heading +X, the tile ahead (1,0) is more urgent than the
	// tile under the camera (0,0) — anticipatory streaming.
	TestEqual(TEXT("loads tile ahead, not under camera"), Plan.ToLoad.Num(), 1);
	TestEqual(TEXT("ahead tile first"), Plan.ToLoad[0], FIntPoint(1, 0));

	return true;
}

#endif // WITH_AUTOMATION_TESTS
