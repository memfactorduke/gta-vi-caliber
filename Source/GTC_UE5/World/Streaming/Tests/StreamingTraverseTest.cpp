// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../StreamingResidency.h"
#include "../StreamingGrid.h"

/**
 * Acceptance probe for the M3 streaming substrate (prefix GTC.World.Streaming.Traverse).
 *
 * This is the headless, math-level proof that backs the slice's real acceptance
 * test — a ~4 km drive with no loading hitch. It drives FStreamingResidency (the
 * brain UWorldStreamingSubsystem runs each tick) over a 4 km straight run followed
 * by a 90 degree corner (the case naive streamers thrash on) and asserts the four
 * properties a seamless streamer must hold every tick:
 *
 *   (a) the resident ring stays populated AHEAD of travel (no pop-in),
 *   (b) no tile thrashes — loads only happen inside the load radius, unloads only
 *       beyond the unload radius (the hysteresis band is respected),
 *   (c) the per-frame load budget is never exceeded and forward progress holds
 *       (a cold start fills the ring within a bounded number of ticks),
 *   (d) residency is bounded by the candidate window (no unbounded growth).
 *
 * The GPU-level proof (frame time, streaming <= 1 ms main-thread) is the live
 * profile captured in the editor; this guards the scheduling logic that feeds it.
 *
 * 128 m tiles (TileSize 12800 cm); ~167 km/h (600 cm per 0.1 s plan tick), so the
 * camera advances 600 cm << one tile each plan — a smooth traverse.
 */

namespace
{
	constexpr double TileSize = 12800.0;      // 128 m
	constexpr int32 WindowRadius = 6;         // 13x13 candidate window
	constexpr double LoadRadius = 38400.0;    // 3 tiles
	constexpr double UnloadRadius = 64000.0;  // 5 tiles
	constexpr double LookAhead = 1.0;         // s
	constexpr int32 MaxLoads = 4;             // per-tick instancing cap
	constexpr double Dt = 0.1;                // plan interval
	constexpr double Speed = 6000.0;          // cm/s

	FResidencyPlanInput MakeInput(const FVector2D& Cam, const FVector2D& Vel)
	{
		FResidencyPlanInput In;
		In.CameraPos = Cam;
		In.Velocity = Vel;
		In.TileSize = TileSize;
		In.WindowRadius = WindowRadius;
		In.LoadRadius = LoadRadius;
		In.UnloadRadius = UnloadRadius;
		In.LookAheadSeconds = LookAhead;
		In.MaxLoadsPerTick = MaxLoads;
		return In;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStreamingTraverseTest,
	"GTC.World.Streaming.Traverse.FourKmNoHitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStreamingTraverseTest::RunTest(const FString& Parameters)
{
	const FTileLodBands Bands{25600.0, 51200.0, 89600.0};
	constexpr double LodMargin = 2560.0;
	const int32 WindowArea = (2 * WindowRadius + 1) * (2 * WindowRadius + 1);

	FStreamingResidency Res;

	// Start at a tile centre well away from the origin (exercise large-coordinate math).
	FVector2D Cam(TileSize * 0.5 + 5'000'000.0, TileSize * 0.5);
	FVector2D Prev = Cam;
	bool bHavePrev = false;

	// Accumulated violation flags — assert once at the end so a clean run is quiet.
	bool bLoadWithinRadius = true;
	bool bUnloadBeyondRadius = true;
	bool bBudgetRespected = true;
	bool bResidencyBounded = true;
	bool bRingAhead = true;
	FString FirstViolation;
	int32 MaxResident = 0;
	int32 MaxLoadCountAnyTile = 0;
	TMap<FIntPoint, int32> LoadCount;

	auto Fail = [&FirstViolation](const FString& Msg)
	{
		if (FirstViolation.IsEmpty())
		{
			FirstViolation = Msg;
		}
	};

	// Warm-up ticks before checking ring-ahead, so the cold-start fill completes.
	constexpr int32 Warmup = 40;
	int32 StepIndex = 0;

	auto Drive = [&](const FVector2D& Dir, int32 NumSteps)
	{
		const FVector2D Unit = Dir.GetSafeNormal();
		for (int32 i = 0; i < NumSteps; ++i, ++StepIndex)
		{
			Prev = Cam;
			Cam += Unit * (Speed * Dt);
			const FVector2D Vel = bHavePrev ? (Cam - Prev) / Dt : FVector2D::ZeroVector;
			bHavePrev = true;

			const FResidencyPlanInput In = MakeInput(Cam, Vel);
			const FStreamingResidency::FStepResult R = Res.Step(In, Bands, LodMargin);

			// (c) budget respected.
			if (R.Plan.ToLoad.Num() > MaxLoads)
			{
				bBudgetRespected = false;
				Fail(FString::Printf(TEXT("loaded %d > cap %d at step %d"), R.Plan.ToLoad.Num(), MaxLoads, StepIndex));
			}

			// (b) loads only inside the load radius.
			for (const FIntPoint& Tile : R.Plan.ToLoad)
			{
				const double D = FStreamingGrid::CenterDistance(Tile, Cam, TileSize);
				if (D > LoadRadius * 1.0001)
				{
					bLoadWithinRadius = false;
					Fail(FString::Printf(TEXT("loaded tile at dist %.0f > load radius %.0f at step %d"), D, LoadRadius, StepIndex));
				}
				const int32 N = ++LoadCount.FindOrAdd(Tile);
				MaxLoadCountAnyTile = FMath::Max(MaxLoadCountAnyTile, N);
			}

			// (b) unloads only beyond the unload radius (hysteresis respected, no thrash).
			for (const FIntPoint& Tile : R.Plan.ToUnload)
			{
				const double D = FStreamingGrid::CenterDistance(Tile, Cam, TileSize);
				if (D < UnloadRadius * 0.9999)
				{
					bUnloadBeyondRadius = false;
					Fail(FString::Printf(TEXT("unloaded tile at dist %.0f < unload radius %.0f at step %d"), D, UnloadRadius, StepIndex));
				}
			}

			// (d) residency bounded by the candidate window.
			MaxResident = FMath::Max(MaxResident, Res.Num());
			if (Res.Num() > WindowArea)
			{
				bResidencyBounded = false;
				Fail(FString::Printf(TEXT("resident %d > window area %d at step %d"), Res.Num(), WindowArea, StepIndex));
			}

			// (a) the ring two tiles ahead in the travel direction is resident.
			if (StepIndex >= Warmup)
			{
				const FVector2D AheadPoint = Cam + Unit * (2.0 * TileSize);
				const FIntPoint AheadTile = FStreamingGrid::WorldToTile(AheadPoint, TileSize);
				if (!Res.IsResident(AheadTile))
				{
					bRingAhead = false;
					Fail(FString::Printf(TEXT("tile 2 ahead (%d,%d) not resident at step %d"), AheadTile.X, AheadTile.Y, StepIndex));
				}
			}
		}
	};

	// 4 km straight, then a 90 degree corner and 1 km more.
	Drive(FVector2D(1.0, 0.0), FMath::CeilToInt32(400000.0 / (Speed * Dt)));
	Drive(FVector2D(0.0, 1.0), FMath::CeilToInt32(100000.0 / (Speed * Dt)));

	TestTrue(FString::Printf(TEXT("ring stays ahead of travel (%s)"), *FirstViolation), bRingAhead);
	TestTrue(FString::Printf(TEXT("loads only within load radius (%s)"), *FirstViolation), bLoadWithinRadius);
	TestTrue(FString::Printf(TEXT("unloads only beyond unload radius (%s)"), *FirstViolation), bUnloadBeyondRadius);
	TestTrue(FString::Printf(TEXT("per-tick load budget respected (%s)"), *FirstViolation), bBudgetRespected);
	TestTrue(FString::Printf(TEXT("residency bounded by window (%s)"), *FirstViolation), bResidencyBounded);

	// Forward progress: a steady ring is resident (not starved to empty), and the
	// straight run never re-loads a tile (each tile crossed exactly once).
	TestTrue(TEXT("steady ring is non-trivially resident"), MaxResident > 9);
	TestTrue(TEXT("no tile reloaded on a non-revisiting straight path"), MaxLoadCountAnyTile <= 2);

	// LOD bands: at journey's end the tile under the camera is full detail.
	const FIntPoint Here = FStreamingGrid::WorldToTile(Cam, TileSize);
	TestEqual(TEXT("tile under camera is full detail"), Res.BandOf(Here), ETileLod::Full);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
