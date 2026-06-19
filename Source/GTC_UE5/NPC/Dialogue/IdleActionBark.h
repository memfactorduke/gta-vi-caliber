// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The little things people mutter while they kill time. Pairs the contextual idle
 * actions from FNpcIdleAction with a quiet, under-the-breath line: someone checking
 * their phone grumbles about the signal, someone checking the time is "so late",
 * a people-watcher passes judgement on a passer-by. Not every action talks — a
 * smoker or a leaner is silent — so the street murmurs occasionally rather than
 * narrating itself.
 *
 * PURE-CORE: scene-free, deterministic FString selection by action token + index
 * (Godot-style posmod wrap, mirroring FBarkPool / FContactBark), no engine coupling.
 * Unit-tested headless (Tests/IdleActionBarkTest.cpp, prefix
 * GTC.NPC.Dialogue.IdleActionBark). Keyed by the same FName tokens FNpcIdleAction
 * produces; an action with no bank returns "" (silent).
 */
struct GTC_UE5_API FIdleActionBark
{
    /**
     * A quiet line for the given idle-action token (e.g. "check_phone",
     * "check_watch", "people_watch"), chosen deterministically from Index
     * (posmod-wrapped). Returns "" for a silent action or an unknown token.
     */
    static FString Line(FName Action, int32 Index);

    /** Number of lines for an action token (0 if the action is silent). */
    static int32 Count(FName Action);
};
