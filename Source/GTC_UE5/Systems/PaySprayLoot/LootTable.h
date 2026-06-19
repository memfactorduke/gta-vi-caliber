// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

/**
 * Pure weighted loot/drop model for defeated enemies and smashed crates.
 *
 * Plain C++ value type (no UObject): a node (enemy, crate) owns one and asks it
 * for drops. All randomness goes through a caller-supplied FRandomStream so the
 * outcome is reproducible in tests; never the global rand.
 *
 * Each entry is an FLootEntry {Id, Weight, Min, Max}. The "empty" drop (Id == "")
 * represents "nothing dropped".
 *
 * RNG parity note: this uses FRandomStream, which is deterministic and
 * seed-reproducible WITHIN UE5 (a given seed replays the same sequence) — it is
 * NOT a Godot PCG32 reimplementation, so drops are NOT byte-identical to Godot.
 * Confirmed safe: no GTC system needs Godot-identical sequences and the save does
 * not serialize the RNG. Mapping: randf -> GetFraction(); randi_range(lo, hi) ->
 * RandRange(lo, hi) (inclusive both ends).
 */
struct GTC_UE5_API FLootEntry
{
    FString Id;
    float Weight = 0.0f;
    int32 Min = 0;
    int32 Max = 0;

    FLootEntry() = default;
    FLootEntry(const FString& InId, float InWeight, int32 InMin, int32 InMax)
        : Id(InId)
        , Weight(InWeight)
        , Min(InMin)
        , Max(InMax)
    {
    }
};

/** One rolled drop: an id and a quantity (quantity 0 for the empty drop). */
struct GTC_UE5_API FLootDrop
{
    FString Id;
    int32 Quantity = 0;
};

class GTC_UE5_API LootTable
{
public:
    /**
     * Normalised drop entries. Public so a test can force a truly empty table
     * (Entries.Empty()) to exercise the zero-weight fallback, mirroring the oracle.
     */
    TArray<FLootEntry> Entries;

    /** Construct from a raw table; an empty list uses DefaultTable(). */
    explicit LootTable(const TArray<FLootEntry>& Table = TArray<FLootEntry>());

    /** A sensible default drop table for a generic enemy/crate (total weight 15). */
    static TArray<FLootEntry> DefaultTable();

    /** Determinism helper: a fresh stream seeded with Seed. */
    static FRandomStream MakeRng(int32 Seed);

    /** Sum of all (non-negative) entry weights. */
    float TotalWeight() const;

    int32 EntryCount() const;

    /**
     * Pick one entry by weight and roll its quantity in [Min, Max] using Rng.
     * An empty / zero-weight table yields the empty drop.
     */
    FLootDrop Roll(FRandomStream& Rng) const;

    /** N independent rolls. */
    TArray<FLootDrop> RollMany(FRandomStream& Rng, int32 N) const;

    /**
     * Gate whether anything drops at all. Chance is clamped to [0, 1];
     * Chance >= 1 always drops, Chance <= 0 never does.
     */
    bool DropChanceSatisfied(FRandomStream& Rng, float Chance) const;

    /**
     * Expected value of one roll given a per-id unit value map:
     * sum over entries of (Weight/Total) * AvgQuantity * UnitValue(Id).
     */
    float ExpectedValue(const TMap<FString, float>& ValueOf) const;

private:
    /**
     * Normalise a raw table: floor weight at 0, fix swapped Min/Max. (Godot's
     * malformed-dict filtering is unrepresentable with a typed FLootEntry, so it
     * has no analogue here.)
     */
    static TArray<FLootEntry> Normalise(const TArray<FLootEntry>& Table);

    /** Roll the quantity for a chosen entry (empty id always yields quantity 0). */
    static FLootDrop RollEntry(const FLootEntry& Entry, FRandomStream& Rng);
};
