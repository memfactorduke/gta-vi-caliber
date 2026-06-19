// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../OceanBuoyancy.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FOceanBuoyancy. Proves a dry body floats free of force, a fully
 * submerged body's force equals the displaced weight (and caps there), a level
 * raft pushes straight up with no torque, a tilted raft produces a *righting*
 * torque, and drag always opposes motion scaled by wetted fraction. Prefix
 * GTC.World.Ocean.Buoyancy.
 */

namespace
{
	// A square raft: four hull samples at (+/-1, +/-1) with the given Z and span.
	void MakeRaft(FBuoyancyPoint Out[4], double Zpp, double Zpm, double Zmp, double Zmm,
		double WaterH, double Span = 0.5, double Vol = 1.0)
	{
		const double Xs[4] = { 1.0, 1.0, -1.0, -1.0 };
		const double Ys[4] = { 1.0, -1.0, 1.0, -1.0 };
		const double Zs[4] = { Zpp, Zpm, Zmp, Zmm };
		for (int32 i = 0; i < 4; ++i)
		{
			Out[i] = FBuoyancyPoint{};
			Out[i].X = Xs[i]; Out[i].Y = Ys[i]; Out[i].Z = Zs[i];
			Out[i].WaterHeight = WaterH; Out[i].Span = Span; Out[i].Volume = Vol;
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBuoyancyExtremesTest,
	"GTC.World.Ocean.Buoyancy.Extremes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuoyancyExtremesTest::RunTest(const FString& Parameters)
{
	const double RhoG = FOceanBuoyancy::SeaWaterDensity * FOceanBuoyancy::DefaultGravity;

	// Dry: raft well above the water -> no force, no torque, nothing submerged.
	FBuoyancyPoint Dry[4];
	MakeRaft(Dry, 0.5, 0.5, 0.5, 0.5, 0.0);
	const FBuoyancyResult D = FOceanBuoyancy::Compute(Dry, 4, 0, 0, 0);
	TestTrue(TEXT("dry no force"), FMath::Abs(D.Force.Z) < Eps);
	TestTrue(TEXT("dry not submerged"), D.SubmergedFraction < Eps);

	// Fully submerged: each unit-volume sample displaces its whole volume.
	FBuoyancyPoint Deep[4];
	MakeRaft(Deep, -2.0, -2.0, -2.0, -2.0, 0.0);
	const FBuoyancyResult S = FOceanBuoyancy::Compute(Deep, 4, 0, 0, 0);
	TestTrue(TEXT("submerged fraction 1"), FMath::Abs(S.SubmergedFraction - 1.0) < Eps);
	TestTrue(TEXT("submerged force = displaced weight"), FMath::Abs(S.Force.Z - RhoG * 4.0) < 0.1);
	TestTrue(TEXT("submerged symmetric no torque"),
		FMath::Abs(S.Torque.X) + FMath::Abs(S.Torque.Y) < 1e-6);

	// Empty input is safe.
	const FBuoyancyResult E = FOceanBuoyancy::Compute(nullptr, 0, 0, 0, 0);
	TestTrue(TEXT("empty zero"), FMath::Abs(E.Force.Z) + E.SubmergedFraction < Eps);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBuoyancyRightingTest,
	"GTC.World.Ocean.Buoyancy.Righting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuoyancyRightingTest::RunTest(const FString& Parameters)
{
	// Level half-submerged raft (water 0.25 above the deck, span 0.5): pure up.
	FBuoyancyPoint Level[4];
	MakeRaft(Level, 0.0, 0.0, 0.0, 0.0, 0.25);
	const FBuoyancyResult L = FOceanBuoyancy::Compute(Level, 4, 0, 0, 0);
	TestTrue(TEXT("level pushes up"), L.Force.Z > 0.0);
	TestTrue(TEXT("level half submerged"), FMath::Abs(L.SubmergedFraction - 0.5) < Eps);
	TestTrue(TEXT("level no righting torque"),
		FMath::Abs(L.Torque.X) + FMath::Abs(L.Torque.Y) < 1e-6);

	// Tilt the +X side down (deeper). Only the +X samples submerge, so the torque
	// about Y must be negative — the restoring direction that lifts +X back up.
	FBuoyancyPoint Tilt[4];
	MakeRaft(Tilt, -0.3, -0.3, 0.3, 0.3, 0.0); // +X pair (idx0,1) low, -X pair (idx2,3) high
	const FBuoyancyResult Tq = FOceanBuoyancy::Compute(Tilt, 4, 0, 0, 0);
	TestTrue(TEXT("tilt has up force"), Tq.Force.Z > 0.0);
	TestTrue(TEXT("tilt righting torque about Y is restoring"), Tq.Torque.Y < -Eps);
	TestTrue(TEXT("tilt symmetric in Y -> no X torque"), FMath::Abs(Tq.Torque.X) < 1e-6);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBuoyancyDepthCurveTest,
	"GTC.World.Ocean.Buoyancy.DepthCurve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuoyancyDepthCurveTest::RunTest(const FString& Parameters)
{
	const double RhoG = FOceanBuoyancy::SeaWaterDensity * FOceanBuoyancy::DefaultGravity;

	// Submersion ramp: 0 above, 0.5 at half span, 1 at/below full span, clamped.
	FBuoyancyPoint P{}; P.X = 0; P.Y = 0; P.Volume = 1.0; P.Span = 0.5; P.WaterHeight = 0.0;
	P.Z = 0.1;   TestTrue(TEXT("above water dry"), FOceanBuoyancy::Submersion01(P) < Eps);
	P.Z = -0.25; TestTrue(TEXT("half span -> 0.5"), FMath::Abs(FOceanBuoyancy::Submersion01(P) - 0.5) < Eps);
	P.Z = -0.5;  TestTrue(TEXT("full span -> 1"), FMath::Abs(FOceanBuoyancy::Submersion01(P) - 1.0) < Eps);
	P.Z = -5.0;  TestTrue(TEXT("deep clamps at 1"), FMath::Abs(FOceanBuoyancy::Submersion01(P) - 1.0) < Eps);

	// Force is monotone non-decreasing as the sample sinks, and caps at rho*g*V.
	bool bMonotone = true;
	double Prev = -1.0;
	double Last = 0.0;
	for (int32 i = 0; i <= 40; ++i)
	{
		FBuoyancyPoint Q{}; Q.Volume = 1.0; Q.Span = 0.5; Q.WaterHeight = 0.0;
		Q.Z = 0.2 - i * 0.05; // sweep from above water downward
		const FBuoyancyResult Rr = FOceanBuoyancy::Compute(&Q, 1, 0, 0, 0);
		if (Rr.Force.Z < Prev - Eps) { bMonotone = false; }
		Prev = Rr.Force.Z;
		Last = Rr.Force.Z;
	}
	TestTrue(TEXT("force rises monotonically with depth"), bMonotone);
	TestTrue(TEXT("force caps at displaced weight"), FMath::Abs(Last - RhoG * 1.0) < 0.1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBuoyancyDragTest,
	"GTC.World.Ocean.Buoyancy.Drag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuoyancyDragTest::RunTest(const FString& Parameters)
{
	// Drag opposes velocity and scales with wetted fraction.
	FBuoyVec3 V; V.X = 3.0; V.Y = -2.0; V.Z = 1.0;
	const FBuoyVec3 Half = FOceanBuoyancy::LinearDrag(V, 0.5, 4.0);
	TestTrue(TEXT("drag opposes X"), Half.X < 0.0);
	TestTrue(TEXT("drag opposes Y"), Half.Y > 0.0);
	TestTrue(TEXT("drag opposes Z"), Half.Z < 0.0);
	TestTrue(TEXT("drag magnitude X = coeff*frac*v"), FMath::Abs(Half.X - (-4.0 * 0.5 * 3.0)) < Eps);

	// Dry hull (fraction 0) -> no drag.
	const FBuoyVec3 None = FOceanBuoyancy::LinearDrag(V, 0.0, 4.0);
	TestTrue(TEXT("dry no drag"), FMath::Abs(None.X) + FMath::Abs(None.Y) + FMath::Abs(None.Z) < Eps);

	// Angular drag opposes spin too.
	FBuoyVec3 W; W.X = 0.0; W.Y = 0.0; W.Z = 2.0;
	const FBuoyVec3 Ad = FOceanBuoyancy::AngularDrag(W, 1.0, 0.7);
	TestTrue(TEXT("angular drag opposes spin"), Ad.Z < 0.0);
	TestTrue(TEXT("angular drag magnitude"), FMath::Abs(Ad.Z - (-0.7 * 2.0)) < Eps);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
