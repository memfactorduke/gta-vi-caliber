// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../TileStreamPriority.h"
#include "../../../Tests/GtcTestTolerances.h"

/**
 * Unit tests for FTileStreamPriority (anticipatory tile load priority).
 * Prefix GTC.World.Streaming.Priority. FVector2D components are doubles, so
 * comparisons use double literals + the shared Eps tolerance.
 */

using GtcTest::Eps;

namespace
{
	void TestVec2Equal(FAutomationTestBase& T, const TCHAR* What, const FVector2D& A, const FVector2D& B, double Tol)
	{
		T.TestEqual(FString::Printf(TEXT("%s.x"), What), A.X, B.X, Tol);
		T.TestEqual(FString::Printf(TEXT("%s.y"), What), A.Y, B.Y, Tol);
	}
}

// --- LookAheadPoint -------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTileStreamPriorityLookAheadTest,
	"GTC.World.Streaming.Priority.LookAhead",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTileStreamPriorityLookAheadTest::RunTest(const FString& Parameters)
{
	// Stationary camera -> look-ahead point is the camera position.
	TestVec2Equal(*this, TEXT("stationary"),
		FTileStreamPriority::LookAheadPoint(FVector2D(5.0, 7.0), FVector2D::ZeroVector, 2.0), FVector2D(5.0, 7.0), Eps);

	// Non-positive look-ahead collapses to the camera position even when moving.
	TestVec2Equal(*this, TEXT("zero lookahead"),
		FTileStreamPriority::LookAheadPoint(FVector2D::ZeroVector, FVector2D(10.0, 0.0), 0.0), FVector2D::ZeroVector, Eps);

	// Moving +X at 10 u/s for 2 s -> 20 units ahead.
	TestVec2Equal(*this, TEXT("moving +x"),
		FTileStreamPriority::LookAheadPoint(FVector2D::ZeroVector, FVector2D(10.0, 0.0), 2.0), FVector2D(20.0, 0.0), Eps);

	// Diagonal velocity integrates per axis.
	TestVec2Equal(*this, TEXT("diagonal"),
		FTileStreamPriority::LookAheadPoint(FVector2D::ZeroVector, FVector2D(3.0, 4.0), 2.0), FVector2D(6.0, 8.0), Eps);

	return true;
}

// --- EffectiveDistance ----------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTileStreamPriorityEffectiveDistanceTest,
	"GTC.World.Streaming.Priority.EffectiveDistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTileStreamPriorityEffectiveDistanceTest::RunTest(const FString& Parameters)
{
	const FVector2D Cam(0.0, 0.0);
	const FVector2D Vel(10.0, 0.0); // heading +X
	const double Look = 1.0;        // look-ahead point lands at (10,0)

	// Stationary camera -> effective distance is plain radial distance.
	TestEqual(TEXT("stationary == radial"),
		FTileStreamPriority::EffectiveDistance(Cam, FVector2D::ZeroVector, FVector2D(10.0, 0.0), Look), 10.0, Eps);

	const double Ahead = FTileStreamPriority::EffectiveDistance(Cam, Vel, FVector2D(10.0, 0.0), Look);   // 0
	const double Behind = FTileStreamPriority::EffectiveDistance(Cam, Vel, FVector2D(-10.0, 0.0), Look); // 20
	const double Lateral = FTileStreamPriority::EffectiveDistance(Cam, Vel, FVector2D(0.0, 10.0), Look); // ~14.14
	const double Farther = FTileStreamPriority::EffectiveDistance(Cam, Vel, FVector2D(20.0, 0.0), Look); // 10

	TestEqual(TEXT("directly ahead at lookahead point is zero"), Ahead, 0.0, Eps);
	TestEqual(TEXT("behind is actual+lookahead"), Behind, 20.0, Eps);
	TestEqual(TEXT("lateral magnitude"), Lateral, FMath::Sqrt(200.0), Eps);

	// The whole point: a tile the camera heads toward is more urgent (smaller
	// effective distance) than an equidistant lateral tile or a tile behind it.
	TestTrue(TEXT("ahead more urgent than lateral"), Ahead < Lateral);
	TestTrue(TEXT("ahead more urgent than behind"), Ahead < Behind);
	TestTrue(TEXT("nearer-ahead more urgent than farther-ahead"), Ahead < Farther);

	return true;
}

// --- AlongTrack -----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTileStreamPriorityAlongTrackTest,
	"GTC.World.Streaming.Priority.AlongTrack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTileStreamPriorityAlongTrackTest::RunTest(const FString& Parameters)
{
	const FVector2D Cam(0.0, 0.0);
	const FVector2D Vel(10.0, 0.0); // heading +X (unit-normalized internally)

	TestEqual(TEXT("ahead positive"), FTileStreamPriority::AlongTrack(Cam, Vel, FVector2D(5.0, 3.0)), 5.0, Eps);
	TestEqual(TEXT("behind negative"), FTileStreamPriority::AlongTrack(Cam, Vel, FVector2D(-5.0, 3.0)), -5.0, Eps);
	TestEqual(TEXT("lateral zero"), FTileStreamPriority::AlongTrack(Cam, Vel, FVector2D(0.0, 7.0)), 0.0, Eps);
	// Stationary camera has no heading -> zero regardless of tile.
	TestEqual(TEXT("stationary zero"),
		FTileStreamPriority::AlongTrack(Cam, FVector2D::ZeroVector, FVector2D(5.0, 3.0)), 0.0, Eps);

	return true;
}

// --- IsMoreUrgent ---------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTileStreamPriorityIsMoreUrgentTest,
	"GTC.World.Streaming.Priority.IsMoreUrgent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTileStreamPriorityIsMoreUrgentTest::RunTest(const FString& Parameters)
{
	const FVector2D Cam(0.0, 0.0);
	const FVector2D Vel(10.0, 0.0);
	const double Look = 1.0;

	// Ahead beats behind.
	TestTrue(TEXT("ahead over behind"),
		FTileStreamPriority::IsMoreUrgent(Cam, Vel, FVector2D(10.0, 0.0), FVector2D(-10.0, 0.0), Look));
	TestFalse(TEXT("behind not over ahead"),
		FTileStreamPriority::IsMoreUrgent(Cam, Vel, FVector2D(-10.0, 0.0), FVector2D(10.0, 0.0), Look));

	// Nearer-ahead beats farther-ahead.
	TestTrue(TEXT("nearer over farther"),
		FTileStreamPriority::IsMoreUrgent(Cam, Vel, FVector2D(12.0, 0.0), FVector2D(20.0, 0.0), Look));

	// Strict-weak-ordering: the comparator must be irreflexive so a sort can use it.
	// A tile is never strictly more urgent than itself (equal effective distance).
	const FVector2D Tile(12.0, 0.0);
	TestFalse(TEXT("equal-distance tile is not more urgent than itself"),
		FTileStreamPriority::IsMoreUrgent(Cam, Vel, Tile, Tile, Look));
	// And two distinct tiles at the same effective distance tie both ways to false.
	TestFalse(TEXT("mirror tiles at equal distance do not outrank each other (A,B)"),
		FTileStreamPriority::IsMoreUrgent(Cam, Vel, FVector2D(10.0, 5.0), FVector2D(10.0, -5.0), Look));
	TestFalse(TEXT("mirror tiles at equal distance do not outrank each other (B,A)"),
		FTileStreamPriority::IsMoreUrgent(Cam, Vel, FVector2D(10.0, -5.0), FVector2D(10.0, 5.0), Look));

	return true;
}

#endif // WITH_AUTOMATION_TESTS
