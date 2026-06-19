// Copyright Epic Games, Inc. All Rights Reserved.

#include "ConversationFloor.h"

bool FConversationFloor::IsLead(int32 SelfSeed, int32 PartnerSeed, uint32 SelfUid, uint32 PartnerUid)
{
	// Lower seed leads. This is the common case and is trivially antisymmetric.
	if (SelfSeed != PartnerSeed)
	{
		return SelfSeed < PartnerSeed;
	}
	// Same seed: break on the (always-distinct in practice) unique ids.
	if (SelfUid != PartnerUid)
	{
		return SelfUid < PartnerUid;
	}
	// Degenerate identical inputs (only in tests): pick deterministically so the
	// function still returns, even though two real citizens never collide here.
	return true;
}

int32 FConversationFloor::TurnAt(double ElapsedSec, double IntervalSec)
{
	if (IntervalSec <= 0.0 || ElapsedSec <= 0.0)
	{
		return 0;
	}
	return static_cast<int32>(ElapsedSec / IntervalSec);
}

bool FConversationFloor::HoldsFloor(bool bSelfIsLead, int32 TurnIndex)
{
	const int32 Turn = TurnIndex < 0 ? 0 : TurnIndex;
	const bool bEvenTurn = (Turn % 2) == 0;
	// Lead speaks even turns; follower speaks odd turns. Exactly one is true.
	return bSelfIsLead == bEvenTurn;
}
