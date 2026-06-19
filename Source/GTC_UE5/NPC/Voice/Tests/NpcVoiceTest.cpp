// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcVoice.h"

/**
 * Behavioural tests for FNpcVoice (GTC-original, no Godot oracle). They assert
 * the properties the audio layer relies on: determinism, per-citizen spread,
 * human-range pitch, persona character, and syllable-paced delivery.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNpcVoiceTest,
	"GTC.NPC.Voice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcVoiceTest::RunTest(const FString& Parameters)
{
	// --- Determinism: same (voice, seed) -> identical profile -----------------
	{
		const FNpcVoiceProfile A = FNpcVoice::ProfileFor(TEXT("influencer"), 42);
		const FNpcVoiceProfile B = FNpcVoice::ProfileFor(TEXT("influencer"), 42);
		TestTrue(TEXT("same voice+seed is the same profile"), A == B);
	}

	// --- Individuation: same persona, different seeds -> different voices ------
	{
		const FNpcVoiceProfile A = FNpcVoice::ProfileFor(TEXT("intern"), 1);
		const FNpcVoiceProfile B = FNpcVoice::ProfileFor(TEXT("intern"), 2);
		TestFalse(TEXT("two interns are not the identical voice"), A == B);
	}

	// --- Pitch stays in a plausible human speaking range ----------------------
	{
		bool bAllInRange = true;
		for (int32 Seed = 0; Seed < 200; ++Seed)
		{
			const FNpcVoiceProfile P = FNpcVoice::ProfileFor(TEXT(""), Seed);
			if (P.BasePitchHz < 80.0f || P.BasePitchHz > 255.0f)
			{
				bAllInRange = false;
				break;
			}
		}
		TestTrue(TEXT("neutral population pitch is always 80..255 Hz"), bAllInRange);
	}

	// --- Population actually spreads across registers (not one cloned pitch) --
	{
		float MinHz = 1e9f, MaxHz = -1e9f;
		for (int32 Seed = 0; Seed < 200; ++Seed)
		{
			const FNpcVoiceProfile P = FNpcVoice::ProfileFor(TEXT(""), Seed);
			MinHz = FMath::Min(MinHz, P.BasePitchHz);
			MaxHz = FMath::Max(MaxHz, P.BasePitchHz);
		}
		TestTrue(TEXT("a crowd spans a wide pitch range"), (MaxHz - MinHz) > 60.0f);
	}

	// --- Persona character: philosopher slower & lower than influencer --------
	// Averaged over seeds so the persona bias dominates individual spread.
	{
		float PhiloPitch = 0.0f, PhiloRate = 0.0f, InflPitch = 0.0f, InflRate = 0.0f;
		const int32 N = 64;
		for (int32 Seed = 0; Seed < N; ++Seed)
		{
			const FNpcVoiceProfile Phi = FNpcVoice::ProfileFor(TEXT("philosopher"), Seed);
			const FNpcVoiceProfile Inf = FNpcVoice::ProfileFor(TEXT("influencer"), Seed);
			PhiloPitch += Phi.BasePitchHz; PhiloRate += Phi.RateSyllablesPerSec;
			InflPitch += Inf.BasePitchHz;  InflRate += Inf.RateSyllablesPerSec;
		}
		TestTrue(TEXT("philosophers average a lower pitch than influencers"), PhiloPitch < InflPitch);
		TestTrue(TEXT("philosophers average a slower rate than influencers"), PhiloRate < InflRate);
	}

	// --- The mime is mute; the neutral citizen is not -------------------------
	{
		TestTrue(TEXT("mime is mute"), FNpcVoice::ProfileFor(TEXT("mime"), 7).bMute);
		TestFalse(TEXT("neutral citizen is audible"), FNpcVoice::ProfileFor(TEXT(""), 7).bMute);
	}

	// --- Formant size tracks register (deeper -> larger) ----------------------
	{
		// Scan for a clearly-deep and a clearly-bright voice and compare formants.
		FNpcVoiceProfile Deep = FNpcVoice::ProfileFor(TEXT(""), 0);
		FNpcVoiceProfile Bright = FNpcVoice::ProfileFor(TEXT(""), 0);
		for (int32 Seed = 0; Seed < 200; ++Seed)
		{
			const FNpcVoiceProfile P = FNpcVoice::ProfileFor(TEXT(""), Seed);
			if (P.BasePitchHz < Deep.BasePitchHz) Deep = P;
			if (P.BasePitchHz > Bright.BasePitchHz) Bright = P;
		}
		TestTrue(TEXT("deeper voice has a larger formant scale"), Deep.FormantScale > Bright.FormantScale);
	}

	// --- Syllable counting ----------------------------------------------------
	{
		TestEqual(TEXT("'hello' is 2 syllables"), FNpcVoice::SyllableCount(TEXT("hello")), 2);
		TestEqual(TEXT("'banana' is 3 syllables"), FNpcVoice::SyllableCount(TEXT("banana")), 3);
		TestEqual(TEXT("empty line is 0 syllables"), FNpcVoice::SyllableCount(TEXT("")), 0);
		TestEqual(TEXT("punctuation only is 0 syllables"), FNpcVoice::SyllableCount(TEXT("?!...")), 0);
		// A vowelless interjection still counts as one beat per word.
		TestEqual(TEXT("'hmm psst' is 2 beats"), FNpcVoice::SyllableCount(TEXT("hmm psst")), 2);
		// Contraction stays one word.
		TestTrue(TEXT("contraction counts vowels"), FNpcVoice::SyllableCount(TEXT("don't")) >= 1);
	}

	// --- Spoken-duration pacing ----------------------------------------------
	{
		const FNpcVoiceProfile P = FNpcVoice::ProfileFor(TEXT(""), 5);
		const float Short = FNpcVoice::EstimateSpokenSeconds(TEXT("Hey."), P);
		const float Long = FNpcVoice::EstimateSpokenSeconds(
			TEXT("I have been thinking about this for an extraordinarily long while."), P);
		TestTrue(TEXT("a longer line takes longer to say"), Long > Short);
		TestTrue(TEXT("duration is clamped to a floor"), Short >= 0.5f);
		TestTrue(TEXT("duration is clamped to a ceiling"), Long <= 8.0f);

		// Rate matters: a faster talker says the same line in less time.
		const FNpcVoiceProfile Fast = FNpcVoice::ProfileFor(TEXT("influencer"), 5);
		const FNpcVoiceProfile Slow = FNpcVoice::ProfileFor(TEXT("philosopher"), 5);
		const FString Mid = TEXT("Tell me what you saw out there today");
		TestTrue(TEXT("faster voice finishes the same line sooner"),
			FNpcVoice::EstimateSpokenSeconds(Mid, Fast) <= FNpcVoice::EstimateSpokenSeconds(Mid, Slow));
	}

	// --- Per-utterance pitch stays inside the profile's band ------------------
	{
		const FNpcVoiceProfile P = FNpcVoice::ProfileFor(TEXT("method_actor"), 11);
		const float Lo = P.BasePitchHz * FMath::Pow(2.0f, -P.PitchJitterSemitones / 12.0f) - 0.5f;
		const float Hi = P.BasePitchHz * FMath::Pow(2.0f, P.PitchJitterSemitones / 12.0f) + 0.5f;
		bool bAllInBand = true;
		for (int32 Line = 0; Line < 100; ++Line)
		{
			const float Hz = FNpcVoice::PitchHzForLine(P, Line);
			if (Hz < Lo || Hz > Hi) { bAllInBand = false; break; }
		}
		TestTrue(TEXT("per-line pitch stays within the jitter band"), bAllInBand);
	}

	// --- Zero-jitter profile returns the base pitch exactly -------------------
	{
		FNpcVoiceProfile Flat = FNpcVoice::ProfileFor(TEXT(""), 3);
		Flat.PitchJitterSemitones = 0.0f;
		TestEqual(TEXT("no jitter -> base pitch"), FNpcVoice::PitchHzForLine(Flat, 99), Flat.BasePitchHz);
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
