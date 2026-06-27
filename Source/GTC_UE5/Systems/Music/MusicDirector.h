// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The interactive score — the music that swells when the heat's on and tails out
 * when you slip away. The in-car radio (FRadioTuner) is DIEGETIC: a station the
 * player tunes. This is the other kind: the NON-diegetic score the game lays over
 * the action, the thing that stabs in on the first gunshot of a shootout and bleeds
 * back to ambience once you've lost the cops. Nothing drives that today. FMusicDirector
 * is it: gameplay reports a 0..1 "heat" each tick (cooked from wanted stars, an
 * active gunfight, a chase) and this turns it into a smoothed intensity and a discrete
 * stem LAYER the audio adapter crossfades.
 *
 * Two details make it feel scored rather than switched. The intensity uses an
 * ASYMMETRIC attack/release: it rushes UP toward the heat (the music hits hard the
 * instant things kick off) but bleeds DOWN slowly (it tails out instead of cutting),
 * so a brief lull mid-chase doesn't drop the drums. And the discrete layer is
 * quantized with HYSTERESIS: you cross UP into a hotter stem at a higher threshold
 * than you fall back DOWN out of it, so an intensity hovering on a boundary doesn't
 * chatter the layer on and off like a stuck thermostat.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject.
 * The same heat over the same deltas always yields the same intensity + layer —
 * unit-tested headless (Tests/MusicDirectorTest.cpp, prefix GTC.Systems.Music).
 *
 * PURE-CORE boundary: cooking the heat value from the wanted/combat/chase systems
 * and crossfading the actual music stems on a layer change is the DEFERRED adapter
 * and is NOT covered by the tests. Units: Heat and Intensity are 0..1; the attack/
 * release rates are per-second; Dt is seconds.
 */
struct FMusicDirector
{
    /** Tuning for how the score reacts. Defaults: snappy attack, slow release, 4 stems. */
    struct FParams
    {
        /** How fast intensity rises toward a higher heat (per second; >1 ~ near-instant). */
        double AttackPerSec = 3.0;
        /** How slowly intensity falls toward a lower heat (per second) — the tail-out. */
        double ReleasePerSec = 0.4;
        /** Number of stem layers (e.g. 4: ambient, tension, action, chaos). Min 1. */
        int32 LayerCount = 4;
        /** Hysteresis band around each layer boundary (0..1) to stop the layer chattering. */
        double Hysteresis = 0.06;
    };

    /** Install the tuning and reset to silence (intensity 0, layer 0). */
    void Configure(const FParams& InParams);

    /**
     * Advance one tick toward `Heat` (clamped [0,1]) over `Dt` seconds (clamped >= 0):
     * intensity eases up at the attack rate / down at the release rate, then the layer
     * is re-evaluated with hysteresis. Call once per frame and read Intensity()/Layer().
     */
    void Update(double Heat, double Dt);

    /** The smoothed score intensity this tick, 0..1. */
    double Intensity() const { return CurrentIntensity; }

    /** The active stem layer this tick, 0..LayerCount()-1. */
    int32 Layer() const { return CurrentLayer; }

    /** Number of stem layers configured (>= 1). */
    int32 LayerCount() const;

private:
    FParams Params;
    double CurrentIntensity = 0.0;
    int32 CurrentLayer = 0;
};
