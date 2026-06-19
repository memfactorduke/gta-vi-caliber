// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcWeatherReaction.h"

/**
 * Behavioural tests for FNpcWeatherReaction: discomfort rises with severity, a
 * clear day is a no-op, storms drive everyone to shelter, rain splits the bold
 * from the timid, and night classification + extra discomfort behave.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNpcWeatherReactionTest,
	"GTC.NPC.Decision.NpcWeatherReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcWeatherReactionTest::RunTest(const FString& Parameters)
{
	// --- Clear day is a no-op -------------------------------------------------
	{
		const FNpcWeatherResponse R = FNpcWeatherReaction::Assess(/*sev*/0, /*hour*/13.0, /*brave*/0.5);
		TestEqual(TEXT("clear day discomfort is 0"), R.Discomfort, 0.0f);
		TestFalse(TEXT("no shelter on a clear day"), R.bSeekShelter);
		TestFalse(TEXT("no hurry on a clear day"), R.bHurry);
	}

	// --- Discomfort rises monotonically with severity -------------------------
	{
		bool bMonotonic = true;
		for (int32 s = 1; s <= 4; ++s)
		{
			if (FNpcWeatherReaction::BaseDiscomfort(s) <= FNpcWeatherReaction::BaseDiscomfort(s - 1))
			{
				bMonotonic = false;
			}
		}
		TestTrue(TEXT("discomfort increases with severity"), bMonotonic);
	}

	// --- A storm drives even the bold to shelter ------------------------------
	{
		const FNpcWeatherResponse Bold = FNpcWeatherReaction::Assess(/*storm*/4, 13.0, /*brave*/1.0);
		const FNpcWeatherResponse Timid = FNpcWeatherReaction::Assess(4, 13.0, /*brave*/0.0);
		TestTrue(TEXT("bold seeks shelter in a storm"), Bold.bSeekShelter);
		TestTrue(TEXT("timid seeks shelter in a storm"), Timid.bSeekShelter);
		TestTrue(TEXT("storm makes people hurry"), Bold.bHurry);
	}

	// --- Rain splits the timid from the bold ----------------------------------
	{
		const FNpcWeatherResponse Timid = FNpcWeatherReaction::Assess(/*rain*/3, 13.0, /*brave*/0.0);
		const FNpcWeatherResponse Bold = FNpcWeatherReaction::Assess(3, 13.0, /*brave*/1.0);
		TestTrue(TEXT("timid seeks shelter in rain"), Timid.bSeekShelter);
		TestFalse(TEXT("bold toughs out the rain"), Bold.bSeekShelter);
		TestTrue(TEXT("rain still makes the bold hurry"), Bold.bHurry);
	}

	// --- Night classification + extra discomfort ------------------------------
	{
		TestTrue(TEXT("2am is night"), FNpcWeatherReaction::IsNight(2.0));
		TestTrue(TEXT("10pm is night"), FNpcWeatherReaction::IsNight(22.0));
		TestFalse(TEXT("1pm is day"), FNpcWeatherReaction::IsNight(13.0));
		TestFalse(TEXT("8am is day"), FNpcWeatherReaction::IsNight(8.0));

		const FNpcWeatherResponse Day = FNpcWeatherReaction::Assess(2, 13.0, 0.5);
		const FNpcWeatherResponse Night = FNpcWeatherReaction::Assess(2, 23.0, 0.5);
		TestTrue(TEXT("night is less comfortable than day for the same weather"),
			Night.Discomfort > Day.Discomfort);
	}

	// --- Bravery monotonicity at fixed weather --------------------------------
	{
		// At rain severity, a braver citizen is never MORE likely to flee than a
		// timid one (shelter is monotonic non-increasing in bravery).
		const bool TimidFlees = FNpcWeatherReaction::Assess(3, 13.0, 0.1).bSeekShelter;
		const bool BoldFlees = FNpcWeatherReaction::Assess(3, 13.0, 0.9).bSeekShelter;
		TestTrue(TEXT("bravery does not increase shelter-seeking"), TimidFlees || !BoldFlees);
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
