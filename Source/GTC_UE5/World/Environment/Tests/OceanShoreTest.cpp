// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../OceanShore.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FOceanShore. Proves the shore blend fades in from the waterline, the
 * foam is a bounded band that vanishes on dry land and in deep water, the wet
 * sand follows the swash line, and the depth colour fade saturates with depth.
 * Prefix GTC.World.Ocean.Shore.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOceanShoreBlendTest,
	"GTC.World.Ocean.Shore.Blend",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOceanShoreBlendTest::RunTest(const FString& Parameters)
{
	// Depth is the instantaneous surface minus the seabed.
	TestTrue(TEXT("depth subtracts seabed"), FMath::Abs(FOceanShore::WaterDepth(0.3, -1.0) - 1.3) < Eps);
	TestTrue(TEXT("dry land negative depth"), FOceanShore::WaterDepth(-0.2, 0.5) < 0.0);

	// Shore blend: 0 at/landward of the waterline, 1 once deep, monotone between.
	TestTrue(TEXT("blend dry is 0"), FOceanShore::ShoreBlend01(-0.5, 2.0) < Eps);
	TestTrue(TEXT("blend at waterline 0"), FOceanShore::ShoreBlend01(0.0, 2.0) < Eps);
	TestTrue(TEXT("blend deep is 1"), FMath::Abs(FOceanShore::ShoreBlend01(2.0, 2.0) - 1.0) < Eps);

	bool bMonotone = true, bBounded = true;
	double Prev = -1.0;
	for (int32 i = 0; i <= 40; ++i)
	{
		const double D = i * 0.1; // 0 -> 4
		const double B = FOceanShore::ShoreBlend01(D, 2.0);
		if (B < Prev - Eps) { bMonotone = false; }
		if (B < -Eps || B > 1.0 + Eps) { bBounded = false; }
		Prev = B;
	}
	TestTrue(TEXT("blend monotone"), bMonotone);
	TestTrue(TEXT("blend bounded"), bBounded);

	// Depth colour fade: 0 at shore, saturating toward 1 in deep water.
	TestTrue(TEXT("color fade shore 0"), FOceanShore::DepthColorFade01(0.0, 3.0) < Eps);
	TestTrue(TEXT("color fade deep near 1"), FOceanShore::DepthColorFade01(30.0, 3.0) > 0.99);
	TestTrue(TEXT("color fade monotone"),
		FOceanShore::DepthColorFade01(5.0, 3.0) > FOceanShore::DepthColorFade01(1.0, 3.0));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOceanShoreFoamTest,
	"GTC.World.Ocean.Shore.Foam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOceanShoreFoamTest::RunTest(const FString& Parameters)
{
	const double FoamDepth = 1.0;

	// Foam is a band: nothing on dry land, nothing in deep water, something inside.
	TestTrue(TEXT("foam dry 0"), FOceanShore::ShoreFoam01(-0.3, FoamDepth) < Eps);
	TestTrue(TEXT("foam at waterline 0"), FOceanShore::ShoreFoam01(0.0, FoamDepth) < Eps);
	TestTrue(TEXT("foam deep 0"), FOceanShore::ShoreFoam01(2.0, FoamDepth) < Eps);

	double Peak = 0.0;
	bool bBounded = true;
	for (int32 i = 0; i <= 40; ++i)
	{
		const double D = i * 0.05; // 0 -> 2
		const double F = FOceanShore::ShoreFoam01(D, FoamDepth);
		Peak = FMath::Max(Peak, F);
		if (F < -Eps || F > 1.0 + Eps) { bBounded = false; }
	}
	TestTrue(TEXT("foam has a peak in the break zone"), Peak > 0.5);
	TestTrue(TEXT("foam bounded [0,1]"), bBounded);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOceanShoreWetSandTest,
	"GTC.World.Ocean.Shore.WetSand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOceanShoreWetSandTest::RunTest(const FString& Parameters)
{
	const double SwashTop = 0.5;   // top of the wave run-up (mean sea + run-up)
	const double DryBand = 0.4;

	// At/below the swash line the sand is wet; well above it is dry; monotone fall.
	TestTrue(TEXT("below swash fully wet"), FMath::Abs(FOceanShore::WetSand01(0.0, SwashTop, DryBand) - 1.0) < Eps);
	TestTrue(TEXT("at swash top still wet"), FMath::Abs(FOceanShore::WetSand01(SwashTop, SwashTop, DryBand) - 1.0) < Eps);
	TestTrue(TEXT("above band dry"), FOceanShore::WetSand01(SwashTop + DryBand, SwashTop, DryBand) < Eps);

	bool bMonotone = true;
	double Prev = 2.0;
	for (int32 i = 0; i <= 40; ++i)
	{
		const double Z = i * 0.05; // rising up the beach
		const double W = FOceanShore::WetSand01(Z, SwashTop, DryBand);
		if (W > Prev + Eps) { bMonotone = false; } // wetness only falls as we climb
		Prev = W;
	}
	TestTrue(TEXT("wetness falls up the beach"), bMonotone);

	// Swash chasing the wave: a higher run-up wets a higher point.
	const double LowRunup = FOceanShore::WetSand01(0.7, 0.5, DryBand);
	const double HighRunup = FOceanShore::WetSand01(0.7, 0.9, DryBand);
	TestTrue(TEXT("bigger run-up wets higher sand"), HighRunup > LowRunup + Eps);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
