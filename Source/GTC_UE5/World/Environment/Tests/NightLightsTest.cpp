// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NightLights.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FNightLights. Proves the hysteresis band holds a light's state (no
 * chatter near the threshold), jitter spreads each light's switch point, and the
 * aggregate lit fraction ramps from all-on at night to all-off by day — and that
 * a jittered population's on-count tracks that fraction. Prefix
 * GTC.World.NightLights.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNightLightsHysteresisTest,
	"GTC.World.NightLights.Hysteresis",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNightLightsHysteresisTest::RunTest(const FString& Parameters)
{
	const double On = 0.25;
	const double Off = 0.35;

	// Extremes: full dark turns an off light on; full day turns a lit light off.
	TestTrue(TEXT("dark turns on"), FNightLights::ShouldBeLit(false, 0.0, On, Off));
	TestTrue(TEXT("day turns off"), !FNightLights::ShouldBeLit(true, 1.0, On, Off));

	// In the hold band, state is preserved either way.
	TestTrue(TEXT("band holds lit"), FNightLights::ShouldBeLit(true, 0.30, On, Off));
	TestTrue(TEXT("band holds unlit"), !FNightLights::ShouldBeLit(false, 0.30, On, Off));

	// No chatter: dither daylight inside the band; an initially-on light never flips.
	bool bLit = true;
	int32 Flips = 0;
	const double Samples[8] = { 0.28, 0.33, 0.26, 0.34, 0.30, 0.27, 0.32, 0.29 };
	for (int32 i = 0; i < 8; ++i)
	{
		const bool Next = FNightLights::ShouldBeLit(bLit, Samples[i], On, Off);
		if (Next != bLit) { ++Flips; }
		bLit = Next;
	}
	TestEqual(TEXT("no chatter inside the band"), Flips, 0);

	// A full dusk sweep flips exactly once (on), a full dawn sweep exactly once (off).
	bLit = false; Flips = 0;
	for (int32 i = 0; i <= 20; ++i)
	{
		const double Day = 1.0 - i * 0.05; // day -> night
		const bool Next = FNightLights::ShouldBeLit(bLit, Day, On, Off);
		if (Next != bLit) { ++Flips; }
		bLit = Next;
	}
	TestEqual(TEXT("dusk turns on exactly once"), Flips, 1);
	TestTrue(TEXT("lit by deep night"), bLit);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNightLightsJitterTest,
	"GTC.World.NightLights.Jitter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNightLightsJitterTest::RunTest(const FString& Parameters)
{
	const double Base = 0.25;
	const double Spread = 0.08;

	// Midpoint seed leaves the base unchanged; extremes shift +/- half the spread.
	TestTrue(TEXT("mid seed = base"), FMath::Abs(FNightLights::JitteredThreshold(Base, 0.5, Spread) - Base) < Eps);
	TestTrue(TEXT("low seed earliest"),
		FMath::Abs(FNightLights::JitteredThreshold(Base, 0.0, Spread) - (Base - Spread * 0.5)) < Eps);
	TestTrue(TEXT("high seed latest"),
		FMath::Abs(FNightLights::JitteredThreshold(Base, 1.0, Spread) - (Base + Spread * 0.5)) < Eps);

	// Bounded to [0,1] even with a wild base/spread.
	TestTrue(TEXT("clamped low"), FNightLights::JitteredThreshold(0.0, 0.0, 1.0) >= 0.0);
	TestTrue(TEXT("clamped high"), FNightLights::JitteredThreshold(1.0, 1.0, 1.0) <= 1.0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNightLightsFractionTest,
	"GTC.World.NightLights.Fraction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNightLightsFractionTest::RunTest(const FString& Parameters)
{
	const double On = 0.25;
	const double Off = 0.35;

	// All lit at night, all dark by day, monotone, in [0,1].
	TestTrue(TEXT("night all lit"), FMath::Abs(FNightLights::LitFraction(0.0, On, Off) - 1.0) < Eps);
	TestTrue(TEXT("day all dark"), FNightLights::LitFraction(1.0, On, Off) < Eps);

	bool bMonotone = true, bBounded = true;
	double Prev = 2.0;
	for (int32 i = 0; i <= 50; ++i)
	{
		const double Day = i * 0.02;
		const double F = FNightLights::LitFraction(Day, On, Off);
		if (F > Prev + Eps) { bMonotone = false; } // fewer lit as it brightens
		if (F < -Eps || F > 1.0 + Eps) { bBounded = false; }
		Prev = F;
	}
	TestTrue(TEXT("fraction monotone in daylight"), bMonotone);
	TestTrue(TEXT("fraction bounded"), bBounded);

	// A jittered population's on-count tracks the aggregate fraction within the band.
	const int32 N = 400;
	const double Day = 0.30; // mid-band
	int32 OnCount = 0;
	for (int32 i = 0; i < N; ++i)
	{
		const double Seed = (i + 0.5) / N; // evenly spread seeds
		const double LightOn = FNightLights::JitteredThreshold(On, Seed, 0.20);
		const double LightOff = FNightLights::JitteredThreshold(Off, Seed, 0.20);
		// Steady-state lit if dark enough for this light's own dusk edge.
		if (FNightLights::ShouldBeLit(false, Day, LightOn, LightOff)) { ++OnCount; }
	}
	const double PopFraction = (double)OnCount / N;
	// The population spreads the switch over the jitter, so at mid-band a healthy
	// share are already on — strictly between none and all (not a hard step).
	TestTrue(TEXT("population partially lit mid-band"), PopFraction > 0.05 && PopFraction < 0.95);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
