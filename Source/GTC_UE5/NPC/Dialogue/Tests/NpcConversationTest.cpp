// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcConversation.h"

/**
 * Parity tests for FNpcConversation, mapped 1:1 from the the reference oracle
 * game/tests/unit/test_npc_conversation.gd (6 funcs). Each TestTrue corresponds
 * to one oracle assertion with the oracle's own literals/seeds. Compound returns
 * are split into independent TestTrue calls.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNpcConversationTest,
	"GTC.NPC.Conversation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcConversationTest::RunTest(const FString& Parameters)
{
	// test_exchange_has_requested_length
	{
		TestEqual(TEXT("exchange has requested length 6"),
			FNpcConversation::Exchange(TEXT("conspiracy"), TEXT("yogi"), 1, 6).Num(), 6);
	}

	// test_exchange_alternates_speakers
	{
		const TArray<FNpcConversationTurn> Ex = FNpcConversation::Exchange(TEXT("conspiracy"), TEXT("yogi"), 7, 4);
		TestEqual(TEXT("exchange[0] speaker == 0"), Ex[0].Speaker, 0);
		TestEqual(TEXT("exchange[1] speaker == 1"), Ex[1].Speaker, 1);
		TestEqual(TEXT("exchange[2] speaker == 0"), Ex[2].Speaker, 0);
		TestEqual(TEXT("exchange[3] speaker == 1"), Ex[3].Speaker, 1);
	}

	// test_exchange_lines_never_empty
	{
		bool bNeverEmpty = true;
		for (const FNpcConversationTurn& Entry : FNpcConversation::Exchange(TEXT("mime"), TEXT("weather"), 3, 8))
		{
			if (Entry.Text == TEXT(""))
			{
				bNeverEmpty = false;
			}
		}
		TestTrue(TEXT("exchange lines never empty"), bNeverEmpty);
	}

	// test_exchange_is_deterministic
	{
		const TArray<FNpcConversationTurn> A = FNpcConversation::Exchange(TEXT("philosopher"), TEXT("food_critic"), 42, 4);
		const TArray<FNpcConversationTurn> B = FNpcConversation::Exchange(TEXT("philosopher"), TEXT("food_critic"), 42, 4);
		bool bDeterministic = true;
		for (int32 i = 0; i < A.Num(); ++i)
		{
			if (A[i].Text != B[i].Text)
			{
				bDeterministic = false;
			}
		}
		TestTrue(TEXT("exchange is deterministic"), bDeterministic);
	}

	// test_exchange_minimum_one_turn
	{
		TestEqual(TEXT("exchange minimum one turn"),
			FNpcConversation::Exchange(TEXT("yogi"), TEXT("yogi"), 0, 0).Num(), 1);
	}

	// test_greeting_is_nonempty_and_stable
	{
		const FString G = FNpcConversation::Greeting(TEXT("influencer"), 5);
		TestNotEqual(TEXT("greeting nonempty"), G, FString(TEXT("")));
		TestEqual(TEXT("greeting stable"), G, FNpcConversation::Greeting(TEXT("influencer"), 5));
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
