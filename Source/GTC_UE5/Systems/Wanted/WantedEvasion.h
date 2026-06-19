// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * FWantedEvasion — pure "go cold" wanted-evasion state machine (UE 5.7 port of Godot
 * `wanted_evasion.gd`, class WantedEvasion, RefCounted). Parity oracle
 * game/tests/unit/test_wanted_evasion.gd (19 funcs).
 *
 * Models the open-world evasion lifecycle: while a cop has line of sight (or a crime is in
 * progress) you are VISIBLE; the moment you break sight a search countdown runs
 * (SEARCHING — the HUD flashes the stars); if it reaches zero unseen you go COLD and
 * the wanted level should clear. Re-sighting at any point snaps back to VISIBLE and
 * refills the timer.
 *
 * No scene access — a subsystem owns one and feeds it line-of-sight + delta. The
 * line-of-sight / AI Perception source that produces `bSeenByPolice` is a Wave-3
 * adapter and DEFERRED; here it is a plain bool driver argument. double precision for
 * float parity with the oracle.
 */
class GTC_UE5_API FWantedEvasion
{
public:
    /** Evasion phases. VISIBLE: seen/committing, timer full. SEARCHING: unseen, counting down. COLD: elapsed. */
    enum class EState : uint8
    {
        Visible,
        Searching,
        Cold
    };

    double SearchDuration = 12.0;

    /**
     * Construct with a search countdown duration (Godot _init default 12.0). A
     * zero/negative duration is clamped to 0.001 so the countdown is finite and
     * SearchProgress() can never divide by zero.
     */
    explicit FWantedEvasion(double Duration = 12.0)
        : SearchDuration(FMath::Max(Duration, 0.001))
    {
        _TimeLeft = SearchDuration;
    }

    EState State() const { return _State; }

    bool IsVisible() const { return _State == EState::Visible; }
    bool IsSearching() const { return _State == EState::Searching; }
    bool IsCold() const { return _State == EState::Cold; }

    /** Seconds remaining on the search countdown: full while VISIBLE, 0 when COLD. */
    double TimeLeft() const { return FMath::Clamp(_TimeLeft, 0.0, SearchDuration); }

    /**
     * 0.0 at the start of a search, ramping to 1.0 at COLD. 0.0 while VISIBLE.
     * Drives a HUD flash/needle; monotonic within one uninterrupted search.
     */
    double SearchProgress() const
    {
        if (_State == EState::Visible)
        {
            return 0.0;
        }
        if (_State == EState::Cold)
        {
            return 1.0;
        }
        const double Elapsed = SearchDuration - FMath::Clamp(_TimeLeft, 0.0, SearchDuration);
        return FMath::Clamp(Elapsed / SearchDuration, 0.0, 1.0);
    }

    /**
     * Core tick. bSeenByPolice true -> snap to VISIBLE and refill. false -> VISIBLE
     * starts the countdown, SEARCHING decrements it by delta and flips to COLD at zero.
     * COLD stays COLD until a reset/crime. Negative/zero delta is treated as no time.
     */
    void Update(bool bSeenByPolice, double Delta)
    {
        if (bSeenByPolice)
        {
            _State = EState::Visible;
            _TimeLeft = SearchDuration;
            return;
        }

        if (_State == EState::Cold)
        {
            return;
        }

        if (_State == EState::Visible)
        {
            // Just lost sight: begin searching from a full timer.
            _State = EState::Searching;
        }

        const double Step = FMath::Max(Delta, 0.0);
        _TimeLeft = FMath::Clamp(_TimeLeft - Step, 0.0, SearchDuration);
        if (_TimeLeft <= 0.0)
        {
            _TimeLeft = 0.0;
            _State = EState::Cold;
        }
    }

    /** Back to VISIBLE with a full timer (respawn, or any state reset). */
    void Reset()
    {
        _State = EState::Visible;
        _TimeLeft = SearchDuration;
    }

    /** A fresh crime keeps you hot even if momentarily unseen: force VISIBLE + refill. */
    void NotifyCrime()
    {
        _State = EState::Visible;
        _TimeLeft = SearchDuration;
    }

private:
    EState _State = EState::Visible;
    double _TimeLeft = 12.0;
};
