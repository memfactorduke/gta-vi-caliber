// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcIdleAction.h"

namespace
{
    // activity -> ordered idle actions (index selection is observable). Tokens are
    // the contract with the Anim Blueprint, so keep them stable once art binds them.
    const TMap<FString, TArray<FName>>& IdleActionBanks()
    {
        static const TMap<FString, TArray<FName>> B = {
            { TEXT("loiter"), {
                TEXT("check_phone"), TEXT("people_watch"), TEXT("smoke"),
                TEXT("stretch"), TEXT("lean"), TEXT("check_watch"),
            }},
            { TEXT("socialize"), {
                TEXT("laugh"), TEXT("gesture"), TEXT("nod"), TEXT("check_phone"),
            }},
            { TEXT("eat"), {
                TEXT("sip_drink"), TEXT("savor"), TEXT("check_phone"), TEXT("wipe_mouth"),
            }},
            { TEXT("commute"), {
                TEXT("check_watch"), TEXT("check_phone"), TEXT("sigh"), TEXT("shift_weight"),
            }},
            { TEXT("goof_off"), {
                TEXT("dance_a_little"), TEXT("kick_pebble"), TEXT("air_drum"), TEXT("stretch"),
            }},
            { TEXT("freshen_up"), {
                TEXT("preen"), TEXT("check_reflection"), TEXT("adjust_clothes"),
            }},
            { TEXT("work"), {
                TEXT("check_clipboard"), TEXT("type"), TEXT("stretch"), TEXT("rub_neck"),
            }},
        };
        return B;
    }

    // Fallback for any activity without a bespoke bank (incl. unknown/empty).
    const TArray<FName>& GenericIdleActions()
    {
        static const TArray<FName> G = {
            TEXT("idle_look"), TEXT("check_phone"), TEXT("shift_weight"), TEXT("stretch"),
        };
        return G;
    }

    const TArray<FName>& BankFor(const FString& Activity)
    {
        if (const TArray<FName>* Bank = IdleActionBanks().Find(Activity))
        {
            return *Bank;
        }
        return GenericIdleActions();
    }
}

FName FNpcIdleAction::Pick(const FString& Activity, int32 Seed)
{
    const TArray<FName>& Bank = BankFor(Activity);
    if (Bank.Num() == 0)
    {
        return NAME_None;
    }
    // Godot posmod(Seed, n): always non-negative in [0, n).
    const int32 N = Bank.Num();
    const int32 Index = ((Seed % N) + N) % N;
    return Bank[Index];
}

int32 FNpcIdleAction::Count(const FString& Activity)
{
    return BankFor(Activity).Num();
}
