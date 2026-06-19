// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcAcquaintance.h"

/**
 * Behavioural tests for FNpcAcquaintance: familiarity rises through the tiers
 * with repeated meetings, fades with Forget, the table is capacity-bounded with
 * least-familiar eviction (a friend survives a flood of strangers), and
 * anonymous (INDEX_NONE) encounters are never recorded.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNpcAcquaintanceTest,
	"GTC.NPC.Decision.NpcAcquaintance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcAcquaintanceTest::RunTest(const FString& Parameters)
{
	// --- Unknown person is a stranger -----------------------------------------
	{
		FNpcAcquaintance A;
		TestEqual(TEXT("unknown familiarity is 0"), A.FamiliarityWith(7), 0.0f);
		TestTrue(TEXT("unknown is a stranger"), A.TierWith(7) == ENpcFamiliarity::Stranger);
	}

	// --- Repeated meetings climb the tiers ------------------------------------
	{
		FNpcAcquaintance A;
		A.Meet(42);
		TestTrue(TEXT("one meeting -> familiar face"), A.TierWith(42) == ENpcFamiliarity::FamiliarFace);
		A.Meet(42);
		A.Meet(42); // 3.0 -> Friend
		TestTrue(TEXT("three meetings -> friend"), A.TierWith(42) == ENpcFamiliarity::Friend);
		TestEqual(TEXT("still one entry (same person)"), A.Num(), 1);
	}

	// --- Familiarity saturates at the ceiling ---------------------------------
	{
		FNpcAcquaintance A;
		for (int32 i = 0; i < 20; ++i) { A.Meet(1); }
		TestTrue(TEXT("familiarity caps at MaxFamiliarity"),
			A.FamiliarityWith(1) <= FNpcAcquaintance::MaxFamiliarity + KINDA_SMALL_NUMBER);
	}

	// --- Anonymous encounters are never recorded ------------------------------
	{
		FNpcAcquaintance A;
		A.Meet(INDEX_NONE);
		TestEqual(TEXT("INDEX_NONE is not recorded"), A.Num(), 0);
		TestEqual(TEXT("INDEX_NONE familiarity is 0"), A.FamiliarityWith(INDEX_NONE), 0.0f);
	}

	// --- Forgetting fades ties and drops dead ones ----------------------------
	{
		FNpcAcquaintance A;
		A.Meet(5); // 1.0
		A.Forget(0.5f);
		TestTrue(TEXT("forgetting reduces familiarity"), A.FamiliarityWith(5) < 1.0f);
		A.Forget(2.0f); // drives below 0 -> removed
		TestEqual(TEXT("fully forgotten entry is dropped"), A.Num(), 0);
		TestTrue(TEXT("forgotten person is a stranger again"), A.TierWith(5) == ENpcFamiliarity::Stranger);
	}

	// --- Capacity bound + least-familiar eviction -----------------------------
	{
		FNpcAcquaintance A;
		// Make id 999 a real friend first.
		A.Meet(999); A.Meet(999); A.Meet(999); A.Meet(999); // saturating toward Friend
		// Now flood the book with many one-off strangers, far exceeding Capacity.
		for (int32 i = 0; i < FNpcAcquaintance::Capacity * 3; ++i)
		{
			A.Meet(1000 + i);
		}
		TestTrue(TEXT("table never exceeds capacity"), A.Num() <= FNpcAcquaintance::Capacity);
		TestTrue(TEXT("a close friend survives a flood of strangers"),
			A.TierWith(999) == ENpcFamiliarity::Friend);
	}

	// --- Tier thresholds ------------------------------------------------------
	{
		TestTrue(TEXT("0 -> stranger"), FNpcAcquaintance::TierFor(0.0f) == ENpcFamiliarity::Stranger);
		TestTrue(TEXT("1 -> familiar"), FNpcAcquaintance::TierFor(1.0f) == ENpcFamiliarity::FamiliarFace);
		TestTrue(TEXT("3 -> friend"), FNpcAcquaintance::TierFor(3.0f) == ENpcFamiliarity::Friend);
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
