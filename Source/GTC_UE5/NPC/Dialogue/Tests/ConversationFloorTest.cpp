// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../ConversationFloor.h"

/**
 * Behavioural tests for FConversationFloor: the lead role is antisymmetric, turn
 * indexing is monotonic and guarded, and exactly one participant holds the floor
 * on every turn (clean, overlap-free turn-taking).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FConversationFloorTest,
	"GTC.NPC.Conversation.Floor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FConversationFloorTest::RunTest(const FString& Parameters)
{
	// --- Lead is antisymmetric (both sides agree on who leads) ----------------
	{
		const bool AB = FConversationFloor::IsLead(10, 20, 100, 200);
		const bool BA = FConversationFloor::IsLead(20, 10, 200, 100);
		TestNotEqual(TEXT("IsLead(A,B) != IsLead(B,A)"), AB, BA);
		TestTrue(TEXT("lower seed leads"), FConversationFloor::IsLead(10, 20, 100, 200));
		TestFalse(TEXT("higher seed follows"), FConversationFloor::IsLead(20, 10, 200, 100));
	}

	// --- Tie on seed breaks on uid (still antisymmetric) ----------------------
	{
		const bool AB = FConversationFloor::IsLead(5, 5, 100, 200);
		const bool BA = FConversationFloor::IsLead(5, 5, 200, 100);
		TestNotEqual(TEXT("uid tie-break is antisymmetric"), AB, BA);
	}

	// --- Turn indexing --------------------------------------------------------
	{
		TestEqual(TEXT("turn 0 at t=0"), FConversationFloor::TurnAt(0.0, 3.0), 0);
		TestEqual(TEXT("turn 0 mid-first-interval"), FConversationFloor::TurnAt(2.9, 3.0), 0);
		TestEqual(TEXT("turn 1 after one interval"), FConversationFloor::TurnAt(3.1, 3.0), 1);
		TestEqual(TEXT("turn 3 after three intervals"), FConversationFloor::TurnAt(9.5, 3.0), 3);
		TestEqual(TEXT("negative elapsed -> turn 0"), FConversationFloor::TurnAt(-5.0, 3.0), 0);
		TestEqual(TEXT("zero interval guarded -> turn 0"), FConversationFloor::TurnAt(5.0, 0.0), 0);
	}

	// --- Exactly one participant holds the floor each turn --------------------
	{
		bool bAlwaysExactlyOne = true;
		bool bAlternates = true;
		for (int32 Turn = 0; Turn < 20; ++Turn)
		{
			const bool LeadHolds = FConversationFloor::HoldsFloor(true, Turn);
			const bool FollowerHolds = FConversationFloor::HoldsFloor(false, Turn);
			if (LeadHolds == FollowerHolds) { bAlwaysExactlyOne = false; } // both or neither
			// Lead holds even turns.
			if (LeadHolds != ((Turn % 2) == 0)) { bAlternates = false; }
		}
		TestTrue(TEXT("exactly one holds the floor each turn"), bAlwaysExactlyOne);
		TestTrue(TEXT("lead speaks even turns, follower odd"), bAlternates);
	}

	// --- Negative turn is treated as turn 0 -----------------------------------
	{
		TestEqual(TEXT("negative turn == turn 0 for lead"),
			FConversationFloor::HoldsFloor(true, -3), FConversationFloor::HoldsFloor(true, 0));
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
