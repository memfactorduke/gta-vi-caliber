// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../Decision/NpcRoutine.h"

/**
 * JSON (de)serialization for individual NPC routines — the human-and-AI editable
 * file format behind the routine editor. A routine round-trips to a small, hand-
 * editable document so a person can tweak it in a text editor and an AI/MCP can
 * read and rewrite it as plain text:
 *
 *   { "format": "gtc_routine", "version": 1,
 *     "routine": { "id": "barista_early_riser", "name": "Early-Riser Barista",
 *       "blocks": [ { "start": 6.0, "end": 9.0, "activity": "goof_off", "place": "gym" }, ... ] } }
 *
 * A bank file ("gtc_routine_bank") holds many routines under "routines". Both carry
 * a tiny envelope (format + version) so routine files migrate independently of the
 * savegame, exactly like .mission.json.
 *
 * Reuses the in-module ordered JSON wrapper (Systems/Save/SaveJson.h, GtcJson) — no
 * new Build.cs dependency. Parse is silent + safe on malformed input (returns false,
 * never a half-built routine). Files default under ProjectSavedDir/Routines/.
 *
 * PURE codec (scene-free), unit-tested headless (Tests/NpcRoutineJsonTest.cpp,
 * prefix GTC.NPC.Routine.NpcRoutineJson).
 */
namespace NpcRoutineJson
{
    /** Bumped only on a breaking field change; routine files migrate on their own. */
    constexpr int32 FormatVersion = 1;

    /** The default on-disk location for a single routine: SavedDir/Routines/<id>.routine.json */
    GTC_UE5_API FString DefaultPathFor(const FString& RoutineId);

    /** The default bank location: SavedDir/Routines/routines.bank.json */
    GTC_UE5_API FString DefaultBankPath();

    // --- single routine --------------------------------------------------------
    GTC_UE5_API FString RoutineToJson(const FNpcRoutine& Routine);
    /** False (and Out reset) on malformed input or wrong/absent format tag. */
    GTC_UE5_API bool JsonToRoutine(const FString& Text, FNpcRoutine& Out);
    GTC_UE5_API bool SaveRoutineToFile(const FNpcRoutine& Routine, const FString& AbsolutePath);
    GTC_UE5_API bool LoadRoutineFromFile(const FString& AbsolutePath, FNpcRoutine& Out);

    // --- a whole bank ----------------------------------------------------------
    GTC_UE5_API FString BankToJson(const TArray<FNpcRoutine>& Routines);
    /** False on malformed input; partial/invalid entries are skipped, not fatal. */
    GTC_UE5_API bool JsonToBank(const FString& Text, TArray<FNpcRoutine>& Out);
    GTC_UE5_API bool SaveBankToFile(const TArray<FNpcRoutine>& Routines, const FString& AbsolutePath);
    GTC_UE5_API bool LoadBankFromFile(const FString& AbsolutePath, TArray<FNpcRoutine>& Out);
}
