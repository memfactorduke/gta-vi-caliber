// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SaveJson.h"

/**
 * FWorldSaveGame — the pure-model UE port of Godot `save_game.gd` (class `SaveGame`,
 * RefCounted). Versioned serialize/load of an arbitrary world+player state Dictionary,
 * as pure JSON over plain data so it unit-tests headless and stays engine-version stable.
 *
 * This is a DISTINCT, SELF-CONTAINED model from the merged GtcSaveData / USaveSubsystem
 * layer: the Godot `SaveGame` envelope is `{version, state}` with VERSION == 1, whereas
 * GtcSaveData uses a `{version, data}` envelope at version 4. We re-use only the in-module
 * ordered JSON value model (SaveJson.h) — the sanctioned "FJsonObject or equivalent" — for
 * the `state` payload, mirroring Godot's observable Dictionary key order. No engine USaveGame,
 * no FArchive: serialization is JSON text exactly like the Godot static SaveGame API.
 *
 * The Godot file also exposes write()/read()/has_save() over `user://`; those map to the
 * static file helpers below (FFileHelper under Saved/, the user:// analogue). The actor
 * `savepoint.gd` (class Savepoint, Node3D) is DEFERRED to Wave 3 and intentionally NOT ported.
 *
 * --- ROUND-TRIP TOLERANCE CONTRACT ---
 * Round-trip parity is by value-within-tolerance / structural equality, NOT byte-identity.
 * Structure (keys present, order, array sizes, version header), integers, strings and bools
 * round-trip exactly; floating-point text is tolerance-bound (compare with GtcTest::Eps).
 *
 * Static-style free functions in a namespace (no UObject), exactly like Godot's static
 * `SaveGame` API. The owning UObject layer is UWorldSerializerSubsystem.
 */
namespace FWorldSaveGame
{
    /** Save format version. Matches Godot `SaveGame.VERSION`. */
    inline constexpr int32 Version = 1;

    /** Default save path under the project's Saved/ dir (the `user://savegame.json` analogue). */
    GTC_UE5_API FString DefaultPath();

    /** Wrap a state object with a version header and serialise to JSON text. */
    GTC_UE5_API FString Serialize(const TSharedRef<FGtcJsonObject>& State);

    /**
     * Parse save text back to the inner `state` object. Anything that isn't a versioned
     * object (version == Version) carrying a `state` object yields an EMPTY object — so
     * bad JSON, the wrong version, and a missing `state` key all collapse to {}, exactly
     * like Godot's deserialize(). Silent on malformed input (a normal first-run path).
     */
    GTC_UE5_API TSharedRef<FGtcJsonObject> Deserialize(const FString& Text);

    /** Serialize `State` and write it to `Path`. Returns success (mirrors Godot write()). */
    GTC_UE5_API bool Write(const TSharedRef<FGtcJsonObject>& State, const FString& Path);
    GTC_UE5_API bool Write(const TSharedRef<FGtcJsonObject>& State);

    /**
     * Read + deserialize a save file. A missing/unreadable file yields an empty object
     * (mirrors Godot read()'s file_exists early-out). Never crashes on a corrupt file.
     */
    GTC_UE5_API TSharedRef<FGtcJsonObject> Read(const FString& Path);
    GTC_UE5_API TSharedRef<FGtcJsonObject> Read();

    /** True when a save file exists at `Path` (mirrors Godot has_save()). */
    GTC_UE5_API bool HasSave(const FString& Path);
    GTC_UE5_API bool HasSave();
}
