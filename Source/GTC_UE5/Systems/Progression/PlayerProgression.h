// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure respect/XP progression model: levels on a rising curve plus per-level
 * unlocks earned through play. Plain C++ value type (no UObject) — a subsystem
 * (ProgressionTracker) owns one and feeds it respect payouts. Headless-testable
 * (reference behavior game/tests/unit/test_player_progression.gd).
 *
 * Curve: leaving level L costs XP_PER_LEVEL_STEP * L; cumulative respect to first
 * reach level L is the triangular sum 100 * (L-1) * L / 2.
 *
 * Parity note: integer XP/level math is exact (int32). UNLOCK_TABLE is iterated in
 * the the reference insertion order via an ordered TArray of gate rows so IsUnlocked /
 * UnlocksAt match observable order. level_progress() is double.
 */
class GTC_UE5_API FPlayerProgression
{
public:
    static constexpr int32 StartLevel = 1;
    static constexpr int32 MaxLevel = 50;
    /** Per-level slope: leaving level `level` costs XpPerLevelStep * level. */
    static constexpr int32 XpPerLevelStep = 100;

    FPlayerProgression();

    // --- Mutators ----------------------------------------------------------

    /**
     * Earn respect. Rolls over into as many new levels as the payout funds (a big
     * bonus can jump several at once), capped at MaxLevel where surplus is dropped.
     * Negative amounts are ignored.
     */
    void AddXp(int32 Amount);

    void Reset();

    // --- Queries -----------------------------------------------------------

    int32 Level() const { return _Level; }

    /** Alias for respect-within-level; respect and xp are the same currency here. */
    int32 Xp() const { return _XpIntoLevel; }

    int32 XpIntoLevel() const { return _XpIntoLevel; }

    int32 TotalXp() const { return _TotalXp; }

    /** Cost to leave the current level (reach the next). 0 at max level. */
    int32 XpForNext() const;

    /** Progress through the current level in 0..1. 1.0 at max level. */
    double LevelProgress() const;

    bool IsMaxLevel() const { return _Level >= MaxLevel; }

    // --- Curve & unlocks ---------------------------------------------------

    /**
     * Cumulative respect needed to first reach `TargetLevel` from a fresh start:
     * the triangular sum of per-level costs. XpToReach(1) == 0.
     */
    static int32 XpToReach(int32 TargetLevel);

    /** Features that unlock exactly at `TargetLevel` (empty if none keyed there). */
    static TArray<FString> UnlocksAt(int32 TargetLevel);

    /** True if `FeatureId` is unlocked at the current level (any gate <= level). */
    bool IsUnlocked(const FString& FeatureId) const;

private:
    /** One unlock gate row, kept in the reference insertion order. */
    struct FUnlockGate
    {
        int32 GateLevel;
        TArray<FString> Features;
    };

    /** UNLOCK_TABLE in insertion order. */
    static const TArray<FUnlockGate>& UnlockTable();

    int32 _Level = StartLevel;
    /** Respect accumulated *within* the current level (resets to leftover on level-up). */
    int32 _XpIntoLevel = 0;
    /** Lifetime respect ever earned, for stat screens. */
    int32 _TotalXp = 0;
};
