// Copyright Epic Games, Inc. All Rights Reserved.

#include "InsultBark.h"

namespace
{
    // Generic heat -> lines, escalating from a muttered grumble to fighting words.
    // GTC-original, voice-agnostic, R-rated by design (no slurs). None has no bank.
    const TMap<ENpcVerbalHeat, TArray<FString>>& GtcInsultHeatLines()
    {
        static const TMap<ENpcVerbalHeat, TArray<FString>> L = {
            { ENpcVerbalHeat::Mutter, {
                TEXT("...unbelievable."),
                TEXT("Some people, I swear."),
                TEXT("Watch it, genius."),
                TEXT("...what a tool."),
                TEXT("Yeah, keep walkin'."),
                TEXT("Real smooth, pal."),
            }},
            { ENpcVerbalHeat::Insult, {
                TEXT("Hey! You blind or just stupid?"),
                TEXT("Move it, jackass!"),
                TEXT("Nice walkin', dipshit!"),
                TEXT("What is your problem, huh?"),
                TEXT("Get your head outta your ass!"),
                TEXT("Use your damn eyes!"),
            }},
            { ENpcVerbalHeat::Curse, {
                TEXT("Watch where you're goin', you absolute asshole!"),
                TEXT("The hell is wrong with you?!"),
                TEXT("Piss off and learn to walk!"),
                TEXT("Move it, you blind son of a bitch!"),
                TEXT("Goddamn it — watch it!"),
                TEXT("Get bent, jackass!"),
            }},
            { ENpcVerbalHeat::SquareUp, {
                TEXT("You wanna go?! Right here, right now!"),
                TEXT("Say that to my face, tough guy!"),
                TEXT("Touch me again, I dare you!"),
                TEXT("I'll knock you flat, you hear me?!"),
                TEXT("Come on then! Let's see what you got!"),
                TEXT("Back off before I make you!"),
            }},
        };
        return L;
    }

    // Muttered talk-shit aimed at a passer-by (gossip, not confrontation).
    const TArray<FString>& GtcInsultPeerSnipeLines()
    {
        static const TArray<FString> P = {
            TEXT("...get a load of this guy."),
            TEXT("Who dresses like that?"),
            TEXT("...struttin' like he owns the block."),
            TEXT("Look at this clown."),
            TEXT("What is he, lost?"),
            TEXT("Pfft. Tourists."),
        };
        return P;
    }

    // voice -> heat -> lines: personality where it pops (an insult, a curse, a brawl).
    // Heats a voice doesn't author fall back to the generic bank above.
    const TMap<FString, TMap<ENpcVerbalHeat, TArray<FString>>>& GtcInsultVoiceHeatLines()
    {
        static const TMap<FString, TMap<ENpcVerbalHeat, TArray<FString>>> V = {
            { TEXT("doomsday"), {
                { ENpcVerbalHeat::Insult, {
                    TEXT("The grounds SHOWED me you'd do that, fool!"),
                    TEXT("Walk like that and you deserve what's coming!"),
                }},
                { ENpcVerbalHeat::Curse, {
                    TEXT("You damn fool, the end's near and you're shovin' ME?!"),
                    TEXT("To hell with you AND the tide!"),
                }},
            }},
            { TEXT("influencer"), {
                { ENpcVerbalHeat::Insult, {
                    TEXT("Do you KNOW how many followers I have, idiot?"),
                    TEXT("Ugh, you almost ruined my shot, loser!"),
                }},
                { ENpcVerbalHeat::Curse, {
                    TEXT("I'm posting your dumbass face EVERYWHERE!"),
                    TEXT("The hell?! That's a lawsuit, jackass!"),
                }},
            }},
            { TEXT("conspiracy"), {
                { ENpcVerbalHeat::Insult, {
                    TEXT("That was DELIBERATE! Who's payin' you?!"),
                    TEXT("Government stooge! Watch it!"),
                }},
                { ENpcVerbalHeat::Curse, {
                    TEXT("The pigeons saw that, you son of a bitch!"),
                    TEXT("I KNEW they'd send some asshole like you!"),
                }},
            }},
            { TEXT("food_critic"), {
                { ENpcVerbalHeat::Insult, {
                    TEXT("Rude AND poorly dressed. One star."),
                    TEXT("The audacity. Absolutely unseasoned."),
                }},
                { ENpcVerbalHeat::Curse, {
                    TEXT("I'll end your whole damn Yelp existence!"),
                    TEXT("Get out of my way, you bland little hack!"),
                }},
            }},
        };
        return V;
    }

    // Godot posmod(index, n): always non-negative in [0, n).
    int32 GtcInsultWrap(int32 Index, int32 N)
    {
        return N <= 0 ? 0 : ((Index % N) + N) % N;
    }

    // Pick the bank for (voice, heat): the voice's own if it authored one, else generic.
    const TArray<FString>* GtcInsultResolveBank(const FString& Voice, ENpcVerbalHeat Heat)
    {
        if (const TMap<ENpcVerbalHeat, TArray<FString>>* ByHeat = GtcInsultVoiceHeatLines().Find(Voice))
        {
            if (const TArray<FString>* Voiced = ByHeat->Find(Heat))
            {
                if (Voiced->Num() > 0)
                {
                    return Voiced;
                }
            }
        }
        return GtcInsultHeatLines().Find(Heat);
    }
}

FString FInsultBark::Line(const FString& Voice, ENpcVerbalHeat Heat, int32 Index)
{
    const TArray<FString>* Pool = GtcInsultResolveBank(Voice, Heat);
    if (!Pool || Pool->Num() == 0)
    {
        return FString(); // None / unknown — nothing to say
    }
    return (*Pool)[GtcInsultWrap(Index, Pool->Num())];
}

int32 FInsultBark::Count(const FString& Voice, ENpcVerbalHeat Heat)
{
    const TArray<FString>* Pool = GtcInsultResolveBank(Voice, Heat);
    return Pool ? Pool->Num() : 0;
}

FString FInsultBark::PeerSnipe(int32 Index)
{
    const TArray<FString>& P = GtcInsultPeerSnipeLines();
    return P[GtcInsultWrap(Index, P.Num())];
}

int32 FInsultBark::PeerSnipeCount()
{
    return GtcInsultPeerSnipeLines().Num();
}
