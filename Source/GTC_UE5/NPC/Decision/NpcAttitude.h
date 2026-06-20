// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A citizen's TEMPER — the personality dimension that decides whether someone
 * shrugs off a shove or gets in your face about it. Most people are easygoing;
 * a minority are surly, and a rare few are hotheads spoiling for it. Layered on
 * top of the existing Bravery/Curiosity traits, temper is what turns "an NPC got
 * bumped" into "that guy just told you where to go" — the spoken, confrontational
 * half of street life ("talk shit", muttered insults, squaring up).
 *
 * The temper is a fixed part of identity (seeded), so the same person always
 * reacts in character. A provocation (a bump, a honk, a near-miss, a stare, or
 * just spotting an irritating passer-by) is mapped — together with how brave the
 * citizen is — to a VERBAL HEAT level the dialogue layer (FInsultBark) speaks at:
 * a muttered grumble, a direct insult, an R-rated curse, or full fighting words.
 *
 * PURE-CORE: scene-free, deterministic selection from seed + traits (Godot-style
 * posmod wrap, mirroring FNpcOccupation / FBarkPool), no engine coupling.
 * Unit-tested headless (Tests/NpcAttitudeTest.cpp, prefix
 * GTC.NPC.Decision.NpcAttitude). The adapter (AGTCCitizen) stamps TemperFor at
 * identity time and calls ProvocationResponse when something sets the citizen off,
 * then voices the result through FInsultBark — gameplay choice testable here,
 * lines are data.
 */

/** How hot a citizen runs. Ordered Meek < Easygoing < Surly < Hothead. */
enum class ENpcTemper : uint8
{
    Meek,       // avoids confrontation; mutters at worst
    Easygoing,  // the default; a grumble if pushed
    Surly,      // quick to insult
    Hothead,    // looking for a reason; curses, squares up
};

/** What set the citizen off. */
enum class ENpcProvocation : uint8
{
    Bumped,      // the player barged into them
    Honked,      // a horn blared right behind them
    NearMiss,    // a car nearly clipped them at the kerb
    Stared,      // the player is crowding / eyeballing them
    PeerPassing, // an irritating passer-by worth a muttered snipe (no contact)
};

/** How loudly the citizen answers. Ordered None < Mutter < Insult < Curse < SquareUp.
 *  Doubles as the bank key for FInsultBark (None has no bank — silence). */
enum class ENpcVerbalHeat : uint8
{
    None,      // says nothing
    Mutter,    // under-the-breath grumble
    Insult,    // a direct, pointed insult
    Curse,     // R-rated profanity
    SquareUp,  // fighting words — only the brave-and-furious get here
};

struct GTC_UE5_API FNpcAttitude
{
    /**
     * This citizen's fixed temper, drawn deterministically from its spawn Seed and
     * archetype id (so the same person is always the same, and different archetypes
     * with the same slot differ). Most results are Easygoing; Surly/Hothead are the
     * minority that make a crowd spicy.
     */
    static ENpcTemper TemperFor(int32 Seed, const FString& ArchetypeId);

    /** True for tempers that pick fights with words (Surly or Hothead). */
    static bool IsRude(ENpcTemper Temper);

    /**
     * The verbal heat a citizen of this Temper answers a Provocation with, given how
     * brave they are (0..1) and a Seed for in-character jitter. Deterministic in all
     * inputs. SquareUp is reserved for rude tempers that are also brave; meek citizens
     * never get past a muttered insult. Returns None when the citizen lets it slide.
     */
    static ENpcVerbalHeat ProvocationResponse(
        ENpcTemper Temper, ENpcProvocation Provocation, double Bravery, int32 Seed);
};
