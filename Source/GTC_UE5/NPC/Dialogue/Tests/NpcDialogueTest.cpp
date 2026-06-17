// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcDialogue.h"
#include "../../Archetypes/NpcArchetypes.h"

/**
 * Parity tests for FNpcDialogue, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_npc_dialogue.gd (13 funcs). Each TestTrue corresponds to
 * one oracle assertion with the oracle's own literals/seeds. Compound `a and b`
 * returns are split into independent TestTrue calls.
 *
 * test_every_voice_can_speak_every_reaction iterates NpcArchetypes::All() exactly
 * as the oracle iterates NpcArchetypes.all(), using each archetype's Voice.
 *
 * All test helpers below are system-prefixed (Dialogue*) to avoid anon-namespace
 * ODR collisions under unity build.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNpcDialogueTest,
	"GTC.NPC.Dialogue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcDialogueTest::RunTest(const FString& Parameters)
{
	// test_bark_is_deterministic
	{
		const FString A = FNpcDialogue::Bark(TEXT("conspiracy"), TEXT("idle"), 42);
		const FString B = FNpcDialogue::Bark(TEXT("conspiracy"), TEXT("idle"), 42);
		TestEqual(TEXT("bark deterministic: a == b"), A, B);
		TestNotEqual(TEXT("bark deterministic: a != \"\""), A, FString(TEXT("")));
	}

	// test_bark_varies_with_seed
	{
		TSet<FString> Seen;
		for (int32 S = 0; S < 12; ++S)
		{
			Seen.Add(FNpcDialogue::Bark(TEXT("doomsday"), TEXT("idle"), S));
		}
		TestTrue(TEXT("bark varies with seed: >= 2 distinct"), Seen.Num() >= 2);
	}

	// test_unknown_voice_falls_back_to_generic
	{
		const FString Line = FNpcDialogue::Bark(TEXT("there_is_no_such_voice"), TEXT("bump"), 1);
		TestNotEqual(TEXT("unknown voice: line != \"\""), Line, FString(TEXT("")));
		TestNotEqual(TEXT("unknown voice: line != \"...\""), Line, FString(TEXT("...")));
	}

	// test_unknown_context_still_speaks
	{
		TestEqual(TEXT("unknown context returns murmur \"...\""),
			FNpcDialogue::Bark(TEXT("yogi"), TEXT("underwater_basket_weaving"), 0), FString(TEXT("...")));
	}

	// test_slots_are_filled
	{
		bool bNoRawSlots = true;
		for (int32 S = 0; S < 20; ++S)
		{
			const FString Line = FNpcDialogue::Bark(TEXT("influencer"), TEXT("idle"), S);
			if (Line.Contains(TEXT("{")) || Line.Contains(TEXT("}")))
			{
				bNoRawSlots = false;
				break;
			}
		}
		TestTrue(TEXT("slots are filled: no raw {token} survives"), bNoRawSlots);
	}

	// test_slot_fill_draws_from_word_banks
	{
		bool bFoundBankWord = false;
		for (int32 S = 0; S < 24 && !bFoundBankWord; ++S)
		{
			const FString Line = FNpcDialogue::Bark(TEXT("influencer"), TEXT("idle"), S);
			for (const FString& Word : FNpcDialogue::Nouns())
			{
				if (Line.Contains(Word)) { bFoundBankWord = true; break; }
			}
			if (bFoundBankWord) { break; }
			for (const FString& Word : FNpcDialogue::Animals())
			{
				if (Line.Contains(Word)) { bFoundBankWord = true; break; }
			}
		}
		TestTrue(TEXT("slot fill draws from word banks"), bFoundBankWord);
	}

	// test_bark_for_activity_maps_to_context
	{
		const FString Via = FNpcDialogue::BarkForActivity(TEXT("food_critic"), TEXT("loiter"), 3);
		const FString Direct = FNpcDialogue::Bark(TEXT("food_critic"), TEXT("idle"), 3);
		TestEqual(TEXT("bark_for_activity loiter -> idle context"), Via, Direct);
	}

	// test_every_voice_can_speak_every_reaction
	{
		bool bAllSpeak = true;
		const TArray<FString> Reactions = { TEXT("see_player"), TEXT("flee"), TEXT("gawk"), TEXT("bump"), TEXT("idle") };
		for (const FNpcArchetype& C : FNpcArchetypes::All())
		{
			for (const FString& Ctx : Reactions)
			{
				if (FNpcDialogue::Bark(C.Voice, Ctx, 1) == TEXT(""))
				{
					bAllSpeak = false;
				}
			}
		}
		TestTrue(TEXT("every voice can speak every reaction"), bAllSpeak);
	}

	// test_weather_bark_speaks_every_condition
	{
		bool bAllConditions = true;
		for (const FString& Cond : { FString(TEXT("clear")), FString(TEXT("cloudy")), FString(TEXT("overcast")), FString(TEXT("rain")) })
		{
			if (FNpcDialogue::WeatherBark(TEXT("conspiracy"), Cond, 1) == TEXT(""))
			{
				bAllConditions = false;
			}
		}
		TestTrue(TEXT("weather bark speaks every condition"), bAllConditions);
	}

	// test_weather_anchor_gets_its_own_forecast
	{
		const FString Anchor = FNpcDialogue::WeatherBark(TEXT("weather"), TEXT("rain"), 0);
		const FString Civilian = FNpcDialogue::WeatherBark(TEXT("conspiracy"), TEXT("rain"), 0);
		TestNotEqual(TEXT("anchor forecast != \"\""), Anchor, FString(TEXT("")));
		TestNotEqual(TEXT("anchor forecast != civilian grumble"), Anchor, Civilian);
	}

	// test_weather_bark_is_deterministic
	{
		const FString A = FNpcDialogue::WeatherBark(TEXT("yogi"), TEXT("overcast"), 5);
		const FString B = FNpcDialogue::WeatherBark(TEXT("yogi"), TEXT("overcast"), 5);
		TestEqual(TEXT("weather bark deterministic: a == b"), A, B);
		TestNotEqual(TEXT("weather bark deterministic: a != \"\""), A, FString(TEXT("")));
	}

	// test_witness_bark_nonempty_and_filled
	{
		bool bWitnessOk = true;
		for (int32 S = 0; S < 12; ++S)
		{
			const FString Line = FNpcDialogue::WitnessBark(S);
			if (Line == TEXT("") || Line.Contains(TEXT("{")) || Line.Contains(TEXT("}")))
			{
				bWitnessOk = false;
			}
		}
		TestTrue(TEXT("witness bark nonempty and filled"), bWitnessOk);
	}

	// test_witness_bark_is_deterministic
	{
		TestEqual(TEXT("witness bark deterministic"), FNpcDialogue::WitnessBark(7), FNpcDialogue::WitnessBark(7));
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
