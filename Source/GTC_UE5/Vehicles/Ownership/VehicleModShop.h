// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure tuning-garage model — the GTC mod shop where money buys tiered performance and
 * armor upgrades that modify a vehicle's stats.
 *
 * STATEFUL per-vehicle instance: one VehicleModShop holds the upgrade levels for a
 * single vehicle. A vehicle starts stock (every category at level 0) and each purchase
 * steps one category up by one tier. No nodes, no wallet coupling: an upgrade resolves
 * against a balance the caller passes in and returns the result so the caller applies
 * the spend — same headless, unit-testable pattern as PaySpray / FactionStanding.
 *
 * Catalogue shape: category -> array of per-tier prices. PriceTiers[i] is the cost to
 * go from level i to level i+1, so MaxLevel == PriceTiers.Num().
 *
 * Stat multipliers are derived purely from current levels (1.0 at stock, rising
 * monotonically): engine -> +8% top speed AND +6% accel/level, tires -> +7% grip/level,
 * brakes -> +10% brake/level, armor -> +25% armor/level. A category only feeds its own
 * stat(s); untouched categories leave their multiplier at exactly 1.0.
 *
 * No RNG; fully deterministic.
 *
 * Parity notes:
 * - Godot's Dictionary is insertion-ordered and Categories()/TotalSpent() rely on it.
 *   TMap does NOT preserve order, so an explicit ordered backing store (TArray<FEntry>
 *   + TMap<FString,int32> index) is used, mirroring FactionStanding.
 * - Godot's _clean_tiers also filters malformed non-int tier values; in typed C++ a
 *   tier is already an int32, so only the "strictly positive" rule survives (same
 *   LootTable-style note: the dynamic-type guard has no typed-C++ analogue).
 */
class GTC_UE5_API VehicleModShop
{
public:
    /** Per-level stat steps (fraction added to the 1.0 base for each level owned). */
    static constexpr double EngineSpeedStep = 0.08;
    static constexpr double EngineAccelStep = 0.06;
    static constexpr double TiresGripStep = 0.07;
    static constexpr double BrakesStep = 0.10;
    static constexpr double ArmorStep = 0.25;

    /** One catalogue row: a category name and its ordered per-tier prices. */
    struct FCategoryDef
    {
        FString Category;
        TArray<int32> PriceTiers;

        FCategoryDef() = default;
        FCategoryDef(const FString& InCategory, const TArray<int32>& InTiers)
            : Category(InCategory)
            , PriceTiers(InTiers)
        {
        }
    };

    /** Outcome of an Upgrade attempt. */
    struct FModUpgradeResult
    {
        bool bSuccess = false;
        int32 Cost = 0;
        int32 NewBalance = 0;
        int32 NewLevel = -1;
        FString Reason;
    };

    /** Construct from a catalogue; an empty list uses DefaultCatalogue(). */
    explicit VehicleModShop(const TArray<FCategoryDef>& Catalogue = TArray<FCategoryDef>());

    /** Built-in tuning catalogue: four categories (engine/brakes/armor/tires), three tiers each. */
    static TArray<FCategoryDef> DefaultCatalogue();

    /** Categories this shop sells, in insertion order. */
    TArray<FString> Categories() const;

    bool HasCategory(const FString& Category) const;

    /** Current installed level of a category (0 if stock / unknown). */
    int32 LevelOf(const FString& Category) const;

    /** Highest level a category can reach (0 if unknown). */
    int32 MaxLevel(const FString& Category) const;

    /**
     * Price to install NextLevel of a category (NextLevel is the level you'd own after
     * the buy, 1-based), or -1 if maxed / unknown / out-of-range.
     */
    int32 PriceFor(const FString& Category, int32 NextLevel) const;

    /** True when the category exists and isn't already at max level. */
    bool CanUpgrade(const FString& Category) const;

    /** Buy the next tier of a category against a wallet balance. Mutates level on success. */
    FModUpgradeResult Upgrade(const FString& Category, int32 Balance);

    /** Top-speed multiplier from engine level (1.0 stock, +8% per engine level). */
    double TopSpeedMultiplier() const;

    /** Acceleration multiplier from engine level (1.0 stock, +6% per engine level). */
    double AccelerationMultiplier() const;

    /** Brake multiplier from brakes level (1.0 stock, +10% per brakes level). */
    double BrakeMultiplier() const;

    /** Armor multiplier from armor level (1.0 stock, +25% per armor level). */
    double ArmorMultiplier() const;

    /** Grip multiplier from tires level (1.0 stock, +7% per tires level). */
    double GripMultiplier() const;

    /** Total money sunk into this vehicle: every tier price up to each category's level. */
    int32 TotalSpent() const;

    /** True when no upgrades have been installed (every category still at level 0). */
    bool IsStock() const;

    /** Snapshot of installed levels for saving, in insertion order. */
    TArray<TPair<FString, int32>> Serialize() const;

    /** Restore installed levels from a Serialize() snapshot; unknown ignored, clamped to [0, MaxLevel]. */
    void Restore(const TArray<TPair<FString, int32>>& State);

    /** Strip every upgrade back to stock (all categories to level 0). */
    void Reset();

private:
    struct FEntry
    {
        FString Category;
        TArray<int32> PriceTiers;
        int32 Level = 0;
    };

    /** Insertion-ordered storage. */
    TArray<FEntry> Entries;
    /** Category -> index into Entries. */
    TMap<FString, int32> Index;

    void Register(const FCategoryDef& Def);

    /** Reject the whole row if any tier <= 0 (Godot _clean_tiers); returns true if accepted. */
    static bool CleanTiers(const TArray<int32>& Tiers, TArray<int32>& OutClean);

    const FEntry* Find(const FString& Category) const;
    FEntry* Find(const FString& Category);
};
