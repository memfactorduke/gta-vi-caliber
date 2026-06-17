// Copyright (c) 2026 GTC contributors

#include "ContrabandMarket.h"

ContrabandMarket::ContrabandMarket(const TArray<FGoodDef>& InGoods)
{
    const TArray<FGoodDef>& Source = InGoods.Num() > 0 ? InGoods : DefaultGoods();
    for (const FGoodDef& Entry : Source)
    {
        Register(Entry);
    }
}

TArray<ContrabandMarket::FGoodDef> ContrabandMarket::DefaultGoods()
{
    return {
        FGoodDef(TEXT("cash"), 100),
        FGoodDef(TEXT("jewelry"), 2500),
        FGoodDef(TEXT("electronics"), 800),
        FGoodDef(TEXT("product"), 1500),
    };
}

int32 ContrabandMarket::GoodsCount() const
{
    return Goods.Num();
}

bool ContrabandMarket::HasGood(const FString& Good) const
{
    return Index.Contains(Good);
}

int32 ContrabandMarket::BasePrice(const FString& Good) const
{
    const FGoodDef* Def = Find(Good);
    return Def ? Def->BasePrice : -1;
}

double ContrabandMarket::MultiplierFor(const FString& DistrictId)
{
    if (DistrictId.IsEmpty())
    {
        return 1.0;
    }
    if (const double* Found = Multipliers.Find(DistrictId))
    {
        return *Found;
    }
    const double Seeded = SeedMultiplier(DistrictId);
    Multipliers.Add(DistrictId, Seeded);
    MultiplierOrder.Add(DistrictId);
    return Seeded;
}

int32 ContrabandMarket::PriceIn(const FString& Good, const FString& DistrictId)
{
    const int32 Base = BasePrice(Good);
    if (Base < 0)
    {
        return -1;
    }
    return static_cast<int32>(FMath::RoundToDouble(static_cast<double>(Base) * MultiplierFor(DistrictId)));
}

ContrabandMarket::FBuyResult ContrabandMarket::Buy(const FString& Good, int32 Qty, const FString& DistrictId, int32 Balance)
{
    if (!Index.Contains(Good))
    {
        return Fail(Balance, FString::Printf(TEXT("unknown good: %s"), *Good));
    }
    if (Qty <= 0)
    {
        return Fail(Balance, FString::Printf(TEXT("qty must be positive: %d"), Qty));
    }
    const int32 Cost = PriceIn(Good, DistrictId) * Qty;
    if (Balance < Cost)
    {
        return Fail(Balance, FString::Printf(TEXT("insufficient funds: need %d, have %d"), Cost, Balance));
    }
    FBuyResult Result;
    Result.bSuccess = true;
    Result.Cost = Cost;
    Result.NewBalance = Balance - Cost;
    Result.Reason = FString();
    return Result;
}

int32 ContrabandMarket::Sell(const FString& Good, int32 Qty, const FString& DistrictId)
{
    if (!Index.Contains(Good) || Qty <= 0)
    {
        return 0;
    }
    return PriceIn(Good, DistrictId) * Qty;
}

FString ContrabandMarket::BestMarket(const FString& Good, const TArray<FString>& DistrictIds)
{
    if (!Index.Contains(Good) || DistrictIds.Num() == 0)
    {
        return FString();
    }
    FString BestId;
    int32 BestPrice = -1;
    for (const FString& DistrictId : DistrictIds)
    {
        const int32 P = PriceIn(Good, DistrictId);
        if (P > BestPrice)
        {
            BestPrice = P;
            BestId = DistrictId;
        }
    }
    return BestId;
}

int32 ContrabandMarket::Profit(const FString& Good, const FString& BuyDistrict, const FString& SellDistrict, int32 Qty)
{
    if (!Index.Contains(Good) || Qty <= 0)
    {
        return 0;
    }
    const int32 Spend = PriceIn(Good, BuyDistrict) * Qty;
    const int32 Revenue = PriceIn(Good, SellDistrict) * Qty;
    return Revenue - Spend;
}

void ContrabandMarket::Carry(const FString& Good, int32 Qty)
{
    if (!Index.Contains(Good) || Qty <= 0)
    {
        return;
    }
    CarriedMap.FindOrAdd(Good) += Qty;
}

int32 ContrabandMarket::Carried(const FString& Good) const
{
    const int32* Found = CarriedMap.Find(Good);
    return Found ? *Found : 0;
}

void ContrabandMarket::Drop(const FString& Good, int32 Qty)
{
    if (!Index.Contains(Good) || Qty <= 0)
    {
        return;
    }
    int32& Held = CarriedMap.FindOrAdd(Good);
    Held = FMath::Max(Held - Qty, 0);
}

int32 ContrabandMarket::TotalCarried() const
{
    int32 Sum = 0;
    for (const TPair<FString, int32>& Pair : CarriedMap)
    {
        Sum += Pair.Value;
    }
    return Sum;
}

double ContrabandMarket::BustRisk(int32 Total, double BaseRisk)
{
    if (Total <= 0)
    {
        return FMath::Clamp(BaseRisk, 0.0, 1.0);
    }
    const double LoadPenalty = static_cast<double>(Total) * 0.05;
    return FMath::Clamp(BaseRisk + LoadPenalty, 0.0, 1.0);
}

void ContrabandMarket::Fluctuate(FRandomStream& Rng, double Volatility)
{
    if (Volatility <= 0.0)
    {
        return;
    }
    const double Swing = FluctuateSwing * Volatility;
    for (const FString& DistrictId : MultiplierOrder)
    {
        const double Delta = Rng.FRandRange(-Swing, Swing);
        double& Mult = Multipliers[DistrictId];
        Mult = FMath::Clamp(Mult + Delta, MultiplierMin, MultiplierMax);
    }
}

void ContrabandMarket::FluctuateNoRng(double /*Volatility*/)
{
}

FRandomStream ContrabandMarket::MakeRng(int32 Seed)
{
    return FRandomStream(Seed);
}

ContrabandMarket::FBuyResult ContrabandMarket::Fail(int32 Balance, const FString& Reason)
{
    FBuyResult Result;
    Result.bSuccess = false;
    Result.Cost = 0;
    Result.NewBalance = Balance;
    Result.Reason = Reason;
    return Result;
}

void ContrabandMarket::Register(const FGoodDef& Entry)
{
    if (Entry.Id.IsEmpty() || Index.Contains(Entry.Id) || Entry.BasePrice <= 0)
    {
        return;
    }
    const int32 NewIndex = Goods.Add(Entry);
    Index.Add(Entry.Id, NewIndex);
}

const ContrabandMarket::FGoodDef* ContrabandMarket::Find(const FString& Good) const
{
    const int32* FoundIndex = Index.Find(Good);
    return FoundIndex ? &Goods[*FoundIndex] : nullptr;
}

double ContrabandMarket::SeedMultiplier(const FString& DistrictId)
{
    const uint32 Hash = StableHash(DistrictId);
    const double Frac = static_cast<double>(Hash % 1000) / 1000.0;
    return MultiplierMin + Frac * (MultiplierMax - MultiplierMin);
}

uint32 ContrabandMarket::StableHash(const FString& Value)
{
    // FNV-1a over UTF-8 bytes: a stable, well-distributed deterministic hash. We do
    // NOT need Godot's hash() — the oracle pins no exact hash values, only band
    // membership and that distinct districts differ.
    uint32 Hash = 2166136261u;
    const FTCHARToUTF8 Utf8(*Value);
    const ANSICHAR* Bytes = reinterpret_cast<const ANSICHAR*>(Utf8.Get());
    const int32 Length = Utf8.Length();
    for (int32 I = 0; I < Length; ++I)
    {
        Hash ^= static_cast<uint8>(Bytes[I]);
        Hash *= 16777619u;
    }
    return Hash;
}
