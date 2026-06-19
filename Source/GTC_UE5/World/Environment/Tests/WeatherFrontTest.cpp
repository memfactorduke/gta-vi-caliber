// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../WeatherFront.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FWeatherFront. Proves the front line advances with time, a point goes
 * from clear (ahead) to weathered (behind) as the front sweeps past it, the
 * intensity is a smooth monotone ramp, and the world globals build cloud->rain in
 * order while a storm darkens even midday. Prefix GTC.World.WeatherFront.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeatherFrontSweepTest,
	"GTC.World.WeatherFront.Sweep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeatherFrontSweepTest::RunTest(const FString& Parameters)
{
	const double Start = 0.0;
	const double Speed = 10.0; // m/s eastward

	// The front line advances with time.
	TestTrue(TEXT("front advances"),
		FMath::Abs(FWeatherFront::FrontLineAt(Start, Speed, 5.0) - 50.0) < Eps);

	// A fixed point at Along=100: ahead of the front early, behind it later.
	TestTrue(TEXT("ahead is positive"), FWeatherFront::SignedDistance(100.0, Start, Speed, 0.0) > 0.0);
	TestTrue(TEXT("behind is negative"), FWeatherFront::SignedDistance(100.0, Start, Speed, 20.0) < 0.0);

	// As the front sweeps over the point, intensity rises monotonically 0 -> 1.
	const double Width = 30.0;
	bool bMonotone = true;
	double Prev = -1.0;
	double First = 0.0, Last = 0.0;
	for (int32 i = 0; i <= 40; ++i)
	{
		const double T = i * 0.5; // front line moves 0 -> 200
		const double S = FWeatherFront::SignedDistance(100.0, Start, Speed, T);
		const double I = FWeatherFront::Intensity01(S, Width);
		if (I < Prev - Eps) { bMonotone = false; }
		if (i == 0) { First = I; }
		Last = I;
		Prev = I;
	}
	TestTrue(TEXT("intensity rises as front arrives"), bMonotone);
	TestTrue(TEXT("starts clear"), First < 0.05);
	TestTrue(TEXT("ends weathered"), Last > 0.95);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeatherFrontShapeTest,
	"GTC.World.WeatherFront.Shape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeatherFrontShapeTest::RunTest(const FString& Parameters)
{
	const double Width = 20.0;

	// Intensity: 0 far ahead, 1 far behind, 0.5 on the line, monotone in distance.
	TestTrue(TEXT("far ahead clear"), FWeatherFront::Intensity01(100.0, Width) < Eps);
	TestTrue(TEXT("far behind full"), FMath::Abs(FWeatherFront::Intensity01(-100.0, Width) - 1.0) < Eps);
	TestTrue(TEXT("on the line is half"), FMath::Abs(FWeatherFront::Intensity01(0.0, Width) - 0.5) < Eps);

	bool bMonotone = true;
	double Prev = 2.0;
	for (int32 i = 0; i <= 40; ++i)
	{
		const double S = -40.0 + i * 2.0; // -40 -> +40
		const double I = FWeatherFront::Intensity01(S, Width);
		if (I > Prev + Eps) { bMonotone = false; } // intensity falls as we move ahead
		Prev = I;
	}
	TestTrue(TEXT("intensity monotone in distance"), bMonotone);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeatherFrontGlobalsTest,
	"GTC.World.WeatherFront.Globals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeatherFrontGlobalsTest::RunTest(const FString& Parameters)
{
	// Clear at noon: no cloud, no rain, dry, fully day (night-amount 0).
	const FWorldGlobals Clear = FWeatherFront::Globals(0.0, 1.0);
	TestTrue(TEXT("clear no cloud"), Clear.CloudCoverage < Eps);
	TestTrue(TEXT("clear no rain"), Clear.RainIntensity < Eps);
	TestTrue(TEXT("clear dry"), Clear.Wetness < Eps);
	TestTrue(TEXT("clear day not night"), Clear.NightAmount < Eps);

	// Storm at noon: full cloud, raining, wet, and darkened despite full daylight.
	const FWorldGlobals Storm = FWeatherFront::Globals(1.0, 1.0, 0.5);
	TestTrue(TEXT("storm full cloud"), FMath::Abs(Storm.CloudCoverage - 1.0) < Eps);
	TestTrue(TEXT("storm raining"), Storm.RainIntensity > 0.9);
	TestTrue(TEXT("storm wet"), Storm.Wetness > 0.9);
	TestTrue(TEXT("storm darkens midday"), FMath::Abs(Storm.NightAmount - 0.5) < Eps);

	// Overcast precedes rain: clouds present at mid intensity while rain is still 0.
	const FWorldGlobals Overcast = FWeatherFront::Globals(0.5, 1.0);
	TestTrue(TEXT("overcast has cloud"), Overcast.CloudCoverage > 0.4);
	TestTrue(TEXT("overcast not yet raining"), Overcast.RainIntensity < Eps);

	// Clear night: night-amount full from darkness alone.
	const FWorldGlobals Night = FWeatherFront::Globals(0.0, 0.0);
	TestTrue(TEXT("clear night is dark"), FMath::Abs(Night.NightAmount - 1.0) < Eps);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
