// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Two-sided small talk for NPC-to-NPC conversations. The crowd already pairs up
 * loitering citizens and turns them to face each other; today they each just
 * recite generic "socialize" barks, so it reads as two people talking AT the air.
 * This gives the exchange a shape: the initiator throws an OPENER (a topic — the
 * weather, the game, the suspicious pigeons), and the partner throws a REPLY (a
 * reaction — agreement, a deflection, their own spin). Alternating the two across
 * turns makes a knot of citizens sound like an actual back-and-forth.
 *
 * PURE-CORE: scene-free, deterministic FString selection by archetype voice + an
 * index (Godot-style posmod wrap), with a generic fallback bank for any voice that
 * hasn't authored its own lines — exactly the FContactBark pattern. No engine
 * coupling; unit-tested headless (Tests/ChatBarkTest.cpp, prefix
 * GTC.NPC.Dialogue.ChatBark). The AGTCCitizen chat loop is the (deferred) consumer:
 * the initiator speaks Opener(), the partner speaks Reply(), each on its turn.
 */
struct GTC_UE5_API FChatBark
{
    /** A conversation-opening line in Voice (starts a topic). Generic fallback;
     *  never empty. Index posmod-wraps. */
    static FString Opener(const FString& Voice, int32 Index);

    /** A responding line in Voice (reacts to the other person). Generic fallback;
     *  never empty. Index posmod-wraps. */
    static FString Reply(const FString& Voice, int32 Index);

    /** Line counts for (voice) — the voice's own bank if it has one, else generic. */
    static int32 OpenerCount(const FString& Voice);
    static int32 ReplyCount(const FString& Voice);

    /**
     * Which of a paired-up couple opens the conversation (the other replies). A
     * TOTAL, ANTISYMMETRIC rule so both sides — each calling with its own view —
     * always pick OPPOSITE roles: the lower seed opens; ties (equal seeds, which are
     * reachable for hand-placed or seed-overlapping citizens) break on a stable
     * per-actor discriminator (`Tiebreak`, e.g. the actor's unique id) so even then
     * exactly one opens. Pure: the adapter supplies seeds + tiebreaks.
     */
    static bool IsOpener(int32 SelfSeed, int32 PartnerSeed, uint32 SelfTiebreak, uint32 PartnerTiebreak);
};
