// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../Wiring/CharacterWiring.h"

/**
 * A saved, hand-editable "character recipe": everything the Forge needs to
 * recreate an authored character — which skeleton, what role, the identity seed.
 * Persisting these lets a creator author a roster once and re-add the whole cast
 * to the game on demand, instead of re-typing paths.
 *
 * Pure data + (de)serialization, headless-tested (Tests/CharacterRecipeTest.cpp,
 * prefix GTC.Characters.Recipe). Reuses the in-module ordered JSON model
 * (Systems/Save/SaveJson) like Missions/Editor/GtcMissionJson, and carries its
 * own tiny file envelope so recipes version independently:
 *
 *   { "format": "gtc_character", "version": 1, "recipe": { ... } }
 */
struct GTC_UE5_API FCharacterRecipe
{
	/** Display label for the roster. */
	FString Name;
	/** Skeletal mesh object path, e.g. "/Game/.../SK_X.SK_X". */
	FString SkeletonPath;
	/** Role the actions are wired for. */
	ECharacterRole Role = ECharacterRole::Pedestrian;
	/** Deterministic identity seed (for the spawned citizen). */
	int32 Seed = 0;

	/** A recipe is usable once it names a skeleton. */
	bool IsValid() const { return !SkeletonPath.IsEmpty(); }
};

namespace GtcCharacterRecipe
{
	/** Recipe-file envelope version (independent of the save game). */
	inline constexpr int32 FormatVersion = 1;

	/** Serialize one recipe to a complete, openable recipe-file JSON string. */
	GTC_UE5_API FString ToJson(const FCharacterRecipe& Recipe);

	/**
	 * Parse a recipe-file JSON string. On success fills Out and returns true; on
	 * malformed/empty/non-recipe input returns false with Out left invalid. Safe
	 * on untrusted text (never crashes).
	 */
	GTC_UE5_API bool FromJson(const FString& Text, FCharacterRecipe& Out);

	/** Save a recipe to a standalone .character.json file (UTF-8, makes dirs). */
	GTC_UE5_API bool SaveToFile(const FCharacterRecipe& Recipe, const FString& AbsolutePath);

	/** Load a recipe from a .character.json file (false + invalid Out on failure). */
	GTC_UE5_API bool LoadFromFile(const FString& AbsolutePath, FCharacterRecipe& Out);

	// --- Banks: a whole authored cast in one file -----------------------------
	//   { "format": "gtc_character_bank", "version": 1, "recipes": [ {...}, ... ] }

	/** Serialize a roster of recipes to a bank-file JSON string. */
	GTC_UE5_API FString BankToJson(const TArray<FCharacterRecipe>& Recipes);

	/**
	 * Parse a bank-file JSON string into Out (cleared first). Returns true when
	 * the envelope is a recognised bank (even if empty); malformed entries are
	 * skipped, not fatal. Returns false on non-bank / unparseable input.
	 */
	GTC_UE5_API bool BankFromJson(const FString& Text, TArray<FCharacterRecipe>& Out);

	/** Save a roster to a standalone .charbank.json file (UTF-8, makes dirs). */
	GTC_UE5_API bool SaveBankToFile(const TArray<FCharacterRecipe>& Recipes, const FString& AbsolutePath);

	/** Load a roster from a bank file (Out cleared; false on read/parse failure). */
	GTC_UE5_API bool LoadBankFromFile(const FString& AbsolutePath, TArray<FCharacterRecipe>& Out);

	/**
	 * Remove duplicate recipes in place, keeping the first occurrence of each
	 * (skeleton path, role) pair — so a merged or re-added roster doesn't spawn
	 * the same character twice. Returns how many were removed. Order-stable.
	 */
	GTC_UE5_API int32 Dedupe(TArray<FCharacterRecipe>& Recipes);
}
