// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * What two citizens are actually talking ABOUT. FChatBark gave conversations a
 * shape (opener then reply) but each line was picked independently, so a reply
 * needn't relate to the opener — "Think it'll rain?" could be answered with
 * "Their fries, though." This pins one shared TOPIC per pair so the exchange
 * coheres: both sides derive the same topic from their two seeds (symmetric, no
 * shared state), the opener raises it and the reply answers THAT topic.
 *
 * Topics are everyday small talk — the weather, the city, food, gossip, the
 * suspicious pigeons, work — so a passing conversation sounds like one, not two
 * non-sequiturs. Lines are voice-agnostic but characterful; an unknown topic
 * falls back to generic small talk so the result is never empty.
 *
 * PURE-CORE: scene-free, deterministic FString selection by topic + index
 * (Godot-style posmod wrap, mirroring FChatBark / FBarkPool), no engine coupling.
 * Unit-tested headless (Tests/NpcTopicTest.cpp, prefix GTC.NPC.Dialogue.NpcTopic).
 * The AGTCCitizen chat loop picks Shared() once per pairing, then voices
 * Line(topic, bReply=false) to open and Line(topic, bReply=true) to answer.
 * GTC-original content.
 */
enum class ENpcChatTopic : uint8
{
	SmallTalk, // generic fallback — also the never-empty safety net
	Weather,
	City,
	Food,
	Gossip,
	Pigeons,
	Work,
};

struct GTC_UE5_API FNpcTopic
{
	/**
	 * The topic two citizens land on, derived purely from their two seeds. SYMMETRIC
	 * — Shared(A,B) == Shared(B,A) — so both sides, computing independently, agree
	 * without messaging. Always a concrete topic (never SmallTalk), spread across the
	 * real topics so different pairs talk about different things.
	 */
	static ENpcChatTopic Shared(int32 SeedA, int32 SeedB);

	/**
	 * A line on `Topic`: the opener (bReply=false) raises it, the reply (bReply=true)
	 * answers it. Index posmod-wraps. An unknown topic falls back to small talk, so
	 * the result is never empty.
	 */
	static FString Line(ENpcChatTopic Topic, bool bReply, int32 Index);

	/** Number of opener (bReply=false) or reply (bReply=true) lines for a topic. */
	static int32 Count(ENpcChatTopic Topic, bool bReply);

	/** A short token for the topic (logging/debug), e.g. "weather". */
	static const TCHAR* Name(ENpcChatTopic Topic);
};
