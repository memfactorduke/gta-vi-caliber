// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FGtcMissionDefinition;
class FGtcJsonObject;

/**
 * Pure (de)serialization for one FGtcMissionDefinition — the layer that makes a mission
 * "openable" as a human-readable, hand-editable .mission.json file.
 *
 * JSON backing: reuses the in-module ordered JSON model (Systems/Save/SaveJson.h:
 * FGtcJsonObject / GtcJson::Serialize/Parse) and the vector helpers (SaveData.h:
 * GtcSaveData::Vec3ToArray/ArrayToVec3). It deliberately does NOT pull in the engine
 * `Json` module — that stays a private dep of GTC_UE5 and adding it to the public deps
 * is a forbidden Build.cs edit (see SaveData.h). It also does NOT route through
 * GtcSaveData::Encode/Decode: those stamp the *save-game* schema version (currently 4)
 * and run save-game migration, which is the wrong meaning for a mission file. Instead a
 * mission file carries its own tiny envelope:
 *
 *   { "format": "gtc_mission", "version": 1, "mission": { ...the definition... } }
 *
 * so mission files version and migrate independently of the save game.
 *
 * Round-trip contract (matches the rest of the save system): structure, ints, strings,
 * bools and enum names round-trip exactly; floats (vectors, radii, time) are value-within-
 * tolerance, compared with GtcTest::Eps in tests, never by byte-identity. Parse is silent
 * and safe on malformed input — JsonToMission returns false and leaves Out.bValid==false,
 * never crashing and never producing a half-built mission a caller might persist.
 */
namespace GtcMissionJson
{
    /** Mission-file envelope version (independent of the save-game version). */
    inline constexpr int32 FormatVersion = 1;

    /** Serialize one definition to a complete, openable mission-file JSON string. */
    GTC_UE5_API FString MissionToJson(const FGtcMissionDefinition& Def);

    /**
     * Parse a mission-file JSON string. On success fills Out and returns true; on any
     * malformed/empty/non-mission input returns false with Out.bValid==false. Safe on
     * untrusted text (never crashes).
     */
    GTC_UE5_API bool JsonToMission(const FString& Text, FGtcMissionDefinition& Out);

    /**
     * Save one mission to a standalone, openable .mission.json file (UTF-8). Creates the
     * parent directory tree if needed. Returns false on a write failure.
     */
    GTC_UE5_API bool SaveMissionToFile(const FGtcMissionDefinition& Def, const FString& AbsolutePath);

    /**
     * Load one mission from a .mission.json file. On a missing/unreadable file or malformed
     * content, returns false with Out.bValid==false (never a half-built mission).
     */
    GTC_UE5_API bool LoadMissionFromFile(const FString& AbsolutePath, FGtcMissionDefinition& Out);

    /** Lower-level: definition -> ordered JSON object (no envelope). */
    GTC_UE5_API TSharedRef<FGtcJsonObject> MissionToObject(const FGtcMissionDefinition& Def);

    /**
     * Lower-level: JSON object -> definition. A null/empty object yields a definition
     * with bValid==false (so a corrupt load can't masquerade as a real mission).
     */
    GTC_UE5_API FGtcMissionDefinition ObjectToMission(const TSharedPtr<FGtcJsonObject>& Object);

    /**
     * Write a whole mission bank into a save section under the "missions" key (for the
     * savegame hook). Reading the bank back drops any mission that fails to parse.
     */
    GTC_UE5_API void WriteBank(const TArray<FGtcMissionDefinition>& Bank, const TSharedRef<FGtcJsonObject>& SectionOut);
    GTC_UE5_API void ReadBank(const TSharedRef<FGtcJsonObject>& SectionIn, TArray<FGtcMissionDefinition>& OutBank);

    /** Write/read a set of GUIDs under Key (used to persist completed-mission progress). */
    GTC_UE5_API void WriteGuidSet(const TSharedRef<FGtcJsonObject>& Section, const FString& Key, const TSet<FGuid>& Ids);
    GTC_UE5_API void ReadGuidSet(const TSharedRef<FGtcJsonObject>& Section, const FString& Key, TSet<FGuid>& OutIds);

    /** Write/read a set of strings under Key (used to persist granted mission unlocks). */
    GTC_UE5_API void WriteStringSet(const TSharedRef<FGtcJsonObject>& Section, const FString& Key, const TSet<FString>& Strings);
    GTC_UE5_API void ReadStringSet(const TSharedRef<FGtcJsonObject>& Section, const FString& Key, TSet<FString>& OutStrings);
}
