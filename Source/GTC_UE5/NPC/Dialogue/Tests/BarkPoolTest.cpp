// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../BarkPool.h"

/**
 * Parity tests for FBarkPool, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_bark_pool.gd (7 funcs). Each TestTrue corresponds to one
 * oracle assertion with the oracle's own literals/indices. Compound `a and b`
 * returns are split into independent TestTrue calls.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBarkPoolTest,
	"GTC.NPC.Bark",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBarkPoolTest::RunTest(const FString& Parameters)
{
	// test_idle_line_nonempty
	TestNotEqual(TEXT("idle line nonempty"),
		FBarkPool::Line(EBarkSituation::IDLE, 0), FString(TEXT("")));

	// test_flee_line_nonempty
	TestNotEqual(TEXT("flee line nonempty"),
		FBarkPool::Line(EBarkSituation::FLEE, 0), FString(TEXT("")));

	// test_index_wraps
	{
		const int32 Count = FBarkPool::Count(EBarkSituation::FLEE);
		TestEqual(TEXT("index wraps: line(count) == line(0)"),
			FBarkPool::Line(EBarkSituation::FLEE, Count), FBarkPool::Line(EBarkSituation::FLEE, 0));
	}

	// test_negative_index_safe
	TestNotEqual(TEXT("negative index safe (posmod)"),
		FBarkPool::Line(EBarkSituation::IDLE, -1), FString(TEXT("")));

	// test_distinct_situations_have_lines
	{
		TestTrue(TEXT("idle has lines"), FBarkPool::Count(EBarkSituation::IDLE) > 0);
		TestTrue(TEXT("alarmed has lines"), FBarkPool::Count(EBarkSituation::ALARMED) > 0);
		TestTrue(TEXT("flee has lines"), FBarkPool::Count(EBarkSituation::FLEE) > 0);
	}

	// test_should_bark_respects_cooldown
	{
		TestTrue(TEXT("should bark when 3.0 >= 2.0"), FBarkPool::ShouldBark(3.0, 2.0));
		TestFalse(TEXT("should not bark when 1.0 >= 2.0"), FBarkPool::ShouldBark(1.0, 2.0));
	}

	// test_different_indices_can_differ
	{
		const FString A = FBarkPool::Line(EBarkSituation::IDLE, 0);
		const FString B = FBarkPool::Line(EBarkSituation::IDLE, 1);
		TestNotEqual(TEXT("different indices can differ"), A, B);
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
