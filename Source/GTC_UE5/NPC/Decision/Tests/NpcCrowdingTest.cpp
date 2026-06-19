// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcCrowding.h"

/**
 * Behavioural tests for FNpcCrowding: occupancy/capacity classifies into the
 * busyness tiers, uncapped places never read as packed, crowd-averse vs
 * crowd-loving citizens avoid differently, and a needed-but-packed place queues.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNpcCrowdingTest,
	"GTC.NPC.Decision.NpcCrowding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcCrowdingTest::RunTest(const FString& Parameters)
{
	// --- Classification thresholds --------------------------------------------
	{
		TestTrue(TEXT("0/4 -> empty"), FNpcCrowding::Classify(0, 4) == ENpcBusyness::Empty);
		TestTrue(TEXT("1/4 -> quiet"), FNpcCrowding::Classify(1, 4) == ENpcBusyness::Quiet);
		TestTrue(TEXT("2/4 -> busy"), FNpcCrowding::Classify(2, 4) == ENpcBusyness::Busy);
		TestTrue(TEXT("4/4 -> packed"), FNpcCrowding::Classify(4, 4) == ENpcBusyness::Packed);
		TestTrue(TEXT("over capacity -> packed"), FNpcCrowding::Classify(7, 4) == ENpcBusyness::Packed);
	}

	// --- Uncapped places are never packed -------------------------------------
	{
		TestTrue(TEXT("uncapped + empty -> empty"), FNpcCrowding::Classify(0, 0) == ENpcBusyness::Empty);
		TestTrue(TEXT("uncapped + occupied -> quiet"), FNpcCrowding::Classify(50, 0) == ENpcBusyness::Quiet);
	}

	// --- Avoidance by tolerance -----------------------------------------------
	{
		// Crowd-averse (low tolerance) avoids packed; crowd-loving doesn't.
		TestTrue(TEXT("averse avoids packed"), FNpcCrowding::ShouldAvoid(ENpcBusyness::Packed, 0.1));
		TestFalse(TEXT("crowd-lover dives into packed"), FNpcCrowding::ShouldAvoid(ENpcBusyness::Packed, 0.9));
		// Only the very averse avoid a merely-busy place.
		TestTrue(TEXT("very averse avoids busy"), FNpcCrowding::ShouldAvoid(ENpcBusyness::Busy, 0.1));
		TestFalse(TEXT("average tolerates busy"), FNpcCrowding::ShouldAvoid(ENpcBusyness::Busy, 0.5));
		// Nobody avoids a quiet/empty place.
		TestFalse(TEXT("nobody avoids quiet"), FNpcCrowding::ShouldAvoid(ENpcBusyness::Quiet, 0.0));
		TestFalse(TEXT("nobody avoids empty"), FNpcCrowding::ShouldAvoid(ENpcBusyness::Empty, 0.0));
	}

	// --- Queue only when packed AND not avoiding ------------------------------
	{
		TestTrue(TEXT("packed + staying -> queue"), FNpcCrowding::ShouldQueue(ENpcBusyness::Packed, /*avoiding*/false));
		TestFalse(TEXT("packed + avoiding -> no queue (leaves)"), FNpcCrowding::ShouldQueue(ENpcBusyness::Packed, true));
		TestFalse(TEXT("busy -> no queue"), FNpcCrowding::ShouldQueue(ENpcBusyness::Busy, false));
	}

	// --- Standoff distance: comfortable walks up, busy/packed hangs back ------
	{
		TestEqual(TEXT("empty -> walk right up"), FNpcCrowding::StandoffDistance(ENpcBusyness::Empty, 0.5), 0.0f);
		TestEqual(TEXT("quiet -> walk right up"), FNpcCrowding::StandoffDistance(ENpcBusyness::Quiet, 0.5), 0.0f);
		// Packed always pushes back; the averse keep more distance than the lovers.
		const float PackedAverse = FNpcCrowding::StandoffDistance(ENpcBusyness::Packed, 0.1);
		const float PackedLover = FNpcCrowding::StandoffDistance(ENpcBusyness::Packed, 0.9);
		TestTrue(TEXT("packed pushes everyone back"), PackedLover > 0.0f);
		TestTrue(TEXT("crowd-averse hang back further when packed"), PackedAverse > PackedLover);
		// Packed > Busy at equal tolerance.
		TestTrue(TEXT("packed stands off further than busy"),
			FNpcCrowding::StandoffDistance(ENpcBusyness::Packed, 0.3)
				> FNpcCrowding::StandoffDistance(ENpcBusyness::Busy, 0.3));
		// A crowd-lover doesn't mind a merely-busy spot.
		TestEqual(TEXT("crowd-lover walks up to a busy spot"),
			FNpcCrowding::StandoffDistance(ENpcBusyness::Busy, 1.0), 0.0f);
	}

	// --- Names ----------------------------------------------------------------
	{
		TestEqual(TEXT("packed name"), FString(FNpcCrowding::Name(ENpcBusyness::Packed)), FString(TEXT("packed")));
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
