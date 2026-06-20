// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcVoice.h"

namespace
{
	// --- Deterministic hashing -------------------------------------------------
	// FNV-1a over the token's chars with the identity seed folded in, then a final
	// avalanche so nearby seeds don't produce nearby streams. Replaces any RNG so
	// the same (Voice, seed) is always the same voice, headless or in PIE.
	uint32 VoiceHash(const FString& Voice, int32 Seed, uint32 Salt)
	{
		uint32 H = 2166136261u ^ Salt;
		for (const TCHAR C : Voice)
		{
			H ^= static_cast<uint32>(C);
			H *= 16777619u;
		}
		const uint32 S = static_cast<uint32>(Seed);
		for (int32 Byte = 0; Byte < 4; ++Byte)
		{
			H ^= (S >> (Byte * 8)) & 0xFFu;
			H *= 16777619u;
		}
		// xorshift-ish finalizer
		H ^= H >> 15;
		H *= 2246822519u;
		H ^= H >> 13;
		return H;
	}

	/** Hash -> [0,1). */
	float Norm01(uint32 H)
	{
		return static_cast<float>(H & 0xFFFFFFu) / static_cast<float>(0x1000000u);
	}

	/** Multiply a pitch by a semitone offset (12-TET). */
	float Semitones(float Hz, float Semis)
	{
		return Hz * FMath::Pow(2.0f, Semis / 12.0f);
	}

	bool IsVowelChar(TCHAR C)
	{
		const TCHAR L = FChar::ToLower(C);
		return L == 'a' || L == 'e' || L == 'i' || L == 'o' || L == 'u' || L == 'y';
	}

	/** A persona's vocal character before per-individual seed spread is applied. */
	struct FPersona
	{
		float PitchSemis;  // offset from the seed-derived register
		float Jitter;      // expressiveness (semitone half-band)
		float Rate;        // syllables / sec
		float Breathiness; // 0..1
		float Roughness;   // 0..1
		bool bMute = false;
	};

	// Persona table. Tokens mirror FNpcArchetype::Voice (see FNpcDialogue banks).
	// Unknown/empty tokens fall through to a neutral persona, so every citizen
	// still gets an audible, individuated voice.
	FPersona PersonaFor(const FString& Voice)
	{
		if (Voice == TEXT("philosopher"))  return {-3.0f, 1.5f, 3.0f, 0.25f, 0.30f, false};
		if (Voice == TEXT("yogi"))         return {-2.0f, 1.2f, 2.8f, 0.50f, 0.10f, false};
		if (Voice == TEXT("life_coach"))   return {-1.0f, 2.0f, 3.4f, 0.45f, 0.15f, false};
		if (Voice == TEXT("food_critic"))  return {-1.0f, 2.5f, 3.8f, 0.20f, 0.20f, false};
		if (Voice == TEXT("method_actor")) return { 0.0f, 4.0f, 3.6f, 0.25f, 0.25f, false};
		if (Voice == TEXT("stunt_double")) return {-2.0f, 2.0f, 4.4f, 0.15f, 0.40f, false};
		if (Voice == TEXT("intern"))       return { 2.0f, 2.5f, 5.0f, 0.30f, 0.15f, false};
		if (Voice == TEXT("influencer"))   return { 3.0f, 3.5f, 5.6f, 0.35f, 0.10f, false};
		if (Voice == TEXT("conspiracy"))   return { 1.0f, 3.0f, 5.2f, 0.15f, 0.45f, false};
		if (Voice == TEXT("doomsday"))     return { 1.0f, 3.5f, 5.4f, 0.10f, 0.50f, false};
		if (Voice == TEXT("mime"))         return { 0.0f, 2.0f, 4.0f, 0.20f, 0.15f, true};
		// neutral citizen
		return {0.0f, 2.0f, 4.2f, 0.20f, 0.15f, false};
	}
}

bool FNpcVoiceProfile::operator==(const FNpcVoiceProfile& Other) const
{
	const float VoiceEps = 1e-4f;
	return bMute == Other.bMute
		&& FMath::IsNearlyEqual(BasePitchHz, Other.BasePitchHz, VoiceEps)
		&& FMath::IsNearlyEqual(PitchJitterSemitones, Other.PitchJitterSemitones, VoiceEps)
		&& FMath::IsNearlyEqual(RateSyllablesPerSec, Other.RateSyllablesPerSec, VoiceEps)
		&& FMath::IsNearlyEqual(Breathiness, Other.Breathiness, VoiceEps)
		&& FMath::IsNearlyEqual(Roughness, Other.Roughness, VoiceEps)
		&& FMath::IsNearlyEqual(FormantScale, Other.FormantScale, VoiceEps);
}

FNpcVoiceProfile FNpcVoice::ProfileFor(const FString& Voice, int32 IdentitySeed)
{
	const FPersona P = PersonaFor(Voice);
	FNpcVoiceProfile Out;

	// Register: spread the population across deep..bright (95..235 Hz) by seed, so
	// even one persona token covers many bodies rather than one cloned voice.
	const float Reg = Norm01(VoiceHash(Voice, IdentitySeed, 0x9E37u));
	float Pitch = 95.0f + Reg * (235.0f - 95.0f);

	// Persona shifts the register, then a sub-semitone individual nudge keeps two
	// same-register, same-persona people from landing on the identical pitch.
	Pitch = Semitones(Pitch, P.PitchSemis);
	const float Nudge = (Norm01(VoiceHash(Voice, IdentitySeed, 0x2545u)) - 0.5f) * 1.5f; // +/-0.75 semis
	Pitch = Semitones(Pitch, Nudge);
	Out.BasePitchHz = FMath::Clamp(Pitch, 80.0f, 255.0f);

	// Expressiveness and tempo carry small individual variation around the persona.
	Out.PitchJitterSemitones = FMath::Max(0.0f,
		P.Jitter + (Norm01(VoiceHash(Voice, IdentitySeed, 0x1B83u)) - 0.5f) * 0.8f);
	Out.RateSyllablesPerSec = FMath::Clamp(
		P.Rate + (Norm01(VoiceHash(Voice, IdentitySeed, 0x7F4Au)) - 0.5f) * 0.6f, 2.0f, 7.0f);

	Out.Breathiness = FMath::Clamp(P.Breathiness, 0.0f, 1.0f);
	Out.Roughness = FMath::Clamp(P.Roughness, 0.0f, 1.0f);

	// Formant size tracks register: deeper voices read as larger vocal tracts.
	Out.FormantScale = FMath::Clamp(150.0f / Out.BasePitchHz, 0.75f, 1.4f);

	Out.bMute = P.bMute;
	return Out;
}

int32 FNpcVoice::SyllableCount(const FString& Line)
{
	int32 Total = 0;
	bool bInWord = false;       // currently scanning a run of letters
	bool bWordHadVowel = false; // this word has produced >=1 vowel group
	bool bPrevVowel = false;    // previous char (this word) was a vowel

	auto EndWord = [&]()
	{
		if (bInWord && !bWordHadVowel)
		{
			// A letter-run with no vowel (e.g. "hmm", "psst") is still one beat.
			++Total;
		}
		bInWord = false;
		bWordHadVowel = false;
		bPrevVowel = false;
	};

	for (const TCHAR C : Line)
	{
		const bool bAlpha = FChar::IsAlpha(C);
		if (bAlpha)
		{
			bInWord = true;
			const bool bVowel = IsVowelChar(C);
			if (bVowel && !bPrevVowel)
			{
				++Total;
				bWordHadVowel = true;
			}
			bPrevVowel = bVowel;
		}
		else if (C == TEXT('\''))
		{
			// Apostrophe keeps a contraction as one word; not a vowel boundary.
			bPrevVowel = false;
		}
		else
		{
			EndWord();
		}
	}
	EndWord();
	return Total;
}

float FNpcVoice::EstimateSpokenSeconds(const FString& Line, const FNpcVoiceProfile& Profile)
{
	const int32 Syllables = SyllableCount(Line);
	const float Rate = FMath::Max(Profile.RateSyllablesPerSec, 1.0f);
	float Seconds = static_cast<float>(Syllables) / Rate;

	// A breath at every sentence-final mark so delivery has phrasing, not a sprint.
	const TCHAR Ellipsis = static_cast<TCHAR>(0x2026);
	int32 Stops = 0;
	for (const TCHAR C : Line)
	{
		if (C == TEXT('.') || C == TEXT('!') || C == TEXT('?') || C == Ellipsis)
		{
			++Stops;
		}
	}
	Seconds += static_cast<float>(Stops) * 0.22f;

	return FMath::Clamp(Seconds, 0.5f, 8.0f);
}

float FNpcVoice::PitchHzForLine(const FNpcVoiceProfile& Profile, int32 LineSeed)
{
	if (Profile.PitchJitterSemitones <= 0.0f)
	{
		return Profile.BasePitchHz;
	}
	const float Signed = Norm01(VoiceHash(TEXT("line"), LineSeed, 0xA13Cu)) * 2.0f - 1.0f; // [-1,1)
	const float Semis = Signed * Profile.PitchJitterSemitones;
	return Semitones(Profile.BasePitchHz, Semis);
}
