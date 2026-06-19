// Copyright Epic Games, Inc. All Rights Reserved.

#include "IdleActionBark.h"

namespace
{
    // action token -> mutters (ordered; index selection is observable). Only the
    // "talkative" actions have banks; the rest (smoke, lean, stretch, nod, ...) are
    // intentionally silent and absent here.
    const TMap<FName, TArray<FString>>& IdleBarkBanks()
    {
        static const TMap<FName, TArray<FString>> B = {
            { TEXT("check_phone"), {
                TEXT("Ugh, no signal."),
                TEXT("Three percent. Perfect."),
                TEXT("Who even texts anymore?"),
                TEXT("Just one more video..."),
            }},
            { TEXT("check_watch"), {
                TEXT("Come on, come on."),
                TEXT("I'm so late."),
                TEXT("Any minute now..."),
                TEXT("Where IS it?"),
            }},
            { TEXT("people_watch"), {
                TEXT("Now there's a character."),
                TEXT("Huh. Bold choice."),
                TEXT("This city, I swear."),
            }},
            { TEXT("sigh"), {
                TEXT("Long day."),
                TEXT("Hm."),
                TEXT("One of those days."),
            }},
            { TEXT("savor"), {
                TEXT("Mm. That's good."),
                TEXT("Worth every penny."),
            }},
            { TEXT("sip_drink"), {
                TEXT("Ahh."),
                TEXT("Needed that."),
            }},
            { TEXT("laugh"), {
                TEXT("Ha! No way."),
                TEXT("Stop it, you."),
            }},
            { TEXT("check_reflection"), {
                TEXT("Lookin' good."),
                TEXT("There we go."),
            }},
        };
        return B;
    }
}

FString FIdleActionBark::Line(FName Action, int32 Index)
{
    const TArray<FString>* Pool = IdleBarkBanks().Find(Action);
    if (!Pool || Pool->Num() == 0)
    {
        return FString(); // a silent action — nothing to mutter
    }
    // Godot posmod(Index, n): always non-negative in [0, n).
    const int32 N = Pool->Num();
    return (*Pool)[((Index % N) + N) % N];
}

int32 FIdleActionBark::Count(FName Action)
{
    const TArray<FString>* Pool = IdleBarkBanks().Find(Action);
    return Pool ? Pool->Num() : 0;
}
