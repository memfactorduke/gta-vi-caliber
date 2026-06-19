// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Two citizens talking — gloriously past each other. Zips two voices' "chat"
 * banks (NpcDialogue) into an alternating exchange.
 *
 * Direct PURE-CORE port of the the reference `NpcConversation` (RefCounted) at
 * `game/scripts/npc/npc_conversation.gd`. Pure + deterministic (same voices +
 * seed = same scene), unit-tested headless (Tests/NpcConversationTest.cpp,
 * prefix GTC.NPC.Conversation).
 */

/** One turn in an exchange: {speaker, text}. Speaker 0 = voice_a (even turns), 1 = voice_b (odd). */
struct GTC_UE5_API FNpcConversationTurn
{
	int32 Speaker = 0;
	FString Text;
};

struct GTC_UE5_API FNpcConversation
{
	/**
	 * Build an alternating exchange of `Turns` lines between two voices. Each
	 * entry's speaker 0 = VoiceA (even turns), 1 = VoiceB (odd turns). Turns is
	 * clamped to a minimum of 1.
	 */
	static TArray<FNpcConversationTurn> Exchange(const FString& VoiceA, const FString& VoiceB, int32 SeedValue, int32 Turns = 4);

	/** A single opening line in `Voice` — the "greet" bark. */
	static FString Greeting(const FString& Voice, int32 SeedValue);
};
