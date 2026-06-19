// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcDetour.h"

/**
 * Behavioural tests for FNpcDetour: glance chance rises with curiosity and stays
 * bounded, and the cooldown is in a sane range, varies by seed, and is shorter
 * for the curious.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNpcDetourTest,
	"GTC.NPC.Decision.NpcDetour",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcDetourTest::RunTest(const FString& Parameters)
{
	// --- Glance chance rises with curiosity, stays bounded --------------------
	{
		const float Incurious = FNpcDetour::GlanceChance(0.0);
		const float Nosy = FNpcDetour::GlanceChance(1.0);
		TestTrue(TEXT("nosier citizens glance more"), Nosy > Incurious);
		TestTrue(TEXT("even the incurious glance sometimes"), Incurious > 0.0f);
		TestTrue(TEXT("chance is bounded"), Nosy <= 0.25f);
		// Clamps out-of-range curiosity.
		TestEqual(TEXT("over-1 curiosity clamps to the 1.0 chance"),
			FNpcDetour::GlanceChance(5.0), FNpcDetour::GlanceChance(1.0));
	}

	// --- Cooldown range, seed variation, curiosity effect ---------------------
	{
		bool bAllInRange = true;
		double MinCd = 1e9, MaxCd = -1e9;
		for (int32 Seed = 0; Seed < 200; ++Seed)
		{
			const double Cd = FNpcDetour::GlanceCooldown(0.5, Seed);
			if (Cd < 1.0 || Cd > 12.0) { bAllInRange = false; }
			MinCd = FMath::Min(MinCd, Cd);
			MaxCd = FMath::Max(MaxCd, Cd);
		}
		TestTrue(TEXT("cooldown stays in a sane range"), bAllInRange);
		TestTrue(TEXT("cooldown varies by seed"), (MaxCd - MinCd) > 1.0);

		// Determinism.
		TestEqual(TEXT("same curiosity+seed -> same cooldown"),
			FNpcDetour::GlanceCooldown(0.5, 9), FNpcDetour::GlanceCooldown(0.5, 9));

		// Averaged over seeds, the curious wait less between glances.
		double CuriousSum = 0.0, IncuriousSum = 0.0;
		const int32 N = 64;
		for (int32 Seed = 0; Seed < N; ++Seed)
		{
			CuriousSum += FNpcDetour::GlanceCooldown(1.0, Seed);
			IncuriousSum += FNpcDetour::GlanceCooldown(0.0, Seed);
		}
		TestTrue(TEXT("the curious glance more often (shorter cooldown)"), CuriousSum < IncuriousSum);
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
