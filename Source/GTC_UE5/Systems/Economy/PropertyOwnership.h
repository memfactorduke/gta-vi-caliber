// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure property-ownership / passive-income model: the player buys safehouses and
 * businesses, which accrue income over game days into a pending bank collected by
 * visiting. Plain C++ value type (no UObject): a purchase resolves against a wallet
 * balance the caller passes in; income accrues on fed-in game days; Collect() hands
 * back banked cash. Headless-testable (parity oracle test_property_ownership.gd).
 *
 * Each catalogue entry is {Id, Name, Price, IncomePerDay, bIsSafehouse}. Garbage
 * entries (empty id, negative price/income) are dropped at construction.
 *
 * Parity note: owned ids are returned sorted (Godot sorts them). Pending income is
 * stored as double, carrying the sub-unit remainder across collections.
 */
struct GTC_UE5_API FPropertyDef
{
    FString Id;
    FString Name;
    int32 Price = 0;
    int32 IncomePerDay = 0;
    bool bIsSafehouse = false;

    FPropertyDef() = default;
    FPropertyDef(const FString& InId, const FString& InName, int32 InPrice, int32 InIncomePerDay, bool bInIsSafehouse)
        : Id(InId)
        , Name(InName)
        , Price(InPrice)
        , IncomePerDay(InIncomePerDay)
        , bIsSafehouse(bInIsSafehouse)
    {
    }
};

/** Result of a buy attempt — mirrors Godot's {success, cost, new_balance, reason}. */
struct GTC_UE5_API FPropertyBuyResult
{
    bool bSuccess = false;
    int32 Cost = 0;
    int32 NewBalance = 0;
    FString Reason;
};

/** Snapshot for save data: the owned ids (sorted) plus the pending bank. */
struct GTC_UE5_API FPropertySaveData
{
    TArray<FString> Owned;
    double Pending = 0.0;
};

class GTC_UE5_API PropertyOwnership
{
public:
    /** Construct from a catalogue; an empty list uses DefaultCatalogue(). */
    explicit PropertyOwnership(const TArray<FPropertyDef>& Catalogue = TArray<FPropertyDef>());

    /** The built-in stock used when an empty catalogue is passed. */
    static TArray<FPropertyDef> DefaultCatalogue();

    int32 PropertyCount() const;

    /** Price of a property, or -1 if the id is unknown. */
    int32 PriceOf(const FString& Id) const;

    /** Daily income a property yields (0 for unknown / safehouse). */
    int32 IncomeOf(const FString& Id) const;

    bool IsSafehouse(const FString& Id) const;

    bool Owns(const FString& Id) const;

    /** Sorted ids the player owns. */
    TArray<FString> OwnedIds() const;

    bool HasSafehouse() const;

    /** Id of the first owned safehouse (sorted), or "" if none. */
    FString NearestSafehouseOwned() const;

    /** Total money spent acquiring every owned property. */
    int32 TotalInvested() const;

    /** Combined daily income across every owned property. */
    int32 DailyIncome() const;

    /** Buy a property against a wallet balance; never mutates the wallet. */
    FPropertyBuyResult Buy(const FString& Id, int32 Balance);

    /** Accumulate income over DeltaDays into the pending bank. Non-finite / <= 0 ignored. */
    void Accrue(double DeltaDays);

    /** Income banked but not yet collected. */
    double PendingIncome() const;

    /** Withdraw whole-money pending income; carries the sub-unit remainder. */
    int32 Collect();

    /** Snapshot for save data. */
    FPropertySaveData Serialize() const;

    /** Rebuild from a Serialize() snapshot; unknown ids dropped. */
    void Restore(const FPropertySaveData& Data);

    /** Sell off everything and clear the pending bank. */
    void Reset();

private:
    /** Insertion-ordered catalogue. */
    TArray<FPropertyDef> Catalogue;
    /** Id -> index into Catalogue. */
    TMap<FString, int32> Index;
    /** Set of owned ids. */
    TSet<FString> OwnedSet;
    /** Income accrued from owned businesses but not yet collected. */
    double Pending = 0.0;

    static FPropertyBuyResult Fail(int32 Balance, const FString& Reason);

    void Register(const FPropertyDef& Entry);
    const FPropertyDef* Find(const FString& Id) const;
};
