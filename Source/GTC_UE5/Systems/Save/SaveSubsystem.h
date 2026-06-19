// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveJson.h"
#include "SaveSubsystem.generated.h"

/**
 * USaveSubsystem — the UE port of Godot `save_manager.gd` (class SaveManager, a Node).
 *
 * Owns the save snapshot (the pure GtcSaveData model) plus the file IO and coordinates
 * persistence through a *registration interface*: instead of the Godot version reaching
 * into the scene tree by group ("player", "wanted", "vehicles", ...), coordinators
 * register a named save/load hook pair here. On Save the subsystem walks every registered
 * hook in registration order, asks each to write its section into the snapshot, wraps it
 * via GtcSaveData::Encode (version 4 header) and writes user-equivalent text to disk. On
 * Load it reads + decodes + migrates the text, then walks each hook to restore from its
 * section. Discovery is by registration, so this stays streaming/Partition-ready and
 * needs no scene edits — the UE analogue of Godot's group-discovery design.
 *
 * Why a GameInstanceSubsystem (not a WorldSubsystem): the Godot SaveManager is a single
 * persistent Node that survives scene/level changes (quick-save/quick-load across the
 * streamed world); a GameInstanceSubsystem has exactly that lifetime — one per running
 * game, outliving individual worlds. Matches the APPROVED design (option 3).
 *
 * Binary USaveGame/FArchive is DEFERRED: persistence is JSON text (GtcSaveData) for now.
 *
 * --- ROUND-TRIP TOLERANCE CONTRACT (inherited from GtcSaveData) ---
 * Register->Save->Load round-trips are by value-within-tolerance / structural equality,
 * NOT byte-identity. Wave 1 producers feed FRandomStream / double values that are
 * deterministic-per-seed but not byte-identical to Godot; floats survive a round-trip
 * only within GtcTest::Eps. Integers/strings/bools/structure survive exactly. Hooks must
 * tolerate this on restore (compare floats with a tolerance, never assert byte-identity).
 *
 * Ownership/lifecycle:
 *  - The subsystem owns no game state of its own — it owns the snapshot wiring and file IO.
 *  - Registered hooks are NON-owning: the registrant owns the hook's captured state and is
 *    responsible for unregistering before it dies (Unregister / RAII at the call site).
 *    A hook firing into a dangling captured object is the registrant's contract to avoid;
 *    the subsystem only guarantees stable iteration order and that Deinitialize clears all
 *    hooks so none survive the GameInstance.
 */

/** Writes this coordinator's state into the shared snapshot under its section key. */
DECLARE_DELEGATE_OneParam(FSaveHook, const TSharedRef<FGtcJsonObject>& /*SectionOut*/);

/**
 * Restores this coordinator from its section. The section is the object stored under the
 * key (empty object when the save had nothing for it — treat as "no saved data").
 */
DECLARE_DELEGATE_OneParam(FLoadHook, const TSharedRef<FGtcJsonObject>& /*SectionIn*/);

UCLASS()
class GTC_UE5_API USaveSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // UGameInstanceSubsystem
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * Register a save/load hook pair under a section key. Re-registering the same key
     * replaces both hooks (and keeps its position). Returns false for an empty key.
     */
    bool RegisterHook(const FString& Section, const FSaveHook& OnSave, const FLoadHook& OnLoad);

    /** Drop a section's hooks. Returns true if a hook existed. */
    bool UnregisterHook(const FString& Section);

    /** Number of registered sections. */
    int32 HookCount() const;

    bool HasHook(const FString& Section) const;

    /** Section keys in registration order. */
    TArray<FString> Sections() const;

    /**
     * Build the snapshot from every registered save hook (in registration order) and
     * return the encoded, version-headed JSON text. Pure w.r.t. disk (no write).
     */
    FString BuildSaveText();

    /**
     * Decode + migrate save text, then drive every registered load hook from its section.
     * An empty/garbage decode restores nothing (the Godot "empty == no saved data" guard),
     * so a corrupt save cannot wipe live state. Returns false if nothing was applied.
     */
    bool ApplySaveText(const FString& Text);

    /** Encode the current snapshot and write it to AbsolutePath. Returns success. */
    bool SaveToFile(const FString& AbsolutePath);

    /**
     * Read + decode + migrate + apply a save file. Missing file / unreadable / empty is a
     * no-op that returns false (matches Godot load_game's early-outs).
     */
    bool LoadFromFile(const FString& AbsolutePath);

    /** Default save path under the project's Saved/ dir (the user:// analogue). */
    static FString DefaultSavePath();

private:
    struct FHookEntry
    {
        FString Section;
        FSaveHook OnSave;
        FLoadHook OnLoad;
    };

    /** Registration-ordered hooks (mirrors Godot's gather/apply ordering over trackers). */
    TArray<FHookEntry> Hooks;
    /** Section -> index into Hooks. */
    TMap<FString, int32> Index;
};
