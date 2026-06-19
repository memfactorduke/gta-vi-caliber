// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure auditory-perception model — the "ear" of the world. Decides which NPCs
 * actually HEAR a positional sound event (gunshot, alarm, engine, explosion,
 * footstep, shout, glass break) given each source's loudness, XZ-plane distance
 * falloff, and the ambient noise floor that masks quiet sounds. Returns a
 * perceived intensity (0..1) per listener and a discrete reaction tier
 * (Unheard / Noticed / Alarmed), so a world/AI controller can route alertness
 * without any scene access.
 *
 * Direct port of the the reference `SoundPropagation` (RefCounted) at
 * `game/scripts/systems/sound_propagation.gd`. All static, position/level-in,
 * scalar/struct-out, no UObject — unit-tested headless via the reference behavior
 * (Tests/SoundPropagationTest.cpp, prefix GTC.Systems.SoundPropagation).
 *
 * Double precision throughout, to match the the reference implementation float math. Spatial math is
 * on the XZ plane (the reference Y-up); height (Y) is ignored — two listeners at equal XZ
 * distance but different height read identically.
 *
 * NOTE: no Godot->UE Z-up axis remap is baked in here — the model stays in the
 * the reference XZ frame so the ported unit tests match the oracle literally. Porting the
 * axis convention to UE's Z-up space is a DEFERRED Wave-3 concern.
 *
 * PURE-CORE boundary: this is the pure auditory MATH only (per-kind loudness,
 * distance attenuation, ambient masking, audibility/reaction gating, and the
 * immutable fan-out / target-pick over caller-supplied listener and event lists).
 * The AI Perception "Hearing" integration — registering sound stimuli in the
 * world and querying what an NPC perceives via the perception system — is a
 * DEFERRED Wave-3 adapter: NOT implemented here and NOT covered by the parity
 * tests. The adapter would feed real listener positions / ambient floor into
 * these helpers and route the resulting reaction tiers into the perception/AI
 * stack.
 */

/** Categorical event kinds a spawner maps real actions onto. the reference ordinal order. */
enum class ESoundKind : uint8
{
    Gunshot,
    SilencedShot,
    Explosion,
    Alarm,
    CarHorn,
    Engine,
    Footstep,
    Shout,
    GlassBreak,
};

/** Discrete listener response tier. the reference ordinal order. */
enum class ESoundReaction : uint8
{
    Unheard,
    Noticed,
    Alarmed,
};

struct GTC_UE5_API FSoundPropagation
{
    /**
     * Distance falloff reference (metres-squared): perceived loudness halves once
     * the squared XZ distance reaches this. Larger = sound carries further.
     */
    static constexpr double FalloffReferenceSq = 400.0;
    /** Perceived intensity at/above which a listener registers the sound at all. */
    static constexpr double NoticedAt = 0.08;
    /** An alarming sound (gunshot, explosion...) reaches Alarmed at this intensity. */
    static constexpr double AlarmedAtAlarming = 0.25;
    /** An ambient sound (engine, horn...) needs to be much louder to alarm. */
    static constexpr double AlarmedAtAmbient = 0.5;
    /** Default audibility threshold (mirrors NoticedAt so "audible" == "not unheard"). */
    static constexpr double DefaultAudible = NoticedAt;
    /** Night drops the ambient floor by this factor (quiet streets carry sound). */
    static constexpr double NightQuietFactor = 0.5;
    /** Heavy rain adds up to this much to the ambient floor (weather masks sound). */
    static constexpr double RainMaskGain = 0.3;

    /**
     * A listener for Emit(): a position plus an optional caller id, carried through
     * intact into the result with {Intensity, Reaction, bHeard} filled in.
     */
    struct FListener
    {
        FVector Pos = FVector::ZeroVector;
        FString Id;

        FListener() = default;
        explicit FListener(const FVector& InPos, const FString& InId = FString())
            : Pos(InPos)
            , Id(InId)
        {
        }
    };

    /** Per-listener perception result from Emit(). */
    struct FListenerResult
    {
        FVector Pos = FVector::ZeroVector;
        FString Id;
        double Intensity = 0.0;
        ESoundReaction Reaction = ESoundReaction::Unheard;
        bool bHeard = false;
    };

    /** A concurrent sound event for LoudestHeard(): a position and a kind. */
    struct FSoundEvent
    {
        FVector Pos = FVector::ZeroVector;
        ESoundKind Kind = ESoundKind::Gunshot;

        FSoundEvent() = default;
        FSoundEvent(const FVector& InPos, ESoundKind InKind)
            : Pos(InPos)
            , Kind(InKind)
        {
        }
    };

    /**
     * The loudest event reaching a listener, from LoudestHeard(). bHeard == false
     * (the empty `{}` in the reference) when nothing clears the audibility threshold.
     */
    struct FHeardEvent
    {
        bool bHeard = false;
        FVector Pos = FVector::ZeroVector;
        ESoundKind Kind = ESoundKind::Gunshot;
        double Intensity = 0.0;
        ESoundReaction Reaction = ESoundReaction::Unheard;
    };

    /** Built-in loudness (0..1) for a sound kind. */
    static double BaseLoudness(ESoundKind Kind);

    /**
     * Built-in loudness for a raw kind ordinal; an unknown/out-of-range ordinal
     * returns 0.0 (mirrors the the reference `base_loudness(999) == 0.0`).
     */
    static double BaseLoudnessForInt(int32 KindRaw);

    /**
     * True for kinds that spike fear/alertness; false for ambient kinds (a
     * suppressed shot is deliberately not alarming — the point of a suppressor).
     */
    static bool IsAlarming(ESoundKind Kind);

    /**
     * Perceived intensity (0..1) of a sound at a listener: loudness attenuated by
     * inverse-square-ish XZ distance falloff, then masked by the ambient floor
     * (subtractive), clamped. Height (Y) is ignored.
     */
    static double PerceivedIntensity(
        const FVector& SourcePos, const FVector& ListenerPos, double Loudness, double Ambient);

    /** Whether the sound is at/above the audibility threshold at the listener. */
    static bool IsAudible(
        const FVector& SourcePos,
        const FVector& ListenerPos,
        double Loudness,
        double Ambient,
        double Threshold = DefaultAudible);

    /**
     * Max XZ distance at which a sound of this loudness stays above HearingFloor
     * (for spawn culling / broad-phase). Loudness or floor <= 0, or a sound already
     * below the floor at the source, returns 0.0.
     */
    static double AudibleRadius(double Loudness, double HearingFloor);

    /**
     * Map a perceived intensity to Unheard / Noticed / Alarmed. An alarming sound
     * reaches Alarmed at a lower intensity than an ambient one.
     */
    static ESoundReaction ReactionFor(double Intensity, bool bAlarming);

    /**
     * Fan one event out to many listeners. Each listener is carried through intact
     * (Pos, Id) with {Intensity, Reaction, bHeard} added. Returns a NEW array — the
     * input is never mutated.
     */
    static TArray<FListenerResult> Emit(
        const FVector& SourcePos, ESoundKind Kind, const TArray<FListener>& Listeners, double Ambient);

    /**
     * Given many concurrent events, return the one that reaches this listener at the
     * highest perceived intensity (with {Intensity, Reaction} filled) — for picking
     * what an idle NPC turns toward. bHeard == false if none are audible.
     */
    static FHeardEvent LoudestHeard(
        const FVector& ListenerPos, const TArray<FSoundEvent>& Events, double Ambient);

    /**
     * Derive the ambient masking floor from time-of-day and weather: night is
     * quieter (sound carries further), rain raises the floor (sound is masked).
     */
    static double AmbientFor(double BaseAmbient, bool bIsNight, double Rain01);
};
