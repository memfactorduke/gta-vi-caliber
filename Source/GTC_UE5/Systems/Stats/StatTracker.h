// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure lifetime-stats and 100%-completion model — the UE 5.7 port of the reference
 * StatTracker (RefCounted). The keyed counters open-world games track (kills, headshots, missions
 * passed, distance driven, ...) plus a few simple achievement thresholds derived from
 * them. Plain C++ value type (no UObject); a subsystem (StatsCoordinator) owns one,
 * pushes gameplay events into it via Add() and reads the totals / ratios / completion.
 * Headless-testable (reference behavior game/tests/unit/test_stat_tracker.gd).
 *
 * DEFERRED-OWNERSHIP (option-1 own-state): FStatTracker takes increments as INPUT — the
 * caller pushes kill/mission/distance/wanted events in via Add(). It never reaches into
 * any live gameplay system, never touches armor/health/PlayerStats; it is abstract
 * keyed counters. Wiring the actual event producers (kills, distance, jacks) is Wave 3.
 *
 * Parity note: AchievedList() / CompletionPercent() iterate achievements in table order,
 * so an ordered TArray of achievement rules (plus a TMap id->index) preserves that
 * observable order. Unknown stats read as 0 and negative increments are ignored, so the
 * numbers can be trusted. Values are double.
 */
class GTC_UE5_API FStatTracker
{
public:
    /** Metres per kilometre, for the DistanceKm() readout. */
    static constexpr double MetresPerKm = 1000.0;

    /** One achievement rule: earned the moment Stat reaches Threshold; never un-earned. */
    struct FAchievement
    {
        FString Id;
        FString Stat;
        double Threshold;
    };

    /** Achievement rules in table order (mirrors the the reference ACHIEVEMENTS Dictionary). */
    static const TArray<FAchievement>& Achievements();

    FStatTracker();

    // --- Mutators ----------------------------------------------------------

    /**
     * Increment a named stat by Amount (defaults to 1). Negative amounts are ignored,
     * so a miscounted event can never drive a total backwards. Mirrors the reference add().
     */
    void Add(const FString& StatId, double Amount = 1.0);

    /** Overwrite a stat outright (e.g. loading a value or a fastest-time record). */
    void SetStat(const FString& StatId, double Value);

    /** Clear every tracked stat. */
    void Reset();

    // --- Queries -----------------------------------------------------------

    /** Current value of a stat, or 0 if it was never touched. */
    double GetStat(const FString& StatId) const;

    /** A copy of every tracked stat (id->value), in first-seen order. */
    TArray<TPair<FString, double>> AllStats() const;

    /** True if any stat has been recorded (AllStats empty). */
    bool IsEmpty() const { return _Order.Num() == 0; }

    /**
     * Fraction of kills that were headshots, 0..1. 0 when there are no kills, so there
     * is never a divide-by-zero. Mirrors the reference headshot_ratio().
     */
    double HeadshotRatio() const;

    /** Total distance driven in kilometres, from the metre counter. */
    double DistanceKm() const;

    // --- Achievements ------------------------------------------------------

    /** True once the achievement's stat has reached its threshold. Unknown ids never. */
    bool IsAchieved(const FString& AchievementId) const;

    /** Every achievement earned so far, in table order. */
    TArray<FString> AchievedList() const;

    /** Share of achievements earned as a 0..100 percentage (100 when all earned). */
    double CompletionPercent() const;

    // --- Persistence (SaveManager) -----------------------------------------

    /**
     * Snapshot the stat store as {id: value} in first-seen order. Derived stats and
     * achievements are recomputed on restore. Mirrors the reference serialize()["stats"].
     */
    TArray<TPair<FString, double>> Serialize() const;

    /**
     * Rebuild from a Serialize() snapshot. Resets first, so missing data yields a clean
     * slate. Mirrors the reference restore() (a non-dictionary "stats" resets and returns).
     */
    void Restore(const TArray<TPair<FString, double>>& Stats);

private:
    /** Insertion-ordered stat values. */
    TArray<double> _Values;
    /** Insertion-ordered stat ids (parallel to _Values). */
    TArray<FString> _Order;
    /** Stat id -> index into _Values/_Order. */
    TMap<FString, int32> _Index;

    void Store(const FString& StatId, double Value);
};
