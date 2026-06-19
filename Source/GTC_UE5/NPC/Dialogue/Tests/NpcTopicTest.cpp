// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcTopic.h"

/**
 * Behavioural tests for FNpcTopic: the shared topic is symmetric + deterministic
 * + spread across pairs, every topic has non-empty opener/reply lines that wrap
 * by index, and an unknown topic falls back to small talk.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNpcTopicTest,
	"GTC.NPC.Dialogue.NpcTopic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcTopicTest::RunTest(const FString& Parameters)
{
	// --- Shared topic is symmetric and deterministic --------------------------
	{
		TestTrue(TEXT("Shared(A,B) == Shared(B,A)"),
			FNpcTopic::Shared(12, 87) == FNpcTopic::Shared(87, 12));
		TestTrue(TEXT("Shared is deterministic"),
			FNpcTopic::Shared(3, 9) == FNpcTopic::Shared(3, 9));
	}

	// --- Shared never returns the fallback, and spreads across pairs ----------
	{
		bool bNeverSmallTalk = true;
		TSet<ENpcChatTopic> Seen;
		for (int32 a = 0; a < 40; ++a)
		{
			for (int32 b = 0; b < 5; ++b)
			{
				const ENpcChatTopic T = FNpcTopic::Shared(a, a * 7 + b);
				if (T == ENpcChatTopic::SmallTalk) { bNeverSmallTalk = false; }
				Seen.Add(T);
			}
		}
		TestTrue(TEXT("Shared never yields the small-talk fallback"), bNeverSmallTalk);
		TestTrue(TEXT("different pairs land on different topics"), Seen.Num() >= 3);
	}

	// --- Every topic has non-empty opener + reply lines, wrapping by index ----
	{
		const ENpcChatTopic Topics[] = {
			ENpcChatTopic::Weather, ENpcChatTopic::City, ENpcChatTopic::Food,
			ENpcChatTopic::Gossip, ENpcChatTopic::Pigeons, ENpcChatTopic::Work,
			ENpcChatTopic::SmallTalk };
		bool bAllGood = true;
		for (ENpcChatTopic T : Topics)
		{
			for (bool bReply : {false, true})
			{
				const int32 C = FNpcTopic::Count(T, bReply);
				if (C <= 0) { bAllGood = false; continue; }
				if (FNpcTopic::Line(T, bReply, 0).IsEmpty()) { bAllGood = false; }
				// posmod wrap + negative safety
				if (FNpcTopic::Line(T, bReply, C) != FNpcTopic::Line(T, bReply, 0)) { bAllGood = false; }
				if (FNpcTopic::Line(T, bReply, -1).IsEmpty()) { bAllGood = false; }
			}
		}
		TestTrue(TEXT("every topic has wrapping, non-empty opener+reply banks"), bAllGood);
	}

	// --- Opener and reply differ (a reply is a distinct line) -----------------
	{
		TestNotEqual(TEXT("weather opener != weather reply"),
			FNpcTopic::Line(ENpcChatTopic::Weather, false, 0),
			FNpcTopic::Line(ENpcChatTopic::Weather, true, 0));
	}

	// --- Name tokens are present ----------------------------------------------
	{
		TestEqual(TEXT("pigeons name"), FString(FNpcTopic::Name(ENpcChatTopic::Pigeons)), FString(TEXT("pigeons")));
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
