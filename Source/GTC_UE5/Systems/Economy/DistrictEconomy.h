// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure living real-estate model — each district carries a desirability the world
 * moves: turf control and local-business investment lift it, recent crime heat drags
 * it down. Desirability scales property values and passive income; district ids line
 * up with GangTerritory so a caller can feed InfluenceIn() into SetControl(). Plain
 * C++ value type (no UObject). Headless-testable (parity oracle test_district_economy.gd).
 *
 * Each district is {Id, Base}; Base is its baseline desirability index. Malformed
 * entries (empty id, non-positive base) are dropped at construction.
 *
 * Parity note: districts iterate in insertion order (Ids()), so an explicit ordered
 * backing store preserves that observable order.
 */
struct GTC_UE5_API FDistrictDef
{
    FString Id;
    double Base = 1.0;

    FDistrictDef() = default;
    FDistrictDef(const FString& InId, double InBase)
        : Id(InId)
        , Base(InBase)
    {
    }
};

class GTC_UE5_API DistrictEconomy
{
public:
    /** Desirability shift per unit of player control. */
    static constexpr double ControlWeight = 0.3;
    /** Desirability shift per owned local business, up to InvestCap. */
    static constexpr double InvestWeight = 0.1;
    static constexpr int32 InvestCap = 3;
    /** Desirability drop at full crime heat. */
    static constexpr double HeatWeight = 0.4;
    /** Desirability band. */
    static constexpr double DesirMin = 0.4;
    static constexpr double DesirMax = 2.0;

    /** Construct from a roster; an empty list uses DefaultDistricts(). */
    explicit DistrictEconomy(const TArray<FDistrictDef>& Districts = TArray<FDistrictDef>());

    /** Built-in roster, ids aligned with GangTerritory.default_districts(). */
    static TArray<FDistrictDef> DefaultDistricts();

    int32 DistrictCount() const;
    bool HasDistrict(const FString& Id) const;

    /** Every district id, in first-seen order. */
    TArray<FString> Ids() const;

    /** Baseline desirability index, or -1.0 if unknown. */
    double BaseIndex(const FString& Id) const;

    /** Player control in [0,1] (0.0 if unknown). */
    double ControlIn(const FString& Id) const;
    /** Crime heat in [0,1] (0.0 if unknown). */
    double HeatIn(const FString& Id) const;
    /** Owned local businesses (0 if unknown). */
    int32 InvestmentIn(const FString& Id) const;

    /** Current desirability multiplier, clamped to the band. 1.0 for unknown. */
    double Desirability(const FString& Id) const;

    /** A base price adjusted for desirability, rounded to whole money. */
    int32 PropertyValue(int32 BasePrice, const FString& Id) const;

    /** Multiplier a caller applies to a property's passive income. */
    double IncomeMultiplier(const FString& Id) const;

    /** Set player control directly, clamped [0,1]. No-op for unknown. */
    void SetControl(const FString& Id, double Value);

    /** Add crime heat, clamped [0,1]. No-op for unknown / non-positive amount. */
    void AddHeat(const FString& Id, double Amount);

    /** Bleed heat off one district, floored at 0. No-op for unknown / non-positive. */
    void DecayHeat(const FString& Id, double Amount);

    /** Bleed heat off every district at once. */
    void DecayAllHeat(double Amount);

    /** Open a local business (raises desirability up to the cap). No-op for unknown. */
    void Invest(const FString& Id);

    /** Close a local business (floored at 0). No-op for unknown. */
    void Divest(const FString& Id);

private:
    struct FEntry
    {
        FString Id;
        double Base = 1.0;
        double Control = 0.0;
        double Heat = 0.0;
        int32 Investment = 0;
    };

    /** Insertion-ordered districts. */
    TArray<FEntry> Entries;
    /** Id -> index into Entries. */
    TMap<FString, int32> Index;

    void Register(const FDistrictDef& Entry);
    const FEntry* Find(const FString& Id) const;
    FEntry* Find(const FString& Id);
};
