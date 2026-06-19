// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../ResidencyBudget.h"

/**
 * Unit tests for FResidencyBudget (resident-set byte accounting + eviction).
 * Prefix GTC.World.Streaming.Residency. Byte values are small here and cast to
 * int32 for comparison to dodge the int64 TestEqual overload.
 */

namespace
{
	FResidentTile Tile(int64 Bytes, double EvictionKey)
	{
		FResidentTile T;
		T.Bytes = Bytes;
		T.EvictionKey = EvictionKey;
		return T;
	}
}

// --- accounting -----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResidencyBudgetTotalsTest,
	"GTC.World.Streaming.Residency.Totals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResidencyBudgetTotalsTest::RunTest(const FString& Parameters)
{
	const TArray<FResidentTile> R = {Tile(100, 1.0), Tile(200, 2.0), Tile(50, 3.0)};

	TestEqual(TEXT("total bytes"), (int32)FResidencyBudget::TotalBytes(R), 350);
	TestTrue(TEXT("over budget"), FResidencyBudget::IsOverBudget(R, 300));
	TestFalse(TEXT("under budget"), FResidencyBudget::IsOverBudget(R, 400));
	TestFalse(TEXT("exactly at budget not over"), FResidencyBudget::IsOverBudget(R, 350));
	TestEqual(TEXT("empty total"), (int32)FResidencyBudget::TotalBytes(TArray<FResidentTile>()), 0);

	return true;
}

// --- SelectEvictions ------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResidencyBudgetSelectTest,
	"GTC.World.Streaming.Residency.SelectEvictions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResidencyBudgetSelectTest::RunTest(const FString& Parameters)
{
	// index:    0          1          2
	// keys:     1          5          3   -> evict order by key desc: 1, 2, 0
	const TArray<FResidentTile> R = {Tile(100, 1.0), Tile(100, 5.0), Tile(100, 3.0)};

	// Free 100: just the highest-key tile (index 1).
	{
		const TArray<int32> E = FResidencyBudget::SelectEvictions(R, 100);
		TestEqual(TEXT("free100 count"), E.Num(), 1);
		TestEqual(TEXT("free100 victim"), E[0], 1);
	}

	// Free 150: highest then next-highest key (1 then 2).
	{
		const TArray<int32> E = FResidencyBudget::SelectEvictions(R, 150);
		TestEqual(TEXT("free150 count"), E.Num(), 2);
		TestEqual(TEXT("free150 first"), E[0], 1);
		TestEqual(TEXT("free150 second"), E[1], 2);
	}

	// Free more than the whole set: evict everything (best effort), key order.
	{
		const TArray<int32> E = FResidencyBudget::SelectEvictions(R, 10000);
		TestEqual(TEXT("free-all count"), E.Num(), 3);
		TestEqual(TEXT("free-all order 0"), E[0], 1);
		TestEqual(TEXT("free-all order 1"), E[1], 2);
		TestEqual(TEXT("free-all order 2"), E[2], 0);
	}

	// Nothing to free -> empty.
	TestEqual(TEXT("free0 empty"), FResidencyBudget::SelectEvictions(R, 0).Num(), 0);

	return true;
}

// --- tie-break: equal key evicts the larger tile first --------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResidencyBudgetTieBreakTest,
	"GTC.World.Streaming.Residency.TieBreak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResidencyBudgetTieBreakTest::RunTest(const FString& Parameters)
{
	// Same eviction key; index 1 is the larger tile -> evicted first so a single
	// eviction frees the needed bytes.
	const TArray<FResidentTile> R = {Tile(50, 5.0), Tile(200, 5.0)};
	const TArray<int32> E = FResidencyBudget::SelectEvictions(R, 100);
	TestEqual(TEXT("tie count"), E.Num(), 1);
	TestEqual(TEXT("tie evicts larger"), E[0], 1);

	return true;
}

// --- EvictionsToAdmit -----------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResidencyBudgetAdmitTest,
	"GTC.World.Streaming.Residency.EvictionsToAdmit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResidencyBudgetAdmitTest::RunTest(const FString& Parameters)
{
	// Resident total 300, budget 300 (full). Admitting a 200-byte tile needs to
	// free 200 -> evict highest keys until >= 200 freed.
	const TArray<FResidentTile> R = {Tile(100, 1.0), Tile(100, 9.0), Tile(100, 5.0)};
	{
		const TArray<int32> E = FResidencyBudget::EvictionsToAdmit(R, 200, 300);
		TestEqual(TEXT("admit200 count"), E.Num(), 2);
		TestEqual(TEXT("admit200 first (key9)"), E[0], 1);
		TestEqual(TEXT("admit200 second (key5)"), E[1], 2);
	}

	// Incoming already fits (300 + 50 <= 400): no eviction.
	TestEqual(TEXT("fits no eviction"), FResidencyBudget::EvictionsToAdmit(R, 50, 400).Num(), 0);

	// Incoming 0, over budget: trim back under the cap (free 300-250=50 -> 1 tile).
	{
		const TArray<int32> E = FResidencyBudget::EvictionsToAdmit(R, 0, 250);
		TestEqual(TEXT("trim count"), E.Num(), 1);
		TestEqual(TEXT("trim highest key"), E[0], 1);
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
