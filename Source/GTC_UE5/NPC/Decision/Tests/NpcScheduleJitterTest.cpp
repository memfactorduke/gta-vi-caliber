// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcScheduleJitter.h"

/**
 * Behavioural tests for FNpcScheduleJitter: each citizen's clock shift is
 * deterministic, bounded to the jitter window, spread across the population (so
 * the city doesn't move in lockstep), and Apply wraps into a valid [0,24) hour.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNpcScheduleJitterTest,
	"GTC.NPC.Decision.NpcScheduleJitter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcScheduleJitterTest::RunTest(const FString& Parameters)
{
	// --- Determinism ----------------------------------------------------------
	{
		TestEqual(TEXT("same seed -> same offset"),
			FNpcScheduleJitter::HourOffset(42), FNpcScheduleJitter::HourOffset(42));
	}

	// --- Bounded to the jitter window, and spread across the population -------
	{
		double MinOff = 1e9, MaxOff = -1e9;
		bool bInRange = true;
		for (int32 Seed = 0; Seed < 300; ++Seed)
		{
			const double Off = FNpcScheduleJitter::HourOffset(Seed);
			if (Off < -FNpcScheduleJitter::DefaultMaxHours - 1e-9
				|| Off > FNpcScheduleJitter::DefaultMaxHours + 1e-9)
			{
				bInRange = false;
			}
			MinOff = FMath::Min(MinOff, Off);
			MaxOff = FMath::Max(MaxOff, Off);
		}
		TestTrue(TEXT("offset stays within +/- the jitter window"), bInRange);
		TestTrue(TEXT("population spreads both earlier and later"), MinOff < -0.3 && MaxOff > 0.3);
	}

	// --- Two citizens generally don't share the same offset -------------------
	{
		TestNotEqual(TEXT("different seeds differ"),
			FNpcScheduleJitter::HourOffset(1), FNpcScheduleJitter::HourOffset(2));
	}

	// --- Zero window is a no-op -----------------------------------------------
	{
		TestEqual(TEXT("zero window -> no offset"), FNpcScheduleJitter::HourOffset(7, 0.0), 0.0);
		TestEqual(TEXT("zero window -> Apply is identity"),
			FNpcScheduleJitter::Apply(9.0, 7, 0.0), 9.0);
	}

	// --- Apply wraps into [0,24) ----------------------------------------------
	{
		bool bAllValid = true;
		for (int32 Seed = 0; Seed < 200; ++Seed)
		{
			const double H = FNpcScheduleJitter::Apply(0.2, Seed); // near midnight, negative shift wraps
			if (H < 0.0 || H >= 24.0) { bAllValid = false; }
			const double H2 = FNpcScheduleJitter::Apply(23.8, Seed); // late, positive shift wraps
			if (H2 < 0.0 || H2 >= 24.0) { bAllValid = false; }
		}
		TestTrue(TEXT("Apply always yields a valid clock hour"), bAllValid);
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
