// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Who gets to talk RIGHT NOW in a two-person chat — the "conversational floor".
 * Two citizens chat by ticking independently (no shared object); before this,
 * each spoke on its own timer, so a pair could blurt over each other or both
 * fall silent. This arbiter gives the exchange clean turn-taking: time is sliced
 * into turns, the pair's "lead" speaker takes the even turns and the other takes
 * the odd ones, so exactly one of them holds the floor per turn — and because
 * both sides compute the same answer from the same two identities, they agree
 * without any shared state or messaging.
 *
 * PURE-CORE: scene-free, deterministic, no engine coupling. Unit-tested headless
 * (Tests/ConversationFloorTest.cpp, prefix GTC.NPC.Conversation.Floor). The
 * AGTCCitizen chat tick asks HoldsFloor() each beat and only voices a line when
 * it's its turn, so the back-and-forth reads as a real conversation.
 */
struct GTC_UE5_API FConversationFloor
{
	/**
	 * Which of the two is the "lead" (takes the even turns). Decided purely from
	 * the two identities so it's stable and antisymmetric: IsLead(A,B) is always
	 * the opposite of IsLead(B,A) for distinct participants. Lower seed leads;
	 * ties break on the unique ids (then a final coin so identical inputs still
	 * resolve), matching FChatBark::IsOpener's intent.
	 */
	static bool IsLead(int32 SelfSeed, int32 PartnerSeed, uint32 SelfUid, uint32 PartnerUid);

	/** The 0-based turn index at `ElapsedSec` into the chat, given the per-turn
	 *  cadence. Never negative; returns 0 for a non-positive interval. */
	static int32 TurnAt(double ElapsedSec, double IntervalSec);

	/**
	 * Does the lead (bSelfIsLead) hold the floor on `TurnIndex`? The lead speaks
	 * even turns, the follower odd turns, so for any turn exactly one of the pair
	 * holds it. Negative turns are treated as turn 0.
	 */
	static bool HoldsFloor(bool bSelfIsLead, int32 TurnIndex);
};
