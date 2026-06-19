// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../CharacterRecipe.h"

/**
 * Round-trip + robustness tests for the character recipe persistence (prefix
 * GTC.Characters.Recipe). Mirrors the mission-json contract: structure / ints /
 * strings / enum tokens round-trip exactly; parse is silent + safe on garbage.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCharacterRecipeTest,
	"GTC.Characters.Recipe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterRecipeTest::RunTest(const FString& Parameters)
{
	// --- Role token round-trips (incl. VehicleOccupant ↔ "occupant") ----------
	{
		const ECharacterRole Roles[] = {
			ECharacterRole::Player, ECharacterRole::Pedestrian,
			ECharacterRole::Combatant, ECharacterRole::VehicleOccupant };
		for (ECharacterRole R : Roles)
		{
			TestEqual(TEXT("role token round-trips"),
				GTCParseCharacterRole(GTCCharacterRoleToken(R)), R);
		}
	}

	// --- Recipe round-trips through JSON for every role -----------------------
	{
		const ECharacterRole Roles[] = {
			ECharacterRole::Player, ECharacterRole::Pedestrian,
			ECharacterRole::Combatant, ECharacterRole::VehicleOccupant };
		for (ECharacterRole R : Roles)
		{
			FCharacterRecipe In;
			In.Name = TEXT("Lola the dock worker");
			In.SkeletonPath = TEXT("/Game/GTCaliberAssets/Content/Characters/SK_Lola.SK_Lola");
			In.Role = R;
			In.Seed = 4242;

			const FString Json = GtcCharacterRecipe::ToJson(In);
			TestTrue(TEXT("json is non-empty"), Json.Len() > 0);

			FCharacterRecipe Out;
			TestTrue(TEXT("round-trip parses"), GtcCharacterRecipe::FromJson(Json, Out));
			TestEqual(TEXT("name preserved"), Out.Name, In.Name);
			TestEqual(TEXT("path preserved"), Out.SkeletonPath, In.SkeletonPath);
			TestEqual(TEXT("role preserved"), Out.Role, In.Role);
			TestEqual(TEXT("seed preserved"), Out.Seed, In.Seed);
			TestTrue(TEXT("result is valid"), Out.IsValid());
		}
	}

	// --- Robustness: garbage / wrong-format / missing skeleton ----------------
	{
		FCharacterRecipe Out;
		TestFalse(TEXT("empty text fails"), GtcCharacterRecipe::FromJson(TEXT(""), Out));
		TestFalse(TEXT("garbage fails"), GtcCharacterRecipe::FromJson(TEXT("}{ not json"), Out));
		TestFalse(TEXT("wrong format fails"),
			GtcCharacterRecipe::FromJson(TEXT("{\"format\":\"something_else\"}"), Out));

		// A recipe with no skeleton path is not usable → serialize, parse, reject.
		FCharacterRecipe NoSkel;
		NoSkel.Name = TEXT("nameless");
		const FString Json = GtcCharacterRecipe::ToJson(NoSkel);
		TestFalse(TEXT("skeleton-less recipe rejected on parse"),
			GtcCharacterRecipe::FromJson(Json, Out));
		TestFalse(TEXT("rejected Out left invalid"), Out.IsValid());
	}

	// --- Bank round-trips a whole roster --------------------------------------
	{
		TArray<FCharacterRecipe> Bank;
		const ECharacterRole Roles[] = {
			ECharacterRole::Player, ECharacterRole::Combatant, ECharacterRole::VehicleOccupant };
		for (int32 i = 0; i < 3; ++i)
		{
			FCharacterRecipe R;
			R.Name = FString::Printf(TEXT("cast_%d"), i);
			R.SkeletonPath = FString::Printf(TEXT("/Game/SK_Cast_%d.SK_Cast_%d"), i, i);
			R.Role = Roles[i];
			R.Seed = 100 + i;
			Bank.Add(R);
		}

		const FString Json = GtcCharacterRecipe::BankToJson(Bank);
		TArray<FCharacterRecipe> Out;
		TestTrue(TEXT("bank parses"), GtcCharacterRecipe::BankFromJson(Json, Out));
		TestEqual(TEXT("bank count preserved"), Out.Num(), Bank.Num());
		if (Out.Num() == 3)
		{
			TestEqual(TEXT("bank[1] name"), Out[1].Name, FString(TEXT("cast_1")));
			TestEqual(TEXT("bank[2] role (occupant)"), Out[2].Role, ECharacterRole::VehicleOccupant);
			TestEqual(TEXT("bank[0] seed"), Out[0].Seed, 100);
		}
	}

	// --- Bank robustness ------------------------------------------------------
	{
		TArray<FCharacterRecipe> Out;
		TestTrue(TEXT("empty bank round-trips to empty"),
			GtcCharacterRecipe::BankFromJson(GtcCharacterRecipe::BankToJson({}), Out));
		TestEqual(TEXT("empty bank has no recipes"), Out.Num(), 0);
		TestFalse(TEXT("a single-recipe file is not a bank"),
			GtcCharacterRecipe::BankFromJson(GtcCharacterRecipe::ToJson(FCharacterRecipe()), Out));
		TestFalse(TEXT("garbage is not a bank"),
			GtcCharacterRecipe::BankFromJson(TEXT("nonsense"), Out));
	}

	// --- Dedupe keeps first of each (path, role), order-stable -----------------
	{
		auto Make = [](const TCHAR* Path, ECharacterRole Role, const TCHAR* Name)
		{
			FCharacterRecipe R; R.SkeletonPath = Path; R.Role = Role; R.Name = Name; return R;
		};
		TArray<FCharacterRecipe> Roster = {
			Make(TEXT("/Game/SK_A.SK_A"), ECharacterRole::Pedestrian, TEXT("first")),
			Make(TEXT("/Game/SK_B.SK_B"), ECharacterRole::Combatant, TEXT("b")),
			Make(TEXT("/game/sk_a.sk_a"), ECharacterRole::Pedestrian, TEXT("dup-of-first")),  // case-insensitive dup
			Make(TEXT("/Game/SK_A.SK_A"), ECharacterRole::Combatant, TEXT("a-but-different-role")), // not a dup
		};

		const int32 Removed = GtcCharacterRecipe::Dedupe(Roster);
		TestEqual(TEXT("dedupe removed exactly one"), Removed, 1);
		TestEqual(TEXT("dedupe left three"), Roster.Num(), 3);
		TestEqual(TEXT("dedupe kept the first occurrence"), Roster[0].Name, FString(TEXT("first")));
		TestEqual(TEXT("dedupe kept the different-role entry"), Roster[2].Name, FString(TEXT("a-but-different-role")));
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
