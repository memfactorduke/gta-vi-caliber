// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../TileHysteresis.h"
#include "../../../Tests/GtcTestTolerances.h"

/**
 * Unit tests for FTileHysteresis (two-radius load/unload band + dwell timer).
 * Prefix GTC.World.Streaming.Hysteresis.
 */

using GtcTest::Eps;

// --- ShouldBeResident -----------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTileHysteresisResidentTest,
	"GTC.World.Streaming.Hysteresis.ShouldBeResident",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTileHysteresisResidentTest::RunTest(const FString& Parameters)
{
	const double Load = 100.0;
	const double Unload = 150.0;

	// Inside the inner radius: load regardless of prior state.
	TestTrue(TEXT("inner loads (was out)"), FTileHysteresis::ShouldBeResident(false, 50.0, Load, Unload));
	TestTrue(TEXT("inner stays (was in)"), FTileHysteresis::ShouldBeResident(true, 50.0, Load, Unload));

	// Beyond the outer radius: unload regardless of prior state.
	TestFalse(TEXT("outer unloads (was in)"), FTileHysteresis::ShouldBeResident(true, 200.0, Load, Unload));
	TestFalse(TEXT("outer stays out (was out)"), FTileHysteresis::ShouldBeResident(false, 200.0, Load, Unload));

	// In the band: hold whatever state we had — this is the anti-thrash core.
	TestTrue(TEXT("band holds resident"), FTileHysteresis::ShouldBeResident(true, 120.0, Load, Unload));
	TestFalse(TEXT("band holds unloaded"), FTileHysteresis::ShouldBeResident(false, 120.0, Load, Unload));

	// Edges: load edge inclusive (resident), unload edge inclusive (unload).
	TestTrue(TEXT("on load edge is resident"), FTileHysteresis::ShouldBeResident(false, 100.0, Load, Unload));
	TestFalse(TEXT("on unload edge unloads"), FTileHysteresis::ShouldBeResident(true, 150.0, Load, Unload));

	// Misconfigured pair (Unload < Load) degrades to a single threshold at Load,
	// never inverts: a tile in [Unload,Load] is resident, not flapping.
	TestTrue(TEXT("bad pair: within load resident"), FTileHysteresis::ShouldBeResident(false, 120.0, 150.0, 100.0));
	TestFalse(TEXT("bad pair: beyond load unloads"), FTileHysteresis::ShouldBeResident(true, 200.0, 150.0, 100.0));

	return true;
}

// --- anti-thrash sweep ----------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTileHysteresisNoThrashTest,
	"GTC.World.Streaming.Hysteresis.NoThrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTileHysteresisNoThrashTest::RunTest(const FString& Parameters)
{
	const double Load = 100.0;
	const double Unload = 150.0;

	// A camera oscillating inside the band must not flip the tile state. Start
	// resident (entered from inside), then jitter across the band; it stays.
	bool bResident = true;
	const double Jitter[] = {110.0, 140.0, 105.0, 149.0, 101.0, 130.0};
	int32 Flips = 0;
	for (double D : Jitter)
	{
		const bool bNext = FTileHysteresis::ShouldBeResident(bResident, D, Load, Unload);
		if (bNext != bResident)
		{
			++Flips;
		}
		bResident = bNext;
	}
	TestEqual(TEXT("no flips while jittering in band"), Flips, 0);
	TestTrue(TEXT("stayed resident"), bResident);

	// Only a decisive crossing past the outer radius flips it.
	bResident = FTileHysteresis::ShouldBeResident(bResident, 160.0, Load, Unload);
	TestFalse(TEXT("decisive exit unloads"), bResident);

	return true;
}

// --- InBand ---------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTileHysteresisInBandTest,
	"GTC.World.Streaming.Hysteresis.InBand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTileHysteresisInBandTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("inside band"), FTileHysteresis::InBand(120.0, 100.0, 150.0));
	TestFalse(TEXT("inside inner not band"), FTileHysteresis::InBand(50.0, 100.0, 150.0));
	TestFalse(TEXT("beyond outer not band"), FTileHysteresis::InBand(200.0, 100.0, 150.0));
	TestFalse(TEXT("on load edge not band"), FTileHysteresis::InBand(100.0, 100.0, 150.0));
	TestFalse(TEXT("on unload edge not band"), FTileHysteresis::InBand(150.0, 100.0, 150.0));
	TestFalse(TEXT("empty band"), FTileHysteresis::InBand(120.0, 150.0, 150.0));

	return true;
}

// --- AdvanceDwell ---------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTileHysteresisDwellTest,
	"GTC.World.Streaming.Hysteresis.Dwell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTileHysteresisDwellTest::RunTest(const FString& Parameters)
{
	// Accumulates while the condition holds.
	double Dwell = 0.0;
	Dwell = FTileHysteresis::AdvanceDwell(Dwell, true, 0.1);
	TestEqual(TEXT("first tick"), Dwell, 0.1, Eps);
	Dwell = FTileHysteresis::AdvanceDwell(Dwell, true, 0.1);
	TestEqual(TEXT("second tick"), Dwell, 0.2, Eps);

	// Resets the instant the condition drops.
	Dwell = FTileHysteresis::AdvanceDwell(Dwell, false, 0.1);
	TestEqual(TEXT("reset on drop"), Dwell, 0.0, Eps);

	// Negative delta is ignored (clamped to no progress).
	Dwell = FTileHysteresis::AdvanceDwell(0.5, true, -1.0);
	TestEqual(TEXT("negative delta ignored"), Dwell, 0.5, Eps);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
