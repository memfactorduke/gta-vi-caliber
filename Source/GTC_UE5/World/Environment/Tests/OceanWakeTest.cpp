// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../OceanWake.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FOceanWake — boat wake & foam parameter generation. Proves the wake
 * angle is the Kelvin angle at low speed and narrows monotonically (and
 * continuously) at high Froude, the wake strength / foam rate ramp from rest and
 * stay bounded, whitecap foam tracks the surface fold, and the trail fades.
 * Prefix GTC.World.Ocean.Wake.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOceanWakeAngleTest,
	"GTC.World.Ocean.Wake.Angle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOceanWakeAngleTest::RunTest(const FString& Parameters)
{
	const double Hull = 4.0;
	const double FrHalfSpeed = 0.5 * FMath::Sqrt(9.81 * Hull); // exactly Fr = 0.5

	// At/below the threshold the wake stays at the full Kelvin angle.
	TestTrue(TEXT("slow wake is Kelvin angle"),
		FMath::Abs(FOceanWake::WakeHalfAngleDeg(0.0, Hull) - FOceanWake::KelvinHalfAngleDeg) < Eps);
	TestTrue(TEXT("threshold is Kelvin angle (continuous)"),
		FMath::Abs(FOceanWake::WakeHalfAngleDeg(FrHalfSpeed, Hull) - FOceanWake::KelvinHalfAngleDeg) < 1e-3);

	// Above the threshold it narrows, and keeps narrowing with speed.
	const double A1 = FOceanWake::WakeHalfAngleDeg(2.0 * FrHalfSpeed, Hull);
	const double A2 = FOceanWake::WakeHalfAngleDeg(4.0 * FrHalfSpeed, Hull);
	TestTrue(TEXT("fast wake narrower than Kelvin"), A1 < FOceanWake::KelvinHalfAngleDeg - Eps);
	TestTrue(TEXT("faster wake even narrower"), A2 < A1 - Eps);
	TestTrue(TEXT("angle stays positive"), A2 > 0.0);

	// Froude number sanity.
	TestTrue(TEXT("froude value"),
		FMath::Abs(FOceanWake::FroudeNumber(FrHalfSpeed, Hull) - 0.5) < Eps);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOceanWakeStrengthTest,
	"GTC.World.Ocean.Wake.Strength",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOceanWakeStrengthTest::RunTest(const FString& Parameters)
{
	const double Planing = 10.0;

	TestTrue(TEXT("no wake at rest"), FOceanWake::WakeStrength01(0.0, Planing) < Eps);
	TestTrue(TEXT("full wake when planing"), FMath::Abs(FOceanWake::WakeStrength01(Planing, Planing) - 1.0) < Eps);

	// Monotone non-decreasing in [0,1] across the ramp.
	bool bMonotone = true, bBounded = true;
	double Prev = -1.0;
	for (int32 i = 0; i <= 40; ++i)
	{
		const double V = i * 0.5;
		const double S = FOceanWake::WakeStrength01(V, Planing);
		if (S < Prev - Eps) { bMonotone = false; }
		if (S < -Eps || S > 1.0 + Eps) { bBounded = false; }
		Prev = S;
	}
	TestTrue(TEXT("wake strength monotone"), bMonotone);
	TestTrue(TEXT("wake strength bounded [0,1]"), bBounded);

	// Foam rate: zero at rest, capped at MaxRate.
	TestTrue(TEXT("no foam at rest"), FOceanWake::BowFoamRate(0.0, Planing, 200.0) < Eps);
	bool bRateBounded = true;
	for (int32 i = 0; i <= 60; ++i)
	{
		const double R = FOceanWake::BowFoamRate(i * 0.5, Planing, 200.0);
		if (R < -Eps || R > 200.0 + Eps) { bRateBounded = false; }
	}
	TestTrue(TEXT("foam rate bounded by max"), bRateBounded);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOceanWakeFoamTest,
	"GTC.World.Ocean.Wake.Foam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOceanWakeFoamTest::RunTest(const FString& Parameters)
{
	// Whitecap foam tracks the surface fold: none on calm water (Jacobian 1),
	// full where it folds (Jacobian <= 0), monotone decreasing between.
	TestTrue(TEXT("no foam on calm sea"), FOceanWake::WhitecapFoam01(1.0) < Eps);
	TestTrue(TEXT("full foam on folded crest"), FMath::Abs(FOceanWake::WhitecapFoam01(0.0) - 1.0) < Eps);
	TestTrue(TEXT("over-folded clamps to full"), FMath::Abs(FOceanWake::WhitecapFoam01(-0.5) - 1.0) < Eps);

	bool bMonotone = true;
	double Prev = 2.0;
	for (int32 i = 0; i <= 30; ++i)
	{
		const double J = i / 30.0; // 0 -> 1
		const double F = FOceanWake::WhitecapFoam01(J, 0.6);
		if (F > Prev + Eps) { bMonotone = false; } // foam falls as Jacobian rises
		Prev = F;
	}
	TestTrue(TEXT("foam decreases as sea smooths"), bMonotone);

	// Trail fade.
	TestTrue(TEXT("fresh trail full"), FMath::Abs(FOceanWake::TrailFade01(0.0, 3.0) - 1.0) < Eps);
	TestTrue(TEXT("dead trail gone"), FOceanWake::TrailFade01(3.0, 3.0) < Eps);
	TestTrue(TEXT("half-aged trail half"), FMath::Abs(FOceanWake::TrailFade01(1.5, 3.0) - 0.5) < Eps);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
