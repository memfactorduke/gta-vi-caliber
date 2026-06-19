// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NpcVoice.h"

/**
 * Turns a citizen's voice profile + a written line into actual SOUND — a short
 * stylised vocalisation, one voiced blip per syllable, pitched and paced by the
 * speaker (think Animal Crossing / Simlish, not literal TTS). This is the piece
 * that makes "every NPC can talk" audible: each person vocalises in their own
 * register so a crowd murmurs in distinct voices, and the blips line up with the
 * lip-sync duration AGTCCitizen already drives.
 *
 * Two stages, both deterministic in (Profile, Line, LineSeed):
 *   Plan      — schedule the syllable blips (when, how long, what pitch).
 *   RenderPcm — synthesise 16-bit mono PCM for those blips (a few formant-ish
 *               harmonics under a Hann window, plus breath noise from the
 *               profile's breathiness and a touch of roughness AM). No engine
 *               audio types, no RNG — noise comes from a hash so the same line
 *               always sounds identical.
 *
 * PURE-CORE: scene-free, allocation-bounded, unit-tested headless
 * (Tests/VoiceSynthTest.cpp, prefix GTC.NPC.Voice.Synth). The thin runtime
 * adapter (UGTCVoiceComponent) just queues RenderPcm's bytes into a procedural
 * sound wave on an attached audio component — all the DSP lives here so it can
 * be verified without an audio device.
 */
struct FVoiceSyllable
{
	/** Onset of the blip within the utterance, seconds. */
	float StartSec = 0.0f;

	/** Voiced length of the blip, seconds (gaps between blips are silence). */
	float DurationSec = 0.0f;

	/** Center pitch of the blip, Hz (contour wanders within the profile band). */
	float PitchHz = 140.0f;

	/** Peak linear amplitude of the blip, 0..1. */
	float Amplitude = 0.8f;
};

struct GTC_UE5_API FVoiceSynth
{
	/** The sample rate RenderPcm uses when none is supplied (Hz). */
	static int32 DefaultSampleRate();

	/**
	 * Schedule the syllable blips for `Line` in this voice. Empty for a mute
	 * profile (e.g. the mime) or a wordless line — those make no sound. Blip
	 * count tracks FNpcVoice::SyllableCount; total span tracks
	 * FNpcVoice::EstimateSpokenSeconds, so audio and lip-sync agree.
	 */
	static TArray<FVoiceSyllable> Plan(const FNpcVoiceProfile& Profile, const FString& Line, int32 LineSeed);

	/**
	 * Synthesise 16-bit mono PCM (little-endian, interleaved trivially as mono)
	 * for `Line` at `SampleRate`. Returns empty for a mute/silent plan. The
	 * buffer length matches the planned utterance duration; non-mute output
	 * always contains audible (non-zero) samples. Deterministic.
	 */
	static TArray<int16> RenderPcm(const FNpcVoiceProfile& Profile, const FString& Line, int32 LineSeed, int32 SampleRate);

	/** Convenience: RenderPcm at DefaultSampleRate(). */
	static TArray<int16> RenderPcm(const FNpcVoiceProfile& Profile, const FString& Line, int32 LineSeed);
};
