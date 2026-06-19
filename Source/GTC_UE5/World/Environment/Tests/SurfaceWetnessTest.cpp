// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../SurfaceWetness.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FSurfaceWetness. Proves wetness rises toward the rain target and
 * decays toward dry, that it lags (no single-step snap), that wetting is faster
 * than drying, and that it converges to the rain intensity. Prefix
 * GTC.World.Wetness.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSurfaceWetnessRiseDecayTest,
	"GTC.World.Wetness.RiseDecay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSurfaceWetnessRiseDecayTest::RunTest(const FString& Parameters)
{
	const double Dt = 0.1;

	// Lag: one step of full rain from dry does not jump to soaked.
	const double OneStep = FSurfaceWetness::Step(0.0, 1.0, Dt);
	TestTrue(TEXT("wetness lags rain (no snap)"), OneStep > 0.0 && OneStep < 1.0);

	// Rain on a dry street: wetness rises monotonically toward 1.
	double W = 0.0;
	bool bRising = true;
	double Prev = -1.0;
	for (int32 i = 0; i < 200; ++i)
	{
		W = FSurfaceWetness::Step(W, 1.0, Dt);
		if (W < Prev - Eps) { bRising = false; }
		Prev = W;
	}
	TestTrue(TEXT("rises under rain"), bRising);
	TestTrue(TEXT("approaches soaked"), W > 0.95);

	// Rain stops on a soaked street: wetness decays monotonically toward dry.
	bool bFalling = true;
	Prev = 2.0;
	for (int32 i = 0; i < 400; ++i)
	{
		W = FSurfaceWetness::Step(W, 0.0, Dt);
		if (W > Prev + Eps) { bFalling = false; }
		Prev = W;
	}
	TestTrue(TEXT("dries when rain stops"), bFalling);
	TestTrue(TEXT("approaches dry"), W < 0.05);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSurfaceWetnessAsymmetryTest,
	"GTC.World.Wetness.Asymmetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSurfaceWetnessAsymmetryTest::RunTest(const FString& Parameters)
{
	const double Dt = 0.1;

	// Steps to wet from 0 up past 0.5 ...
	int32 WetSteps = 0;
	double W = 0.0;
	while (W < 0.5 && WetSteps < 100000) { W = FSurfaceWetness::Step(W, 1.0, Dt); ++WetSteps; }

	// ... versus steps to dry from 1 down past 0.5.
	int32 DrySteps = 0;
	W = 1.0;
	while (W > 0.5 && DrySteps < 100000) { W = FSurfaceWetness::Step(W, 0.0, Dt); ++DrySteps; }

	TestTrue(TEXT("drying is slower than wetting"), DrySteps > WetSteps);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSurfaceWetnessConvergeTest,
	"GTC.World.Wetness.Converge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSurfaceWetnessConvergeTest::RunTest(const FString& Parameters)
{
	const double Dt = 0.1;

	// Sustained drizzle (rain 0.5) settles at ~0.5, not soaked.
	TestTrue(TEXT("equilibrium tracks rain"), FMath::Abs(FSurfaceWetness::Equilibrium(0.5) - 0.5) < Eps);

	double W = 0.0;
	for (int32 i = 0; i < 2000; ++i) { W = FSurfaceWetness::Step(W, 0.5, Dt); }
	TestTrue(TEXT("converges to drizzle level"), FMath::Abs(W - 0.5) < 1e-2);

	// Bounded across a noisy rain signal (alternating heavy/none).
	bool bBounded = true;
	W = 0.3;
	for (int32 i = 0; i < 500; ++i)
	{
		const double Rain = (i % 2 == 0) ? 1.0 : 0.0;
		W = FSurfaceWetness::Step(W, Rain, Dt);
		if (W < -Eps || W > 1.0 + Eps) { bBounded = false; }
	}
	TestTrue(TEXT("stays in [0,1]"), bBounded);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
