// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

/**
 * Pure black-market / contraband trading model — buy goods low in one district and
 * sell high in another, with per-district price multipliers driving arbitrage and
 * carried contraband raising bust risk. Plain C++ value type (no UObject):
 * buys/sells never mutate the wallet; the caller applies the result. Headless-testable
 * (reference behavior test_contraband_market.gd).
 *
 * Each good is {Id, BasePrice}. Garbage entries (empty id, non-positive price) are
 * dropped at construction. Per-district price = BasePrice * a district multiplier
 * derived from a STABLE HASH of the district id (so every district pays a fixed,
 * different rate without a hand-maintained table) and drifting via Fluctuate().
 *
 * Parity note: the reference uses its built-in hash(); we use a deterministic FNV-1a string
 * hash instead. The oracle never pins exact hash values — it derives expected prices
 * from MultiplierFor() itself and only asserts band membership and that distinct
 * districts differ — so any stable distinct mapping satisfies it.
 *
 * RNG parity note: Fluctuate() drifts through FRandomStream — deterministic and
 * seed-reproducible WITHIN UE5, NOT byte-identical to Godot. The oracle only pins
 * determinism (same seed -> same drift) and that drift shifts the multiplier, never
 * an exact seed->value sequence. Mapping: randf_range(-s, s) -> FRandRange(-s, s).
 */
class GTC_UE5_API ContrabandMarket
{
public:
    /** Multiplier band a district's price can sit in. */
    static constexpr double MultiplierMin = 0.6;
    static constexpr double MultiplierMax = 1.6;
    /** How far a single Fluctuate() step may nudge a multiplier (scaled by volatility). */
    static constexpr double FluctuateSwing = 0.25;

    struct FGoodDef
    {
        FString Id;
        int32 BasePrice = 0;

        FGoodDef() = default;
        FGoodDef(const FString& InId, int32 InBasePrice)
            : Id(InId)
            , BasePrice(InBasePrice)
        {
        }
    };

    /** Result of a buy attempt — mirrors {success, cost, new_balance, reason}. */
    struct FBuyResult
    {
        bool bSuccess = false;
        int32 Cost = 0;
        int32 NewBalance = 0;
        FString Reason;
    };

    /** Construct from a catalogue; an empty list uses DefaultGoods(). */
    explicit ContrabandMarket(const TArray<FGoodDef>& Goods = TArray<FGoodDef>());

    /** Built-in stock used when an empty list is passed. */
    static TArray<FGoodDef> DefaultGoods();

    int32 GoodsCount() const;
    bool HasGood(const FString& Good) const;

    /** Base (district-neutral) price of a good, or -1 if unknown. */
    int32 BasePrice(const FString& Good) const;

    /** Stable per-district multiplier in band; empty id falls back to 1.0. */
    double MultiplierFor(const FString& DistrictId);

    /** Price of a good in a district: BasePrice * multiplier, rounded. Unknown -> -1. */
    int32 PriceIn(const FString& Good, const FString& DistrictId);

    /** Resolve a buy against a wallet balance; never mutates the wallet. */
    FBuyResult Buy(const FString& Good, int32 Qty, const FString& DistrictId, int32 Balance);

    /** Revenue from selling qty in a district. Unknown / non-positive qty -> 0. */
    int32 Sell(const FString& Good, int32 Qty, const FString& DistrictId);

    /** Of the supplied districts, which pays the most for a good. Unknown / empty -> "". */
    FString BestMarket(const FString& Good, const TArray<FString>& DistrictIds);

    /** Arbitrage profit buying in BuyDistrict, selling in SellDistrict (can be negative). */
    int32 Profit(const FString& Good, const FString& BuyDistrict, const FString& SellDistrict, int32 Qty);

    /** Add contraband to what the player carries. Non-positive / unknown are no-ops. */
    void Carry(const FString& Good, int32 Qty);
    /** How much of a good the player carries (0 if none / unknown). */
    int32 Carried(const FString& Good) const;
    /** Remove carried contraband, floored at 0. Non-positive / unknown are no-ops. */
    void Drop(const FString& Good, int32 Qty);
    /** Total units of all contraband on the player. */
    int32 TotalCarried() const;

    /** Bust risk: base plus a load penalty, clamped [0,1]. */
    static double BustRisk(int32 Total, double BaseRisk);

    /** Nudge every known district's multiplier; deterministic for a given seed. */
    void Fluctuate(FRandomStream& Rng, double Volatility);
    /** No-rng overload mirroring the reference `fluctuate(null, ...)`: a no-op. */
    void FluctuateNoRng(double Volatility);

    /** Determinism helper: a fresh stream seeded with Seed. */
    static FRandomStream MakeRng(int32 Seed);

private:
    /** Insertion-ordered goods. */
    TArray<FGoodDef> Goods;
    /** Id -> index into Goods. */
    TMap<FString, int32> Index;
    /** district_id -> multiplier; lazily seeded then mutated by Fluctuate(). Insertion-ordered. */
    TArray<FString> MultiplierOrder;
    TMap<FString, double> Multipliers;
    /** good -> qty carried. */
    TMap<FString, int32> CarriedMap;

    static FBuyResult Fail(int32 Balance, const FString& Reason);

    void Register(const FGoodDef& Entry);
    const FGoodDef* Find(const FString& Good) const;

    /** Map a district id's stable hash onto the multiplier band. */
    static double SeedMultiplier(const FString& DistrictId);
    /** Deterministic 32-bit FNV-1a hash of a string. */
    static uint32 StableHash(const FString& Value);
};
