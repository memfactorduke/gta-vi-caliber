// Copyright Epic Games, Inc. All Rights Reserved.

#include "CharacterRecipe.h"

#include "../../Systems/Save/SaveJson.h"
#include "Misc/FileHelper.h"

namespace
{
	// One recipe <-> one JSON object (no envelope), shared by the single-recipe
	// and bank serializers.
	TSharedRef<FGtcJsonObject> RecipeToObject(const FCharacterRecipe& Recipe)
	{
		TSharedRef<FGtcJsonObject> O = MakeShared<FGtcJsonObject>();
		O->SetString(TEXT("name"), Recipe.Name);
		O->SetString(TEXT("skeleton_path"), Recipe.SkeletonPath);
		O->SetString(TEXT("role"), GTCCharacterRoleToken(Recipe.Role));
		O->SetNumber(TEXT("seed"), static_cast<double>(Recipe.Seed));
		return O;
	}

	// Read one recipe object; returns false (and leaves Out invalid) if it names
	// no skeleton, so a half-built recipe never reaches a caller.
	bool RecipeFromObject(const TSharedPtr<FGtcJsonObject>& O, FCharacterRecipe& Out)
	{
		Out = FCharacterRecipe();
		if (!O.IsValid())
		{
			return false;
		}
		Out.Name = O->GetString(TEXT("name"));
		Out.SkeletonPath = O->GetString(TEXT("skeleton_path"));
		Out.Role = GTCParseCharacterRole(O->GetString(TEXT("role"), TEXT("pedestrian")));
		Out.Seed = static_cast<int32>(O->GetNumber(TEXT("seed"), 0.0));
		if (!Out.IsValid())
		{
			Out = FCharacterRecipe();
			return false;
		}
		return true;
	}
}

FString GtcCharacterRecipe::ToJson(const FCharacterRecipe& Recipe)
{
	TSharedRef<FGtcJsonObject> Root = MakeShared<FGtcJsonObject>();
	Root->SetString(TEXT("format"), TEXT("gtc_character"));
	Root->SetNumber(TEXT("version"), GtcCharacterRecipe::FormatVersion);
	Root->SetObject(TEXT("recipe"), RecipeToObject(Recipe));
	return GtcJson::Serialize(Root);
}

bool GtcCharacterRecipe::FromJson(const FString& Text, FCharacterRecipe& Out)
{
	Out = FCharacterRecipe();

	const FGtcJsonValuePtr Val = GtcJson::Parse(Text);
	if (!Val.IsValid())
	{
		return false;
	}
	const TSharedPtr<FGtcJsonObject> Root = Val->AsObject();
	if (!Root.IsValid() || Root->GetString(TEXT("format")) != TEXT("gtc_character"))
	{
		return false;
	}
	return RecipeFromObject(Root->GetObject(TEXT("recipe")), Out);
}

FString GtcCharacterRecipe::BankToJson(const TArray<FCharacterRecipe>& Recipes)
{
	TArray<FGtcJsonValuePtr> Arr;
	Arr.Reserve(Recipes.Num());
	for (const FCharacterRecipe& R : Recipes)
	{
		Arr.Add(FGtcJsonValue::MakeObject(RecipeToObject(R)));
	}

	TSharedRef<FGtcJsonObject> Root = MakeShared<FGtcJsonObject>();
	Root->SetString(TEXT("format"), TEXT("gtc_character_bank"));
	Root->SetNumber(TEXT("version"), GtcCharacterRecipe::FormatVersion);
	Root->SetArray(TEXT("recipes"), Arr);
	return GtcJson::Serialize(Root);
}

bool GtcCharacterRecipe::BankFromJson(const FString& Text, TArray<FCharacterRecipe>& Out)
{
	Out.Reset();

	const FGtcJsonValuePtr Val = GtcJson::Parse(Text);
	if (!Val.IsValid())
	{
		return false;
	}
	const TSharedPtr<FGtcJsonObject> Root = Val->AsObject();
	if (!Root.IsValid() || Root->GetString(TEXT("format")) != TEXT("gtc_character_bank"))
	{
		return false;
	}

	// Skip any malformed entry rather than failing the whole load.
	for (const FGtcJsonValuePtr& Entry : Root->GetArray(TEXT("recipes")))
	{
		FCharacterRecipe R;
		if (Entry.IsValid() && RecipeFromObject(Entry->AsObject(), R))
		{
			Out.Add(R);
		}
	}
	return true;
}

int32 GtcCharacterRecipe::Dedupe(TArray<FCharacterRecipe>& Recipes)
{
	TSet<FString> Seen;
	const int32 Before = Recipes.Num();
	for (int32 i = 0; i < Recipes.Num(); )
	{
		// Key on skeleton + role; case-insensitive path so /Game vs /game match.
		const FString Key = Recipes[i].SkeletonPath.ToLower()
			+ TEXT("|") + GTCCharacterRoleToken(Recipes[i].Role);
		if (Seen.Contains(Key))
		{
			Recipes.RemoveAt(i);  // shifts down; do not advance i
		}
		else
		{
			Seen.Add(Key);
			++i;
		}
	}
	return Before - Recipes.Num();
}

bool GtcCharacterRecipe::SaveBankToFile(const TArray<FCharacterRecipe>& Recipes, const FString& AbsolutePath)
{
	return FFileHelper::SaveStringToFile(
		BankToJson(Recipes), *AbsolutePath, FFileHelper::EEncodingOptions::ForceUTF8);
}

bool GtcCharacterRecipe::LoadBankFromFile(const FString& AbsolutePath, TArray<FCharacterRecipe>& Out)
{
	Out.Reset();
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *AbsolutePath))
	{
		return false;
	}
	return BankFromJson(Text, Out);
}

bool GtcCharacterRecipe::SaveToFile(const FCharacterRecipe& Recipe, const FString& AbsolutePath)
{
	return FFileHelper::SaveStringToFile(
		ToJson(Recipe), *AbsolutePath, FFileHelper::EEncodingOptions::ForceUTF8);
}

bool GtcCharacterRecipe::LoadFromFile(const FString& AbsolutePath, FCharacterRecipe& Out)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *AbsolutePath))
	{
		Out = FCharacterRecipe();
		return false;
	}
	return FromJson(Text, Out);
}
