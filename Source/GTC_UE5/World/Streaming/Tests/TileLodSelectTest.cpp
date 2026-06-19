// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../TileLodSelect.h"

/**
 * Unit tests for FTileLodSelect (per-tile detail-band selection + LOD
 * hysteresis). Prefix GTC.World.Streaming.Lod.
 */

namespace
{
	FTileLodBands MakeBands()
	{
		FTileLodBands B;
		B.FullDist = 100.0;
		B.HlodDist = 300.0;
		B.ImpostorDist = 600.0;
		return B;
	}
}

// --- Select (no hysteresis) -----------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTileLodSelectBandTest,
	"GTC.World.Streaming.Lod.Select",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTileLodSelectBandTest::RunTest(const FString& Parameters)
{
	const FTileLodBands B = MakeBands();

	TestTrue(TEXT("close -> Full"), FTileLodSelect::Select(50.0, B) == ETileLod::Full);
	TestTrue(TEXT("on full edge -> Full"), FTileLodSelect::Select(100.0, B) == ETileLod::Full);
	TestTrue(TEXT("mid -> HLOD"), FTileLodSelect::Select(200.0, B) == ETileLod::HLOD);
	TestTrue(TEXT("on hlod edge -> HLOD"), FTileLodSelect::Select(300.0, B) == ETileLod::HLOD);
	TestTrue(TEXT("far -> Impostor"), FTileLodSelect::Select(500.0, B) == ETileLod::Impostor);
	TestTrue(TEXT("on impostor edge -> Impostor"), FTileLodSelect::Select(600.0, B) == ETileLod::Impostor);
	TestTrue(TEXT("beyond -> Unloaded"), FTileLodSelect::Select(601.0, B) == ETileLod::Unloaded);

	// Misordered config normalises (running max) instead of inverting: HlodDist
	// below FullDist collapses the HLOD band to empty, never picks a smaller edge.
	FTileLodBands Bad;
	Bad.FullDist = 300.0;
	Bad.HlodDist = 100.0;     // < FullDist -> clamped up to 300
	Bad.ImpostorDist = 600.0;
	TestTrue(TEXT("bad config: within full"), FTileLodSelect::Select(200.0, Bad) == ETileLod::Full);
	TestTrue(TEXT("bad config: impostor band intact"), FTileLodSelect::Select(450.0, Bad) == ETileLod::Impostor);

	return true;
}

// --- SelectWithHysteresis -------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTileLodSelectHysteresisTest,
	"GTC.World.Streaming.Lod.Hysteresis",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTileLodSelectHysteresisTest::RunTest(const FString& Parameters)
{
	const FTileLodBands B = MakeBands();
	const double M = 10.0;

	// Just past the Full/HLOD boundary but within the margin: hold Full.
	TestTrue(TEXT("dead-zone holds Full"),
		FTileLodSelect::SelectWithHysteresis(105.0, ETileLod::Full, B, M) == ETileLod::Full);
	// Clearly past it: coarsen to HLOD.
	TestTrue(TEXT("past margin coarsens"),
		FTileLodSelect::SelectWithHysteresis(120.0, ETileLod::Full, B, M) == ETileLod::HLOD);

	// Coming back, just inside the boundary but within margin: hold HLOD (no
	// flicker back to Full).
	TestTrue(TEXT("dead-zone holds HLOD"),
		FTileLodSelect::SelectWithHysteresis(95.0, ETileLod::HLOD, B, M) == ETileLod::HLOD);
	// Clearly inside: refine to Full.
	TestTrue(TEXT("inside margin refines"),
		FTileLodSelect::SelectWithHysteresis(85.0, ETileLod::HLOD, B, M) == ETileLod::Full);

	// Multi-band jump out (teleport far): Full -> Unloaded in one resolve.
	TestTrue(TEXT("teleport far -> Unloaded"),
		FTileLodSelect::SelectWithHysteresis(700.0, ETileLod::Full, B, M) == ETileLod::Unloaded);
	// Multi-band jump in: Unloaded -> Full in one resolve.
	TestTrue(TEXT("teleport near -> Full"),
		FTileLodSelect::SelectWithHysteresis(50.0, ETileLod::Unloaded, B, M) == ETileLod::Full);

	// Stable: distance squarely in HLOD stays HLOD.
	TestTrue(TEXT("stable HLOD"),
		FTileLodSelect::SelectWithHysteresis(200.0, ETileLod::HLOD, B, M) == ETileLod::HLOD);

	return true;
}

// --- no flicker sweep -----------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTileLodSelectNoFlickerTest,
	"GTC.World.Streaming.Lod.NoFlicker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTileLodSelectNoFlickerTest::RunTest(const FString& Parameters)
{
	const FTileLodBands B = MakeBands();
	const double M = 10.0;

	// Oscillate distance across the Full/HLOD boundary within the dead-zone;
	// the band must not change.
	ETileLod Lod = ETileLod::Full;
	const double Jitter[] = {102.0, 98.0, 105.0, 95.0, 108.0, 92.0};
	int32 Changes = 0;
	for (double D : Jitter)
	{
		const ETileLod Next = FTileLodSelect::SelectWithHysteresis(D, Lod, B, M);
		if (Next != Lod)
		{
			++Changes;
		}
		Lod = Next;
	}
	TestEqual(TEXT("no band changes in dead-zone"), Changes, 0);
	TestTrue(TEXT("still Full"), Lod == ETileLod::Full);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
