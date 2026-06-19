// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A citizen's actual *vocal* identity — the synthesis parameters that turn a
 * written bark (FNpcDialogue / FBarkPool / FContactBark) into an audible voice.
 *
 * Until now "voice" in this codebase meant a comic text persona ("philosopher",
 * "influencer", "mime"...) that only picked WHICH words a citizen said. Nobody
 * actually made a sound: AGTCCitizen::SpeakLine fired the OnSpeak lip-sync hook
 * with a crude char-count duration and no pitch, rate, or timbre. This model is
 * the missing half — so "every NPC can talk" means every NPC has a distinct
 * SOUND, not just a script.
 *
 * FNpcVoiceProfile is the per-person vocal signature: base pitch, how much that
 * pitch wanders, speaking rate, and timbre (breathiness/roughness/formant size).
 * It is derived deterministically from {persona voice token, identity seed}, so
 * the same citizen always sounds the same across reloads, two "influencer"s
 * sound related but not identical, and the population spreads across registers
 * (deep to bright) instead of one cloned voice. A runtime audio layer
 * (MetaHuman / Audio2Face, a pitch-shifted phoneme bank, or a TTS bridge) reads
 * the profile to set voice pitch/rate and drive jaw/viseme timing.
 *
 * PURE-CORE: scene-free, allocation-light, fully deterministic, no UObject and
 * no engine audio types — unit-tested headless (Tests/NpcVoiceTest.cpp, prefix
 * GTC.NPC.Voice). GTC-original content (not a Godot parity port). The thin actor
 * adapter (AGTCCitizen) caches a profile at identity time and feeds spoken lines
 * through EstimateSpokenSeconds so lip-sync spans match the line's syllables and
 * the speaker's tempo, not its character length.
 */
struct GTC_UE5_API FNpcVoiceProfile
{
	/** Median speaking fundamental (Hz). Human conversational F0 ~85..255. */
	float BasePitchHz = 140.0f;

	/** Half-width of natural pitch movement around BasePitchHz, in semitones. */
	float PitchJitterSemitones = 2.0f;

	/** Delivery tempo (syllables per second). Conversational ~3..6. */
	float RateSyllablesPerSec = 4.2f;

	/** Airy/whispery weight, 0 (clear) .. 1 (breathy). */
	float Breathiness = 0.2f;

	/** Gravel/creak weight, 0 (smooth) .. 1 (rough). */
	float Roughness = 0.15f;

	/**
	 * Vocal-tract length proxy that scales formants: <1 brighter/smaller, >1
	 * darker/larger. Tracks register (deeper voices read as bigger bodies).
	 */
	float FormantScale = 1.0f;

	/**
	 * True for a citizen who emits no spoken audio (e.g. the street "mime"). It
	 * still gets timing so gesture/lip beats line up, but the audio layer should
	 * play no clip. Everyone else makes a sound.
	 */
	bool bMute = false;

	/** Two profiles are equal when every parameter matches (determinism tests). */
	bool operator==(const FNpcVoiceProfile& Other) const;
};

struct GTC_UE5_API FNpcVoice
{
	/**
	 * The deterministic vocal signature for a persona `Voice` token (as carried
	 * on FNpcArchetype::Voice) and a per-citizen `IdentitySeed` (its spawn seed).
	 * Same inputs always yield the same profile. The persona sets the character
	 * (a philosopher is slow and low, an influencer fast and bright, a doomsday
	 * prepper urgent and rough, a mime mute); the seed spreads individuals across
	 * registers within that character so a crowd never sounds cloned. An unknown
	 * or empty token yields a neutral, seed-spread voice (never silent).
	 */
	static FNpcVoiceProfile ProfileFor(const FString& Voice, int32 IdentitySeed);

	/**
	 * Syllable estimate for a spoken line: counts vowel groups across alphabetic
	 * runs, at least one per word. Punctuation/whitespace are ignored. Empty or
	 * wordless input returns 0. Deterministic and locale-free (ASCII vowels +
	 * 'y'); good enough to pace lip-sync, not a linguistics engine.
	 */
	static int32 SyllableCount(const FString& Line);

	/**
	 * How long `Line` takes this voice to say, in seconds: syllables / rate plus
	 * a short beat per sentence-final mark (. ! ? …), clamped to a sane span so a
	 * one-word retort still gets a beat and a long ramble can't stall the world.
	 * Replaces the old char-count guess; honours the speaker's own tempo.
	 */
	static float EstimateSpokenSeconds(const FString& Line, const FNpcVoiceProfile& Profile);

	/**
	 * The center pitch (Hz) for one specific utterance, jittered within the
	 * profile's band by `LineSeed` so successive lines from the same person vary
	 * naturally (question lift, tired drop) without leaving their range. Returns
	 * BasePitchHz when the band is zero. Deterministic in (Profile, LineSeed).
	 */
	static float PitchHzForLine(const FNpcVoiceProfile& Profile, int32 LineSeed);
};
