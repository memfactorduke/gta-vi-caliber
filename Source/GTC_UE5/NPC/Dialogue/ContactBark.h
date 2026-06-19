// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../Decision/NpcContact.h"

/**
 * What a citizen actually says when the player is right on top of them — the
 * voice of the close-range contact layer (FNpcContact) and of a near pass-by.
 * Before this, contact reactions borrowed the generic ALARMED / FLEE / see_player
 * banks, so a shove and a gunshot drew the same "What was that?!"; these lines are
 * specific to being brushed, bumped, shoved, squared-up-to, scared, or floored, so
 * the moment reads as a real person reacting to YOU.
 *
 * PURE-CORE: scene-free, deterministic line selection by index (Godot-style
 * posmod wrap, mirroring FBarkPool), unit-tested headless
 * (Tests/ContactBarkTest.cpp, prefix GTC.NPC.Dialogue.ContactBark). GTC-original
 * content (not a Godot parity port), so the banks are free to grow. Voice-flavoured
 * variants are a later concern; these are characterful but voice-agnostic.
 */
struct GTC_UE5_API FContactBark
{
    /**
     * A voice-flavoured line for being touched, by archetype voice + reaction kind +
     * an index (counter/seed, posmod-wrapped). `Voice` is the archetype voice token
     * ("doomsday"/"influencer"/"conspiracy"/"mime"/...); a voice that hasn't authored
     * this reaction — or an empty/unknown voice — falls back to a generic bank, so a
     * doomsday prepper and an influencer snap at you differently while everyone else
     * still has something to say. Returns "" for Ignore / unknown.
     */
    static FString Line(const FString& Voice, ENpcContactReaction Reaction, int32 Index);

    /** Number of lines for (voice, reaction) — the voice's own bank if it has one,
     *  else the generic bank (0 for Ignore). */
    static int32 Count(const FString& Voice, ENpcContactReaction Reaction);

    /**
     * A light acknowledgement for the player passing close by without contact — a
     * greeting, a mutter, a "watch yourself". Index posmod-wraps. Never empty.
     */
    static FString PassingLine(int32 Index);

    /** Number of pass-by acknowledgement lines. */
    static int32 PassingCount();
};
