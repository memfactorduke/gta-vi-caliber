// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure operational business-empire model: owned ventures convert SUPPLIES into PRODUCT
 * over game time at a rate scaled by staff and an upgrade tier; the player cashes out
 * the stockpile at a per-unit price moving with demand and a police-heat discount.
 * Plain C++ value type (no UObject): every money op resolves against a wallet balance
 * the caller passes in and never mutates it. Headless-testable (parity oracle
 * test_business_venture.gd).
 *
 * Catalogue row: {Id, Name, ProductPerDay, ProductPerSupply, MaxProduct, SaleValue,
 * MaxStaff, MaxTier}. Rows with empty id or non-positive ProductPerDay / SaleValue are
 * dropped at construction.
 *
 * Parity note: catalogue/owned ventures iterate in insertion order; OwnedIds() is
 * sorted. Operational state (supply/product) is stored as double.
 */
struct GTC_UE5_API FVentureDef
{
    FString Id;
    FString Name;
    double ProductPerDay = 0.0;
    double ProductPerSupply = 1.0;
    double MaxProduct = 100.0;
    int32 SaleValue = 0;
    int32 MaxStaff = 5;
    int32 MaxTier = 3;

    FVentureDef() = default;
};

/** Result of acquire / buy_supplies / upgrade — mirrors {success, cost, new_balance, reason}. */
struct GTC_UE5_API FVentureMoneyResult
{
    bool bSuccess = false;
    int32 Cost = 0;
    int32 NewBalance = 0;
    FString Reason;
};

/** Result of a sell — mirrors {success, proceeds, sold, reason}. */
struct GTC_UE5_API FVentureSellResult
{
    bool bSuccess = false;
    int32 Proceeds = 0;
    int32 Sold = 0;
    FString Reason;
};

/** One owned venture's live operation state (for Serialize/Restore). */
struct GTC_UE5_API FVentureState
{
    FString Id;
    double Supply = 0.0;
    double Product = 0.0;
    int32 Staff = 0;
    int32 Tier = 0;
};

/** Snapshot for save data: owned operations (insertion-ordered) plus lifetime gross. */
struct GTC_UE5_API FVentureSaveData
{
    TArray<FVentureState> Owned;
    int32 Gross = 0;
    bool bValid = true;
};

class GTC_UE5_API BusinessVenture
{
public:
    static constexpr double DemandMin = 0.5;
    static constexpr double DemandMax = 2.0;
    static constexpr double HeatDiscount = 0.3;
    static constexpr double StaffGain = 0.5;
    static constexpr double TierGain = 0.35;
    static constexpr double DefaultProductPerSupply = 1.0;
    static constexpr double DefaultMaxProduct = 100.0;
    static constexpr int32 DefaultMaxStaff = 5;
    static constexpr int32 DefaultMaxTier = 3;
    static constexpr double MaxReasonableProduct = 1000000.0;
    static constexpr int32 MaxReasonableSaleValue = 10000000;

    /** Construct from a roster; an empty list uses DefaultVentures(). */
    explicit BusinessVenture(const TArray<FVentureDef>& Ventures = TArray<FVentureDef>());

    /** Built-in roster used when an empty array is passed. */
    static TArray<FVentureDef> DefaultVentures();

    int32 VentureCount() const;
    bool HasVenture(const FString& Id) const;
    /** Catalogue ids in first-seen order. */
    TArray<FString> Ids() const;

    bool Owns(const FString& Id) const;
    /** Owned ids, sorted. */
    TArray<FString> OwnedIds() const;

    double SupplyIn(const FString& Id) const;
    double ProductIn(const FString& Id) const;
    int32 StaffIn(const FString& Id) const;
    int32 TierIn(const FString& Id) const;

    /** Take over a venture against a wallet balance. */
    FVentureMoneyResult Acquire(const FString& Id, int32 Cost, int32 Balance);
    /** Restock raw materials, clamped so supply never exceeds the max_product ceiling. */
    FVentureMoneyResult BuySupplies(const FString& Id, int32 Units, int32 UnitCost, int32 Balance);
    /** Raise tier toward max_tier against the wallet. */
    FVentureMoneyResult Upgrade(const FString& Id, int32 Cost, int32 Balance);

    /** Hire one worker, bounded by max_staff. */
    bool Hire(const FString& Id);
    /** Lay off one worker, bounded at 0. */
    bool Fire(const FString& Id);

    /** Effective product/day; 0.0 if unowned or out of supply. */
    double ProductionRate(const FString& Id) const;
    /** Advance every owned venture by DeltaDays. Non-positive spans ignored. */
    void Accrue(double DeltaDays);

    /** Per-unit cash-out price scaled by clamped demand and docked by heat. 0 for unknown. */
    int32 SalePrice(const FString& Id, double Demand, double Heat) const;
    /** Cash out up to Units of accrued product; never mutates the wallet. */
    FVentureSellResult Sell(const FString& Id, int32 Units, double Demand, double Heat);

    /** Empire-wide stockpile across owned ventures (floored). */
    int32 TotalProduct() const;
    /** Lifetime gross revenue from all sales. */
    int32 GrossEarned() const;

    /** Snapshot for save data. */
    FVentureSaveData Serialize() const;
    /** Rebuild from a Serialize() snapshot; unknown ids dropped, fields clamped. */
    void Restore(const FVentureSaveData& Data);

private:
    struct FCatalogueRow
    {
        FString Id;
        FString Name;
        double ProductPerDay = 0.0;
        double ProductPerSupply = 1.0;
        double MaxProduct = 100.0;
        int32 SaleValue = 0;
        int32 MaxStaff = 5;
        int32 MaxTier = 3;
    };

    struct FOwnedOp
    {
        FString Id;
        double Supply = 0.0;
        double Product = 0.0;
        int32 Staff = 0;
        int32 Tier = 0;
    };

    /** Insertion-ordered catalogue. */
    TArray<FCatalogueRow> Catalogue;
    /** Id -> index into Catalogue. */
    TMap<FString, int32> Index;
    /** Insertion-ordered owned operations. */
    TArray<FOwnedOp> Owned;
    /** Id -> index into Owned. */
    TMap<FString, int32> OwnedIndex;
    /** Lifetime gross revenue. */
    int32 Gross = 0;

    static double StaffFactor(int32 Staff);
    static double TierFactor(int32 Tier);

    static FVentureMoneyResult Fail(int32 Balance, const FString& Reason);
    static FVentureSellResult SellFail(const FString& Reason);

    void Register(const FVentureDef& Entry);
    const FCatalogueRow* Find(const FString& Id) const;
    const FOwnedOp* FindOwned(const FString& Id) const;
    FOwnedOp* FindOwned(const FString& Id);
    void AddOwned(const FOwnedOp& Op);

    /** Supply level at which a venture could fill to max_product. */
    double SupplyCeiling(const FString& Id) const;
};
