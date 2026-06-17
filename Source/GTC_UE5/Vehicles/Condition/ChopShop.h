// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

/**
 * Pure chop-shop / import-export valuation model — fence a (stolen) vehicle for cash priced by its
 * CLASS, its CONDITION (fed from FVehicleHealth::HealthFraction or FVehicleCondition::Condition), a
 * rotating "most-wanted" orders list that pays a demand bonus, and a heat discount if the car is hot.
 *
 * No nodes, no scene access: a chop-shop trigger owns one, reads the delivered vehicle's class +
 * condition, and applies the returned payout to the wallet — so the valuation/demand math stays
 * unit-tested headless (Tests/ChopShopTest.cpp). The orders list rotation takes an injected RNG so
 * it's deterministic.
 *
 * Each class is {id, base}; base is its pristine fence value. Malformed entries (missing/empty id,
 * non-positive base) are dropped.
 *
 * Parity notes:
 * - int32 payouts; int(round(...)) maps to FMath::RoundToInt with the float promotion preserved
 *   (the product is computed in double, matching Godot's float round()).
 * - Godot's Dictionary is insertion-ordered; Ids()/Requested() rely on it, so an ordered backing
 *   store (TArray + TMap index) mirrors that. Requests likewise keep insertion order.
 * - RNG: Godot's rotate_requests/_shuffle take a RandomNumberGenerator seeded by the caller and run
 *   a Fisher-Yates with rng.randi() % (i+1). UE uses FRandomStream. The oracle pins only
 *   determinism (same seed -> same requested set) and count/range, NOT a Godot-byte-identical
 *   sequence, so the shuffle is deterministic-per-seed but NOT byte-identical to Godot. A
 *   RotateRequestsNoRng() no-op mirrors Godot's rng == null early return.
 *
 * Deferred Wave-3 adapters (NOT implemented/tested here): the chop-shop trigger volume, the wallet
 * actor that banks the payout, and the most-wanted UI board.
 */
class GTC_UE5_API FChopShop
{
public:
    /** Even a totalled car is worth this fraction of base (scrap). */
    static constexpr double ScrapFloor = 0.2;
    /** Payout multiplier for a class that's currently on the most-wanted orders list. */
    static constexpr double RequestBonus = 1.5;
    /** Fraction shaved off when the delivered car is hot (freshly stolen / wanted). */
    static constexpr double HeatDiscount = 0.25;

    /** One catalogue row used to construct a price list. */
    struct FClassDef
    {
        FString Id;
        int32 Base = 0;
    };

    /** Outcome of a Deliver() call (mirrors the Godot result Dictionary). */
    struct FDeliverResult
    {
        bool bAccepted = false;
        int32 Payout = 0;
        bool bWasRequested = false;
        FString Reason;
    };

    /** Construct from a class price list; an empty list uses DefaultClasses(). */
    explicit FChopShop(const TArray<FClassDef>& Classes = TArray<FClassDef>());

    /** The built-in class price list (pristine fence values). */
    static TArray<FClassDef> DefaultClasses();

    int32 ClassCount() const;

    bool HasClass(const FString& Id) const;

    /** Registered class ids in insertion order. */
    TArray<FString> Ids() const;

    /** Pristine fence value of a class, or -1 if unknown. */
    int32 BaseValueOf(const FString& Id) const;

    /** Fence value for a class at Condition (0..1), demand-bonused if requested. 0 for unknown. */
    int32 Value(const FString& Id, double Condition) const;

    /** Whether a class is on the current most-wanted orders list. */
    bool IsRequested(const FString& Id) const;

    /** The current most-wanted class ids, in insertion order. */
    TArray<FString> Requested() const;

    /** Set the most-wanted orders directly (ignores unknown class ids). */
    void SetRequests(const TArray<FString>& ClassIds);

    /** Roll a fresh set of Count distinct most-wanted classes using Rng. Deterministic per seed. */
    void RotateRequests(FRandomStream& Rng, int32 Count);

    /** Null-rng path: no-op, mirroring Godot's rng == null early return. */
    void RotateRequestsNoRng();

    /** Fence a delivered vehicle. Pays Value() (heat-discounted if bHot), banks it, fulfils the order. */
    FDeliverResult Deliver(const FString& Id, double Condition, bool bHot = false);

    int32 TotalEarned() const;

    int32 DeliveriesCount() const;

private:
    struct FClassEntry
    {
        FString Id;
        int32 Base = 0;
    };

    /** Insertion-ordered class storage. */
    TArray<FClassEntry> Entries;
    /** Class id -> index into Entries. */
    TMap<FString, int32> Index;
    /** Most-wanted ids, in insertion order. */
    TArray<FString> Requests;

    int32 Earned = 0;
    int32 Deliveries = 0;

    void Register(const FClassDef& Def);
    const FClassEntry* Find(const FString& Id) const;

    /** Fisher-Yates in-place shuffle using the injected stream (deterministic per seed). */
    static void ShuffleIds(FRandomStream& Rng, TArray<FString>& Arr);
};
