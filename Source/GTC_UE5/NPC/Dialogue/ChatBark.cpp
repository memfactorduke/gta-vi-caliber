// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChatBark.h"

namespace
{
    // Godot posmod(index, n): always non-negative in [0, n).
    int32 Wrap(int32 Index, int32 N)
    {
        return ((Index % N) + N) % N;
    }

    // Generic small talk — what anyone might say. Used directly, and as the fallback
    // for any voice without its own bank.
    const TArray<FString>& GenericOpeners()
    {
        static const TArray<FString> O = {
            TEXT("How's your day going?"),
            TEXT("Some weather we're having."),
            TEXT("Did you catch the game?"),
            TEXT("You come here often?"),
            TEXT("Long line today, huh?"),
            TEXT("Crazy out here lately."),
        };
        return O;
    }
    const TArray<FString>& GenericReplies()
    {
        static const TArray<FString> R = {
            TEXT("Tell me about it."),
            TEXT("Right? Same here."),
            TEXT("Can't complain."),
            TEXT("Oh, totally."),
            TEXT("Ha, you said it."),
            TEXT("Hadn't thought of it like that."),
        };
        return R;
    }

    // voice -> openers / replies, for the personalities worth flavouring.
    const TMap<FString, TArray<FString>>& VoiceOpeners()
    {
        static const TMap<FString, TArray<FString>> M = {
            { TEXT("conspiracy"), { TEXT("You ever notice the pigeons recharging?"),
                                    TEXT("They changed the relish formula again."),
                                    TEXT("Don't look now, but that lamppost is new.") }},
            { TEXT("influencer"), { TEXT("Okay you HAVE to see my feed."),
                                    TEXT("I'm basically verified now."),
                                    TEXT("Is the lighting better over here?") }},
            { TEXT("doomsday"),   { TEXT("The foam spelled a warning today."),
                                    TEXT("Enjoy the sun while it lasts."),
                                    TEXT("I've read the espresso. It's grim.") }},
            { TEXT("yogi"),       { TEXT("You're holding tension in your shoulders."),
                                    TEXT("Have you tried just... breathing?") }},
            { TEXT("weather"),    { TEXT("Forecast looks grim, between us."),
                                    TEXT("Cold front moving in, mark my words.") }},
            { TEXT("food_critic"),{ TEXT("Don't get the hot dog here. Trust me."),
                                    TEXT("This bench? Two stars.") }},
        };
        return M;
    }
    const TMap<FString, TArray<FString>>& VoiceReplies()
    {
        static const TMap<FString, TArray<FString>> M = {
            { TEXT("conspiracy"), { TEXT("That tracks, honestly."), TEXT("Wake up, is all I say.") }},
            { TEXT("influencer"), { TEXT("Slay. Anyway — me."), TEXT("Did you film that? No? Ugh.") }},
            { TEXT("doomsday"),   { TEXT("It's all connected, isn't it."), TEXT("The end is a medium roast.") }},
            { TEXT("yogi"),       { TEXT("Mmm. Let it go."), TEXT("Namaste to that.") }},
            { TEXT("weather"),    { TEXT("And back to you in studio."), TEXT("Doppler agrees.") }},
            { TEXT("food_critic"),{ TEXT("Overrated. Like everything."), TEXT("Needs salt. It all does.") }},
        };
        return M;
    }

    const TArray<FString>& ResolveOpeners(const FString& Voice)
    {
        if (const TArray<FString>* V = VoiceOpeners().Find(Voice))
        {
            if (V->Num() > 0) { return *V; }
        }
        return GenericOpeners();
    }
    const TArray<FString>& ResolveReplies(const FString& Voice)
    {
        if (const TArray<FString>* V = VoiceReplies().Find(Voice))
        {
            if (V->Num() > 0) { return *V; }
        }
        return GenericReplies();
    }
}

FString FChatBark::Opener(const FString& Voice, int32 Index)
{
    const TArray<FString>& Bank = ResolveOpeners(Voice);
    return Bank[Wrap(Index, Bank.Num())];
}

FString FChatBark::Reply(const FString& Voice, int32 Index)
{
    const TArray<FString>& Bank = ResolveReplies(Voice);
    return Bank[Wrap(Index, Bank.Num())];
}

int32 FChatBark::OpenerCount(const FString& Voice)
{
    return ResolveOpeners(Voice).Num();
}

int32 FChatBark::ReplyCount(const FString& Voice)
{
    return ResolveReplies(Voice).Num();
}

bool FChatBark::IsOpener(int32 SelfSeed, int32 PartnerSeed, uint32 SelfTiebreak, uint32 PartnerTiebreak)
{
    // Lower seed opens; equal seeds break on the stable per-actor tiebreak. Both
    // sides call with swapped arguments, so the result is antisymmetric — exactly
    // one of the pair is the opener (the only way both agree to take opposite turns).
    if (SelfSeed != PartnerSeed)
    {
        return SelfSeed < PartnerSeed;
    }
    return SelfTiebreak < PartnerTiebreak;
}
