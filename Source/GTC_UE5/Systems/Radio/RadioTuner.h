// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The in-vehicle radio dial — the deterministic half of M5's "radio: streaming
 * music channels in vehicles" (docs/ROADMAP.md). It answers the only question the
 * audio adapter actually needs each frame: given the dial position and how much
 * time has passed, WHICH station is the player listening to, WHICH track on that
 * station is playing, and HOW FAR into it are we. Picking the matching USoundBase,
 * seeking the audio component to TrackOffset, and crossfading on a station change
 * is the DEFERRED adapter's job; this core never touches a sound.
 *
 * The model is built around the one thing that makes a GTA-style radio feel alive:
 * every station is a LIVE broadcast on its own continuous clock, not a player you
 * pause. The world advances a single shared clock every frame (Advance), and each
 * station maps that clock onto its looping playlist through its own phase offset.
 * So when you tune away from a station and come back a minute later, you don't
 * resume where you left it — you rejoin the broadcast already a minute further on,
 * mid-track, exactly like a real dial. Two stations with different phases are
 * desynchronised for free, so the city never sounds like every car is playing the
 * same song in lockstep.
 *
 * The dial is a ring: stations 0..N-1 followed by a Radio Off slot, and Next /
 * Previous wrap around the whole ring (so scrolling past the last station lands on
 * Off, and once more wraps back to the first station). TuneTo jumps straight to a
 * station; an out-of-range index is treated as Off rather than an error, so the
 * adapter can hand through a saved dial position without bounds-checking it.
 *
 * GTC-original pure-core (no Godot oracle): scene-free, deterministic, double
 * precision, no UObject. The clock is the single source of truth, so the same
 * elapsed time always yields the same NowPlaying — that determinism is what lets
 * it be unit-tested headless (Systems/Radio/Tests/RadioTunerTest.cpp, prefix
 * GTC.Systems.Radio).
 *
 * PURE-CORE boundary: resolving a (StationIndex, TrackIndex) to an actual audio
 * asset, seeking/blending the UAudioComponent, and reading the dial from Enhanced
 * Input is the DEFERRED adapter and is NOT covered by the tests. Units: every
 * duration and offset is in seconds; the shared clock is monotonic seconds since
 * Configure.
 */
struct GTC_UE5_API FRadioTuner
{
    /** One broadcast channel: a looping playlist plus its own head-start on the clock. */
    struct FStation
    {
        /** Per-track lengths in seconds, in broadcast order; the playlist loops forever.
         *  Zero-or-negative-length entries are skipped (never reported as NowPlaying) so
         *  a malformed lineup degrades gracefully instead of dividing by zero. */
        TArray<double> TrackSeconds;

        /** Seconds of head-start on the shared clock. Two stations with different phases
         *  sit at different points in their playlists at the same world time, so the city
         *  doesn't play every station in lockstep. May exceed the playlist length; it is
         *  folded modulo the total just like the clock. */
        double PhaseSeconds = 0.0;
    };

    /** What the audio adapter seeks to when the dial is on a music station. */
    struct FNowPlaying
    {
        /** Index into the station's TrackSeconds of the track on air right now. */
        int32 TrackIndex = INDEX_NONE;
        /** Seconds elapsed into that track, in [0, TrackSeconds[TrackIndex]). */
        double TrackOffset = 0.0;
    };

    /** Selection value returned by Station() when the dial is on Radio Off. */
    static constexpr int32 OffStation = INDEX_NONE;

    /**
     * Replace the station lineup and reset the dial to Off and the clock to 0.
     * Call once when the lineup is known (e.g. at world load); the adapter then
     * just Advances and tunes. Stations are taken by value so the caller's array
     * can be moved in.
     */
    void Configure(TArray<FStation> InStations);

    /**
     * Advance the shared broadcast clock by Dt seconds. Call once per frame with
     * the frame delta; a negative Dt is ignored (the clock never runs backwards),
     * so a paused or rewound game can't desync the dial. Every station advances
     * together because they all read this one clock.
     */
    void Advance(double Dt);

    /** Scroll the dial one step forward around the ring (… → station N-1 → Off → station 0 → …). */
    void Next();

    /** Scroll the dial one step backward around the ring. */
    void Previous();

    /**
     * Jump the dial straight to StationIndex. An index outside [0, NumStations())
     * — including OffStation — tunes to Radio Off, so a restored/garbage dial
     * position is always safe.
     */
    void TuneTo(int32 StationIndex);

    /** Turn the dial to Radio Off without changing the clock (stations keep broadcasting). */
    void TuneOff();

    /** True when the dial is on a music station (not Radio Off). */
    bool IsOn() const { return SelectedStation != OffStation; }

    /** The tuned station index, or OffStation when off. */
    int32 Station() const { return SelectedStation; }

    /** Number of configured stations (the Off slot is not counted). */
    int32 NumStations() const { return Stations.Num(); }

    /** Seconds elapsed on the shared broadcast clock since Configure. */
    double Clock() const { return ClockSeconds; }

    /**
     * The track + offset on air for the tuned station right now, derived from the
     * shared clock and the station's phase. When the dial is Off, or the tuned
     * station has no playable (positive-length) track, TrackIndex is INDEX_NONE
     * and TrackOffset is 0 — the adapter reads that as "play nothing".
     */
    FNowPlaying NowPlaying() const;

private:
    TArray<FStation> Stations;
    int32 SelectedStation = OffStation;
    double ClockSeconds = 0.0;
};
