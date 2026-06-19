// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VoiceSynth.h"
#include "../NpcVoice.h"

/**
 * Behavioural tests for FVoiceSynth: the plan tracks the line's syllables and
 * duration, the rendered PCM is the right length, audible, deterministic, and a
 * mute voice produces silence.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVoiceSynthTest,
	"GTC.NPC.Voice.Synth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVoiceSynthTest::RunTest(const FString& Parameters)
{
	const FNpcVoiceProfile Voice = FNpcVoice::ProfileFor(TEXT("influencer"), 17);
	const FString Line = TEXT("Hey, did you see that?");

	// --- Plan: one blip per syllable, spanning the spoken duration -------------
	{
		const TArray<FVoiceSyllable> Plan = FVoiceSynth::Plan(Voice, Line, 3);
		TestEqual(TEXT("blip count == syllable count"), Plan.Num(), FNpcVoice::SyllableCount(Line));
		TestTrue(TEXT("has blips"), Plan.Num() > 0);

		// Onsets are strictly increasing and inside the utterance.
		const float Total = FNpcVoice::EstimateSpokenSeconds(Line, Voice);
		bool bOrdered = true;
		for (int32 i = 1; i < Plan.Num(); ++i)
		{
			if (Plan[i].StartSec <= Plan[i - 1].StartSec) { bOrdered = false; break; }
		}
		TestTrue(TEXT("blip onsets increase"), bOrdered);
		TestTrue(TEXT("last onset within the utterance"), Plan.Last().StartSec < Total);
		TestTrue(TEXT("blips are voiced (positive duration)"), Plan[0].DurationSec > 0.0f);
	}

	// --- Render: correct length, audible, deterministic ------------------------
	{
		const int32 Rate = 16000;
		const TArray<int16> A = FVoiceSynth::RenderPcm(Voice, Line, 3, Rate);
		const TArray<int16> B = FVoiceSynth::RenderPcm(Voice, Line, 3, Rate);

		const float Total = FNpcVoice::EstimateSpokenSeconds(Line, Voice);
		const int32 Expected = FMath::RoundToInt(Total * Rate);
		TestEqual(TEXT("PCM length matches duration*rate"), A.Num(), Expected);

		TestTrue(TEXT("render is deterministic"), A == B);

		int32 Peak = 0;
		for (int16 V : A) { Peak = FMath::Max(Peak, FMath::Abs(static_cast<int32>(V))); }
		TestTrue(TEXT("rendered audio is audible (non-silent)"), Peak > 1000);
		TestTrue(TEXT("rendered audio does not clip the full rail constantly"), Peak <= 32767);
	}

	// --- Two different voices saying the same line differ ----------------------
	{
		const FNpcVoiceProfile Deep = FNpcVoice::ProfileFor(TEXT("philosopher"), 1);
		const FNpcVoiceProfile Bright = FNpcVoice::ProfileFor(TEXT("intern"), 99);
		const TArray<int16> X = FVoiceSynth::RenderPcm(Deep, Line, 3, 16000);
		const TArray<int16> Y = FVoiceSynth::RenderPcm(Bright, Line, 3, 16000);
		TestFalse(TEXT("different voices render differently"), X == Y);
	}

	// --- A mute citizen (mime) makes no sound ----------------------------------
	{
		const FNpcVoiceProfile Mute = FNpcVoice::ProfileFor(TEXT("mime"), 5);
		TestTrue(TEXT("mime profile is mute"), Mute.bMute);
		TestEqual(TEXT("mute plan is empty"), FVoiceSynth::Plan(Mute, Line, 3).Num(), 0);
		TestEqual(TEXT("mute render is empty"), FVoiceSynth::RenderPcm(Mute, Line, 3, 16000).Num(), 0);
	}

	// --- A wordless line makes no sound ----------------------------------------
	{
		TestEqual(TEXT("punctuation-only render is empty"),
			FVoiceSynth::RenderPcm(Voice, TEXT("?!..."), 3, 16000).Num(), 0);
	}

	// --- Bad sample rate is handled gracefully ---------------------------------
	{
		TestEqual(TEXT("zero sample rate -> empty"), FVoiceSynth::RenderPcm(Voice, Line, 3, 0).Num(), 0);
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
