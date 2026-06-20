// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "../Decision/NpcRoutine.h"
#include "GTCRoutineSubsystem.generated.h"

/**
 * UGTCRoutineSubsystem — the live store + editor for individual NPC daily routines.
 * This is the "routine editor" runtime: it holds a bank of FNpcRoutine, loads/saves
 * them as human-and-AI editable .routine.json files, assigns routines to individual
 * citizens (by seed/stable id), and serves the chosen routine to AGTCCitizen so two
 * people of the same archetype can live genuinely different days.
 *
 * Editable three ways, all hitting the same bank:
 *   - HUMAN, in-game: `GTC.Routine.*` console commands (List/New/AddBlock/Save/...).
 *   - HUMAN/AI, on disk: edit the .routine.json files under Saved/Routines in any
 *     text editor, then
 *     `GTC.Routine.Reload`. The format is tiny and self-describing.
 *   - AI/MCP or Blueprint: the BlueprintCallable UFUNCTIONs below (id/string based, so
 *     no reflected struct is needed), callable from an MCP exec bridge or a BP.
 *
 * Mirrors the proven UGtcMissionEditorSubsystem pattern (UWorldSubsystem + console
 * statics + GtcJson). The pure logic lives in FNpcRoutine/FNpcRoutineLibrary +
 * NpcRoutineJson (headless-tested); this adapter is the thin live shell.
 */
UCLASS()
class GTC_UE5_API UGTCRoutineSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    // --- Live routine resolution (what AGTCCitizen calls) ----------------------

    /** The routine assigned to a citizen seed/stable id, falling back to a stable
     *  seed-picked routine from the bank. Returns nullptr only when the bank is empty
     *  (the citizen then keeps its archetype schedule). */
    const FNpcRoutine* ResolveRoutineForSeed(int32 Seed) const;

    /** Copy the resolved routine's blocks for a seed into Out. False (Out untouched)
     *  if the bank is empty — caller keeps its archetype default. */
    bool ResolveBlocksForSeed(int32 Seed, TArray<FNpcScheduleBlock>& Out) const;

    /** The whole live bank (for the editor / debug). */
    const TArray<FNpcRoutine>& GetRoutines() const { return Routines; }

    // --- Editing (Blueprint / MCP / console all funnel here) -------------------

    /** Create an empty routine (or return false if the id already exists). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Routine")
    bool CreateRoutine(const FString& Id, const FString& DisplayName);

    /** Append a [Start,End) block. Creates the routine if it doesn't exist. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Routine")
    bool AddBlock(const FString& Id, float StartHour, float EndHour, const FString& Activity, const FString& Place);

    /** Remove the block at Index from a routine. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Routine")
    bool RemoveBlock(const FString& Id, int32 Index);

    /** Delete a routine from the bank. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Routine")
    bool DeleteRoutine(const FString& Id);

    /** Assign a routine id to a citizen seed/stable id (live citizens hot-pick it). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Routine")
    bool AssignRoutineToSeed(int32 Seed, const FString& RoutineId);

    /** The id assigned to a seed, or empty. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Routine")
    FString GetAssignedRoutineId(int32 Seed) const;

    // --- Persistence -----------------------------------------------------------

    /** Save one routine to AbsolutePath (or its default Saved/Routines path if empty). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Routine")
    bool SaveRoutine(const FString& Id, const FString& AbsolutePath);

    /** Load a single .routine.json into the bank (replacing any same-id entry). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Routine")
    bool LoadRoutineFile(const FString& AbsolutePath);

    /** Save the whole bank to AbsolutePath (or the default bank path if empty). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Routine")
    bool SaveBank(const FString& AbsolutePath);

    /** Replace the live bank from a bank file (or the default bank path if empty). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Routine")
    bool LoadBank(const FString& AbsolutePath);

    /** Reload the default bank file from disk — the hot-reload after a hand/AI edit. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Routine")
    bool ReloadDefaultBank();

private:
    /** Mutable lookup into the bank (CRUD). */
    FNpcRoutine* FindMutable(const FString& Id);

    /** The live bank of individual routines. Plain (the element type is pure-core,
     *  non-reflected); the editor surface is the UFUNCTIONs + console, not a panel. */
    TArray<FNpcRoutine> Routines;

    /** Per-citizen assignment: seed/stable id -> routine id. */
    TMap<int32, FString> Assignments;
};
