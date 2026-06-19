// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../StreamBudget.h"

/**
 * Unit tests for FStreamBudget (per-frame admission budgeting).
 * Prefix GTC.World.Streaming.Budget.
 */

// --- CanAdmit -------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStreamBudgetCanAdmitTest,
	"GTC.World.Streaming.Budget.CanAdmit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStreamBudgetCanAdmitTest::RunTest(const FString& Parameters)
{
	// First item always admitted (forward progress) even if its cost dwarfs the
	// per-tick cost budget.
	TestTrue(TEXT("first admitted despite cost"), FStreamBudget::CanAdmit(0, 0.0, 999.0, 5, 2.0));

	// Subsequent items respect the cost budget.
	TestTrue(TEXT("second fits"), FStreamBudget::CanAdmit(1, 1.0, 1.0, 5, 2.0));   // 1+1 <= 2
	TestFalse(TEXT("second over budget"), FStreamBudget::CanAdmit(1, 1.0, 2.0, 5, 2.0)); // 1+2 > 2

	// Op-limit stops admission regardless of remaining cost budget.
	TestFalse(TEXT("op limit reached"), FStreamBudget::CanAdmit(5, 0.0, 0.1, 5, 1000.0));

	// Non-positive op budget admits nothing, not even the first.
	TestFalse(TEXT("zero ops admits nothing"), FStreamBudget::CanAdmit(0, 0.0, 0.1, 0, 1000.0));

	return true;
}

// --- AdmitCount -----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStreamBudgetAdmitCountTest,
	"GTC.World.Streaming.Budget.AdmitCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStreamBudgetAdmitCountTest::RunTest(const FString& Parameters)
{
	const TArray<double> Five = {1.0, 1.0, 1.0, 1.0, 1.0};

	// Op-limited: 3 of 5 admitted.
	TestEqual(TEXT("op limited"), FStreamBudget::AdmitCount(Five, 3, 100.0), 3);

	// Cost-limited: budget 2.5 admits first (1) + second (2) then stops (3 > 2.5).
	TestEqual(TEXT("cost limited"), FStreamBudget::AdmitCount(Five, 100, 2.5), 2);

	// All fit when both budgets are generous.
	TestEqual(TEXT("all fit"), FStreamBudget::AdmitCount(Five, 100, 100.0), 5);

	// Oversized first item: admitted alone to make progress.
	TestEqual(TEXT("oversized first alone"), FStreamBudget::AdmitCount(TArray<double>({10.0, 1.0}), 5, 2.0), 1);

	// Empty queue admits nothing; zero ops admits nothing.
	TestEqual(TEXT("empty"), FStreamBudget::AdmitCount(TArray<double>(), 5, 100.0), 0);
	TestEqual(TEXT("zero ops"), FStreamBudget::AdmitCount(Five, 0, 100.0), 0);

	// Mixed costs within budget: 1+2+3 = 6 <= 10 -> all three.
	TestEqual(TEXT("mixed within"), FStreamBudget::AdmitCount(TArray<double>({1.0, 2.0, 3.0}), 10, 10.0), 3);

	return true;
}

// --- spreading a burst across frames --------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStreamBudgetSpreadTest,
	"GTC.World.Streaming.Budget.Spread",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStreamBudgetSpreadTest::RunTest(const FString& Parameters)
{
	// A burst of 10 unit-cost tiles, budget 3/tick: drains over 4 ticks
	// (3 + 3 + 3 + 1) and never admits more than the op cap in any single tick.
	TArray<double> Pending;
	for (int32 I = 0; I < 10; ++I)
	{
		Pending.Add(1.0);
	}

	int32 Ticks = 0;
	int32 Drained = 0;
	int32 MaxInOneTick = 0;
	while (Pending.Num() > 0 && Ticks < 100)
	{
		const int32 N = FStreamBudget::AdmitCount(Pending, 3, 1000.0);
		MaxInOneTick = FMath::Max(MaxInOneTick, N);
		TestTrue(TEXT("makes progress each tick"), N > 0);
		Pending.RemoveAt(0, N);
		Drained += N;
		++Ticks;
	}

	TestEqual(TEXT("drained all"), Drained, 10);
	TestEqual(TEXT("took 4 ticks"), Ticks, 4);
	TestTrue(TEXT("never exceeded op cap"), MaxInOneTick <= 3);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
