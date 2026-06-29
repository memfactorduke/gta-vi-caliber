// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The 100%-completion ledger — the open-world pillar the game is missing. Missions
 * track their own completion, but nothing tracks the scattered world collectibles a
 * trailer-grade map lives on: the spray tags, the hidden packages, the photo
 * viewpoints, the buried caches. FCollectionTracker is the one place that knows how
 * many of each KIND exist, which ones you've actually found, and how close to 100%
 * the whole map is — the number a completionist chases and the reward tiers unlock
 * against.
 *
 * Each KIND is a named category with a known total (e.g. 50 spray tags). You
 * register the categories the map ships, then Collect(category, index) as the
 * player finds each item. The one correctness rule that matters is IDEMPOTENCE:
 * finding the same item twice — a save reload re-firing a pickup, a player
 * re-entering a trigger — must never push a count past its total or inflate the
 * percentage. So each category remembers the SET of indices found, not a bare
 * counter; Collect returns whether this was a NEW find (so the adapter knows
 * whether to play the reward sting) and silently ignores duplicates, unknown
 * categories, and out-of-range indices.
 *
 * Overall completion weights by raw item counts across categories — 5 of 50
 * viewpoints and 5 of 5 tags don't count the same — and crosses reward tiers at
 * 25 / 50 / 75 / 100% for the unlock layer to read.
 *
 * GTC-original pure-core: scene-free, deterministic, no UObject. The same finds in
 * any order yield the same totals — unit-tested headless
 * (Tests/CollectionTrackerTest.cpp, prefix GTC.Systems.Collection).
 *
 * PURE-CORE boundary: placing the collectible actors, mapping a world pickup to a
 * (category, index), the completion HUD/map, and granting the tier rewards is the
 * DEFERRED adapter and is NOT covered by the tests. Units: counts are item counts;
 * fractions/completion are 0..1.
 */
struct GTC_UE5_API FCollectionTracker
{
    /** Overall-completion reward tiers, crossed at 0 / 25 / 50 / 75 / 100%. */
    enum class EReward : uint8
    {
        None,     // under 25%
        Bronze,   // >= 25%
        Silver,   // >= 50%
        Gold,     // >= 75%
        Platinum, // 100% — the full map
    };

    /**
     * Register (or re-register) a collectible category with `Total` items. Re-defining
     * an existing category resets its found set and total. A non-positive Total makes
     * the category trivially complete and contributes nothing to the grand total.
     */
    void DefineCategory(const FString& CategoryId, int32 Total);

    /**
     * Mark item `ItemIndex` (0..Total-1) of `CategoryId` as found. Returns true only on
     * a NEW find; returns false (and changes nothing) for an unknown category, an
     * out-of-range index, or an item already found.
     */
    bool Collect(const FString& CategoryId, int32 ItemIndex);

    /** Number of categories registered. */
    int32 CategoryCount() const { return Categories.Num(); }

    /** Items found / total in a category (0 for an unknown category). */
    int32 FoundIn(const FString& CategoryId) const;
    int32 TotalIn(const FString& CategoryId) const;

    /** Completion of one category, 0..1 — 1 for a complete or zero-total category, 0 if unknown. */
    double FractionIn(const FString& CategoryId) const;

    /** True when a known category has every item (or has a non-positive total). */
    bool IsCategoryComplete(const FString& CategoryId) const;

    /** Items found across all categories. */
    int32 TotalFound() const;

    /** Items that exist across all categories (categories with non-positive totals add 0). */
    int32 GrandTotal() const;

    /** Overall completion, 0..1 — TotalFound / GrandTotal, and 1 when there's nothing to collect. */
    double Completion() const;

    /** True when every registered item has been found. */
    bool IsComplete() const;

    /** The reward tier earned by the current overall completion. */
    EReward Reward() const;

private:
    struct FCategory
    {
        FString Id;
        int32 Total = 0;
        TArray<int32> Found; // indices found, deduped — the SET that makes Collect idempotent
    };

    const FCategory* FindCategory(const FString& CategoryId) const;
    FCategory* FindCategory(const FString& CategoryId);

    TArray<FCategory> Categories;
};
