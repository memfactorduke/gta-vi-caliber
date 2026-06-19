// Copyright Epic Games, Inc. All Rights Reserved.

#include "VoiceSynth.h"

namespace
{
	// Deterministic hash -> [0,1), so "noise" (breath, roughness, contour) is the
	// same every time a given line is spoken by a given voice. Mirrors NpcVoice's
	// finalizer so the two stay in the same statistical family.
	uint32 SynthHash(uint32 A, uint32 B)
	{
		uint32 H = 2166136261u ^ A;
		H *= 16777619u;
		H ^= B;
		H *= 16777619u;
		H ^= H >> 15;
		H *= 2246822519u;
		H ^= H >> 13;
		return H;
	}

	float Hash01(uint32 A, uint32 B)
	{
		return static_cast<float>(SynthHash(A, B) & 0xFFFFFFu) / static_cast<float>(0x1000000u);
	}

	constexpr float kTwoPi = 6.28318530718f;
}

int32 FVoiceSynth::DefaultSampleRate()
{
	return 22050;
}

TArray<FVoiceSyllable> FVoiceSynth::Plan(const FNpcVoiceProfile& Profile, const FString& Line, int32 LineSeed)
{
	TArray<FVoiceSyllable> Plan;
	if (Profile.bMute)
	{
		return Plan; // silent citizen: no sound, lip-sync handled elsewhere
	}

	const int32 Count = FNpcVoice::SyllableCount(Line);
	if (Count <= 0)
	{
		return Plan;
	}

	const float Total = FNpcVoice::EstimateSpokenSeconds(Line, Profile);

	// Lay the syllables across the utterance: each slot is voiced for ~70% of its
	// span, the rest a brief gap so blips read as separate beats rather than a
	// drone. Pitch contour wanders within the profile band per syllable.
	const float Slot = Total / static_cast<float>(Count);
	const float VoicedFrac = 0.70f;
	for (int32 i = 0; i < Count; ++i)
	{
		FVoiceSyllable Syl;
		Syl.StartSec = static_cast<float>(i) * Slot;
		Syl.DurationSec = Slot * VoicedFrac;
		Syl.PitchHz = FNpcVoice::PitchHzForLine(Profile, LineSeed * 131 + i * 7919);
		// A gentle amplitude shape: open a touch louder, settle after — and never
		// dead silent, so every blip is audible.
		const float Shape = 0.75f + 0.2f * Hash01(static_cast<uint32>(LineSeed), static_cast<uint32>(i));
		Syl.Amplitude = FMath::Clamp(Shape, 0.4f, 1.0f);
		Plan.Add(Syl);
	}
	return Plan;
}

TArray<int16> FVoiceSynth::RenderPcm(const FNpcVoiceProfile& Profile, const FString& Line, int32 LineSeed, int32 SampleRate)
{
	TArray<int16> Pcm;
	if (SampleRate <= 0)
	{
		return Pcm;
	}

	const TArray<FVoiceSyllable> Syllables = Plan(Profile, Line, LineSeed);
	if (Syllables.Num() == 0)
	{
		return Pcm; // mute / silent
	}

	const float Total = FNpcVoice::EstimateSpokenSeconds(Line, Profile);
	const int32 NumSamples = FMath::Max(1, FMath::RoundToInt(Total * static_cast<float>(SampleRate)));
	Pcm.SetNumZeroed(NumSamples);

	// Formant-ish timbre: fundamental plus two harmonics, the upper one brightened
	// or darkened by the profile's vocal-tract size (FormantScale<1 = brighter).
	const float Bright = FMath::Clamp(1.0f / FMath::Max(Profile.FormantScale, 0.1f), 0.5f, 1.6f);
	const float Breath = FMath::Clamp(Profile.Breathiness, 0.0f, 1.0f);
	const float Rough = FMath::Clamp(Profile.Roughness, 0.0f, 1.0f);
	const float ToneGain = 1.0f - 0.4f * Breath; // breathy voices trade tone for air

	for (int32 s = 0; s < Syllables.Num(); ++s)
	{
		const FVoiceSyllable& Syl = Syllables[s];
		const int32 Start = FMath::Clamp(FMath::RoundToInt(Syl.StartSec * SampleRate), 0, NumSamples - 1);
		const int32 Len = FMath::Max(1, FMath::RoundToInt(Syl.DurationSec * SampleRate));
		const int32 End = FMath::Min(Start + Len, NumSamples);

		for (int32 n = Start; n < End; ++n)
		{
			const int32 Local = n - Start;
			const float T = static_cast<float>(Local) / static_cast<float>(SampleRate);
			// Hann window so each blip fades in/out (no clicks).
			const float Frac = static_cast<float>(Local) / static_cast<float>(FMath::Max(End - Start - 1, 1));
			const float Window = 0.5f - 0.5f * FMath::Cos(kTwoPi * Frac);

			const float Phase = kTwoPi * Syl.PitchHz * T;
			float Tone = FMath::Sin(Phase)
				+ 0.5f * FMath::Sin(2.0f * Phase)
				+ 0.25f * Bright * FMath::Sin(3.0f * Phase);
			Tone *= ToneGain / 1.75f; // normalise the harmonic stack

			// Breath: deterministic hashed noise, scaled by breathiness.
			float Air = 0.0f;
			if (Breath > 0.0f)
			{
				Air = (Hash01(static_cast<uint32>(LineSeed) ^ static_cast<uint32>(s), static_cast<uint32>(n)) * 2.0f - 1.0f) * Breath * 0.5f;
			}

			// Roughness: slow amplitude modulation that adds a gravel edge.
			const float Gravel = 1.0f - Rough * 0.4f * (0.5f + 0.5f * FMath::Sin(Phase * 0.5f));

			const float Sample = (Tone + Air) * Gravel * Syl.Amplitude * Window;
			const int32 Mixed = static_cast<int32>(Pcm[n]) + static_cast<int32>(Sample * 26000.0f);
			Pcm[n] = static_cast<int16>(FMath::Clamp(Mixed, -32768, 32767));
		}
	}

	return Pcm;
}

TArray<int16> FVoiceSynth::RenderPcm(const FNpcVoiceProfile& Profile, const FString& Line, int32 LineSeed)
{
	return RenderPcm(Profile, Line, LineSeed, DefaultSampleRate());
}
