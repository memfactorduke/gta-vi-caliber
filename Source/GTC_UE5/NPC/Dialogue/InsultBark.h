// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../Decision/NpcAttitude.h"

/**
 * What a citizen SAYS when their temper is up — the spoken voice of FNpcAttitude.
 * The attitude layer decides how hot a citizen runs and, on a provocation, picks a
 * verbal heat (a muttered grumble, a pointed insult, an R-rated curse, or fighting
 * words); this bank supplies the actual line at that heat, so a barged-into hothead
 * tells you off in character instead of borrowing the polite "excuse me" bank.
 *
 * A separate "peer snipe" channel is the muttered talk-shit a surly citizen aims at
 * an irritating passer-by — gossip, not a confrontation, so it lives apart from the
 * heat banks.
 *
 * PURE-CORE: scene-free, deterministic line selection by index (Godot-style posmod
 * wrap, mirroring FContactBark), unit-tested headless (Tests/InsultBarkTest.cpp,
 * prefix GTC.NPC.Dialogue.InsultBark). GTC-original content (R-rated by design, no
 * slurs, no commercial-game lines), so the banks are free to grow. Voice-flavoured
 * variants fall back to a generic bank for that heat, exactly like FContactBark.
 */
struct GTC_UE5_API FInsultBark
{
    /**
     * A voice-flavoured line at the given verbal Heat, by archetype voice token +
     * index (counter/seed, posmod-wrapped). A voice that hasn't authored this heat —
     * or an empty/unknown voice — falls back to the generic bank, so a conspiracy
     * vendor and an influencer curse you out differently while everyone still has
     * something to say. Returns "" for ENpcVerbalHeat::None.
     */
    static FString Line(const FString& Voice, ENpcVerbalHeat Heat, int32 Index);

    /** Number of lines for (voice, heat) — the voice's own bank if it has one, else
     *  the generic bank (0 for None). */
    static int32 Count(const FString& Voice, ENpcVerbalHeat Heat);

    /**
     * A muttered snipe at an irritating passer-by (the PeerPassing provocation) — talk
     * shit under the breath, not a confrontation. Index posmod-wraps; never empty.
     */
    static FString PeerSnipe(int32 Index);

    /** Number of peer-snipe lines. */
    static int32 PeerSnipeCount();
};
