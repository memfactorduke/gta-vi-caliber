// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Utility-AI need model for a single NPC — the "why" behind everything it does.
 *
 * Direct port of the Godot `NpcNeeds` (RefCounted) at
 * `game/scripts/npc/npc_needs.gd`. Five drives, each a satisfaction level in
 * [0, 1] (1 = fully sated, 0 = desperate). They decay over time at per-NPC
 * rates; activities top them back up. NpcMind reads these to decide when a
 * citizen abandons its scripted routine. Pure state + math, scene-free,
 * unit-tested headless (Tests/NpcNeedsTest.cpp, prefix
 * GTC.NPC.Decision.NpcNeeds).
 *
 * The canonical drive order (Needs()) is stable so seeds/tests stay
 * deterministic, and every scan iterates it in that order — insertion-order
 * parity with the Godot `values` Dictionary. Computed precision is `double` to
 * match GDScript.
 */
struct GTC_UE5_API FNpcNeeds
{
    /**
     * Canonical drive order. Index 0..N is stable so seeds/tests stay
     * deterministic. Mirrors the Godot `NEEDS` PackedStringArray.
     */
    static const TArray<FString>& Needs();

    /** name -> satisfaction in [0, 1]. Keyed by Needs() entries. */
    TMap<FString, double> Values;

    /**
     * Start every drive at `Initial` (clamped). Default: a content, well-rested
     * NPC.
     */
    explicit FNpcNeeds(double Initial = 1.0);

    /**
     * Drain each drive by its hourly rate x elapsed game-hours. Missing rate = 0
     * (that drive does not decay). Clamped at 0 so a starving NPC can't go
     * negative.
     */
    void Decay(double Hours, const TMap<FString, double>& Rates);

    /** Replenish one drive (e.g. eating restores "hunger"). Clamped at 1. */
    void Satisfy(const FString& Need, double Amount);

    /** How badly this drive wants attention: 0 (sated) .. 1 (desperate). */
    double Urgency(const FString& Need) const;

    /**
     * The single most-deprived drive, scanning in canonical order so ties resolve
     * deterministically toward the earlier need.
     */
    FString MostUrgent() const;

    /** Urgency of the most-deprived drive in [0, 1] — the brain's "interrupt" signal. */
    double PeakUrgency() const;
};
