// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure assassination-contract board — each contract names a target, pays a reward,
 * and on completion emits a STOCK-MARKET SHOCK the caller applies to a StockMarket
 * (the "do the hit, move the market" loop). Plain C++ value type (no UObject),
 * deliberately decoupled from StockMarket: Complete() returns a {CompanyId, Magnitude,
 * Spillover} effect descriptor the caller feeds to StockMarket::ApplyRivalryShock.
 * Headless-testable (parity oracle test_hit_contract.gd).
 *
 * A contract is {Id, Target, CompanyId, Magnitude, Spillover, Reward, Difficulty}.
 * Malformed entries (empty id, non-positive reward) are dropped at construction. One
 * contract is active at a time; finished ones leave the available pool.
 *
 * Parity note: contracts iterate in insertion order (Ids()/Available()), so an
 * explicit ordered backing store preserves that observable order.
 */
struct GTC_UE5_API FHitContractDef
{
    FString Id;
    FString Target;
    FString CompanyId;
    double Magnitude = 0.0;
    double Spillover = 0.0;
    int32 Reward = 0;
    int32 Difficulty = 1;

    FHitContractDef() = default;
};

/** Market shock a contract produces — feed to StockMarket::ApplyRivalryShock. */
struct GTC_UE5_API FMarketEffect
{
    bool bValid = false;
    FString CompanyId;
    double Magnitude = 0.0;
    double Spillover = 0.0;
};

/** Result of completing a contract — mirrors {success, reward, market_effect, reason}. */
struct GTC_UE5_API FHitCompleteResult
{
    bool bSuccess = false;
    int32 Reward = 0;
    FMarketEffect MarketEffect;
    FString Reason;
};

class GTC_UE5_API HitContract
{
public:
    /** Construct from a board; an empty list uses DefaultContracts(). */
    explicit HitContract(const TArray<FHitContractDef>& Contracts = TArray<FHitContractDef>());

    /** Built-in board, tied to StockMarket's default roster. */
    static TArray<FHitContractDef> DefaultContracts();

    int32 ContractCount() const;
    bool HasContract(const FString& Id) const;

    /** Every contract id, in first-seen order. */
    TArray<FString> Ids() const;

    /** Reward a contract pays, or -1 for an unknown id. */
    int32 RewardOf(const FString& Id) const;

    /** Display target for a contract, or "" for an unknown id. */
    FString TargetOf(const FString& Id) const;

    /** The market shock a contract produces (invalid for an unknown id). */
    FMarketEffect MarketEffectOf(const FString& Id) const;

    /** Contract ids still takeable: not completed and not currently active. */
    TArray<FString> Available() const;

    /** Take a contract. Fails on unknown / completed / while another is active. */
    bool Accept(const FString& Id);

    /** The active contract id, or "". */
    FString Active() const;
    bool HasActive() const;

    /** Drop the active contract back into the pool. Returns the abandoned id, or "". */
    FString Abandon();

    /** Complete the active contract: bank reward, mark done, return the market shock. */
    FHitCompleteResult Complete();

    bool IsCompleted(const FString& Id) const;
    int32 CompletedCount() const;

    /** Lifetime reward cash from completed hits. */
    int32 TotalEarned() const;

private:
    /** Insertion-ordered contracts. */
    TArray<FHitContractDef> Contracts;
    /** Id -> index into Contracts. */
    TMap<FString, int32> Index;
    /** Currently accepted contract id, or "". */
    FString ActiveId;
    /** Set of completed contract ids. */
    TSet<FString> Completed;
    /** Lifetime reward cash earned. */
    int32 Earned = 0;

    void Register(const FHitContractDef& Entry);
    const FHitContractDef* Find(const FString& Id) const;
};
