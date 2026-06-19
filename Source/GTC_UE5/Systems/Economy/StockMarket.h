// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

/**
 * Pure equities-market model — a roster of tradeable companies whose share prices
 * react to in-world EVENTS, with a tracked portfolio. Plain C++ value type (no
 * UObject): trades never mutate the wallet; the caller applies the result.
 * Headless-testable (reference behavior test_stock_market.gd).
 *
 * Each company is {Id, Sector, BasePrice, Volatility}. Garbage entries (empty id,
 * non-positive price) are dropped at construction. Current price is BasePrice * a
 * per-company multiplier (starts 1.0) moved by events (scaled by volatility) and by
 * Fluctuate() drift, clamped to the band.
 *
 * Parity note: companies/holdings iterate in insertion order (Sectors(), portfolio
 * sums), so explicit ordered backing stores preserve that observable order. Average
 * cost is stored as double.
 *
 * RNG parity note: Fluctuate() drifts through FRandomStream — deterministic and
 * seed-reproducible WITHIN UE5, NOT byte-identical to Godot. The oracle only pins
 * determinism (same seed -> same price) and that zero-volatility holds, never an exact
 * seed->value sequence. Mapping: randf_range(-s, s) -> FRandRange(-s, s).
 */
class GTC_UE5_API StockMarket
{
public:
    /** Band a company's price multiplier can sit in. */
    static constexpr double MultiplierMin = 0.1;
    static constexpr double MultiplierMax = 8.0;
    /** How far a single Fluctuate() step may nudge a multiplier at volatility 1.0. */
    static constexpr double FluctuateSwing = 0.15;

    struct FCompanyDef
    {
        FString Id;
        FString Sector;
        int32 BasePrice = 0;
        double Volatility = 0.5;

        FCompanyDef() = default;
        FCompanyDef(const FString& InId, const FString& InSector, int32 InBasePrice, double InVolatility)
            : Id(InId)
            , Sector(InSector)
            , BasePrice(InBasePrice)
            , Volatility(InVolatility)
        {
        }
    };

    /** Result of a buy — mirrors {success, cost, new_balance, reason}. */
    struct FBuyResult
    {
        bool bSuccess = false;
        int32 Cost = 0;
        int32 NewBalance = 0;
        FString Reason;
    };

    /** Result of a sell — mirrors {success, proceeds, realized, reason}. */
    struct FSellResult
    {
        bool bSuccess = false;
        int32 Proceeds = 0;
        int32 Realized = 0;
        FString Reason;
    };

    /** Construct from a roster; an empty list uses DefaultCompanies(). */
    explicit StockMarket(const TArray<FCompanyDef>& Companies = TArray<FCompanyDef>());

    /** Built-in roster used when an empty list is passed. */
    static TArray<FCompanyDef> DefaultCompanies();

    int32 CompanyCount() const;
    bool HasCompany(const FString& Id) const;

    /** Every distinct sector, in first-seen order. */
    TArray<FString> Sectors() const;

    /** Base (event-neutral) price, or -1 if unknown. */
    int32 BasePrice(const FString& Id) const;
    /** Volatility in [0,1], or 0.0 if unknown. */
    double Volatility(const FString& Id) const;
    /** Sector, or "" if unknown. */
    FString SectorOf(const FString& Id) const;
    /** Current price multiplier (1.0 neutral), or 1.0 if unknown. */
    double Multiplier(const FString& Id) const;
    /** Current share price: base * multiplier, floored at 1. -1 for unknown. */
    int32 Price(const FString& Id) const;

    /** Move one company's price (volatility-scaled, clamped). False for unknown id. */
    bool ApplyCompanyEvent(const FString& Id, double Magnitude);
    /** Shock an entire sector. Returns the number of companies moved. */
    int32 ApplySectorEvent(const FString& Sector, double Magnitude);
    /** Shock a company that ripples to its sector rivals in the opposite direction. */
    bool ApplyRivalryShock(const FString& Id, double Magnitude, double Spillover);

    /** Resolve a buy against a wallet balance; never mutates the wallet. */
    FBuyResult Buy(const FString& Id, int32 Qty, int32 Balance);
    /** Sell shares the player holds at the current price; never mutates the wallet. */
    FSellResult Sell(const FString& Id, int32 Qty);

    /** Shares of a company currently held (0 if none / unknown). */
    int32 SharesHeld(const FString& Id) const;
    /** Quantity-weighted average price paid for the current position (0.0 if none). */
    double AvgCost(const FString& Id) const;

    /** Market value of the whole portfolio at current prices. */
    int32 PortfolioValue() const;
    /** Cost basis of everything currently held. */
    int32 TotalInvested() const;
    /** Paper P/L on open positions: value - cost basis. */
    int32 UnrealizedGain() const;
    /** Locked-in P/L from everything sold so far. */
    int32 RealizedGain() const;

    /** Nudge every company's multiplier; deterministic for a given seed. */
    void Fluctuate(FRandomStream& Rng, double Intensity);
    /** No-rng overload mirroring the reference `fluctuate(null, ...)`: a no-op. */
    void FluctuateNoRng(double Intensity);

    /** Determinism helper: a fresh stream seeded with Seed. */
    static FRandomStream MakeRng(int32 Seed);

private:
    struct FCompany
    {
        FString Id;
        FString Sector;
        int32 BasePrice = 0;
        double Volatility = 0.5;
        double MultiplierValue = 1.0;
    };

    struct FHolding
    {
        FString Id;
        int32 Qty = 0;
        double AvgCost = 0.0;
    };

    /** Insertion-ordered companies. */
    TArray<FCompany> Companies;
    /** Id -> index into Companies. */
    TMap<FString, int32> Index;
    /** Insertion-ordered holdings. */
    TArray<FHolding> Holdings;
    /** Id -> index into Holdings. */
    TMap<FString, int32> HoldingIndex;
    /** Running realized P/L from sells. */
    int32 Realized = 0;

    static FBuyResult FailBuy(int32 Balance, const FString& Reason);
    static FSellResult FailSell(const FString& Reason);

    void Register(const FCompanyDef& Entry);
    const FCompany* Find(const FString& Id) const;
    FCompany* Find(const FString& Id);
    const FHolding* FindHolding(const FString& Id) const;
    FHolding* FindHolding(const FString& Id);
    void EraseHolding(const FString& Id);

    /** Apply a volatility-scaled multiplicative move, clamped to band. */
    void Move(const FString& Id, double Magnitude);
};
