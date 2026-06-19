// Copyright (c) 2026 GTC contributors

#include "StockMarket.h"

StockMarket::StockMarket(const TArray<FCompanyDef>& InCompanies)
{
    const TArray<FCompanyDef>& Source = InCompanies.Num() > 0 ? InCompanies : DefaultCompanies();
    for (const FCompanyDef& Entry : Source)
    {
        Register(Entry);
    }
}

TArray<StockMarket::FCompanyDef> StockMarket::DefaultCompanies()
{
    return {
        FCompanyDef(TEXT("augury_air"), TEXT("aviation"), 42, 0.7),
        FCompanyDef(TEXT("pelican_air"), TEXT("aviation"), 38, 0.7),
        FCompanyDef(TEXT("bittn_tech"), TEXT("tech"), 120, 0.9),
        FCompanyDef(TEXT("fruit_systems"), TEXT("tech"), 210, 0.6),
        FCompanyDef(TEXT("cluckin_co"), TEXT("food"), 24, 0.3),
        FCompanyDef(TEXT("merryweather"), TEXT("defense"), 88, 0.8),
        FCompanyDef(TEXT("lombank"), TEXT("banking"), 64, 0.5),
    };
}

int32 StockMarket::CompanyCount() const
{
    return Companies.Num();
}

bool StockMarket::HasCompany(const FString& Id) const
{
    return Index.Contains(Id);
}

TArray<FString> StockMarket::Sectors() const
{
    TArray<FString> Seen;
    for (const FCompany& Company : Companies)
    {
        Seen.AddUnique(Company.Sector);
    }
    return Seen;
}

int32 StockMarket::BasePrice(const FString& Id) const
{
    const FCompany* Company = Find(Id);
    return Company ? Company->BasePrice : -1;
}

double StockMarket::Volatility(const FString& Id) const
{
    const FCompany* Company = Find(Id);
    return Company ? Company->Volatility : 0.0;
}

FString StockMarket::SectorOf(const FString& Id) const
{
    const FCompany* Company = Find(Id);
    return Company ? Company->Sector : FString();
}

double StockMarket::Multiplier(const FString& Id) const
{
    const FCompany* Company = Find(Id);
    return Company ? Company->MultiplierValue : 1.0;
}

int32 StockMarket::Price(const FString& Id) const
{
    const int32 Base = BasePrice(Id);
    if (Base < 0)
    {
        return -1;
    }
    return FMath::Max(1, static_cast<int32>(FMath::RoundToDouble(static_cast<double>(Base) * Multiplier(Id))));
}

bool StockMarket::ApplyCompanyEvent(const FString& Id, double Magnitude)
{
    if (!Index.Contains(Id))
    {
        return false;
    }
    Move(Id, Magnitude);
    return true;
}

int32 StockMarket::ApplySectorEvent(const FString& Sector, double Magnitude)
{
    int32 Moved = 0;
    // Snapshot ids first: Move() mutates only multipliers, but iterate by value for clarity.
    for (const FCompany& Company : Companies)
    {
        if (Company.Sector == Sector)
        {
            Move(Company.Id, Magnitude);
            Moved += 1;
        }
    }
    return Moved;
}

bool StockMarket::ApplyRivalryShock(const FString& Id, double Magnitude, double Spillover)
{
    const FCompany* Target = Find(Id);
    if (!Target)
    {
        return false;
    }
    const FString Sector = Target->Sector;
    const double RivalMag = -Magnitude * FMath::Clamp(Spillover, 0.0, 1.0);
    // Collect ids first so we move each exactly once in insertion order.
    TArray<FString> Ids;
    Ids.Reserve(Companies.Num());
    for (const FCompany& Company : Companies)
    {
        Ids.Add(Company.Id);
    }
    for (const FString& OtherId : Ids)
    {
        if (OtherId == Id)
        {
            Move(Id, Magnitude);
        }
        else if (SectorOf(OtherId) == Sector)
        {
            Move(OtherId, RivalMag);
        }
    }
    return true;
}

StockMarket::FBuyResult StockMarket::Buy(const FString& Id, int32 Qty, int32 Balance)
{
    if (!Index.Contains(Id))
    {
        return FailBuy(Balance, FString::Printf(TEXT("unknown company: %s"), *Id));
    }
    if (Qty <= 0)
    {
        return FailBuy(Balance, FString::Printf(TEXT("qty must be positive: %d"), Qty));
    }
    const int32 Unit = Price(Id);
    const int32 Cost = Unit * Qty;
    if (Balance < Cost)
    {
        return FailBuy(Balance, FString::Printf(TEXT("insufficient funds: need %d, have %d"), Cost, Balance));
    }
    const int32 PrevQty = SharesHeld(Id);
    const double PrevBasis = AvgCost(Id) * static_cast<double>(PrevQty);
    const int32 NewQty = PrevQty + Qty;
    const double NewAvg = (PrevBasis + static_cast<double>(Cost)) / static_cast<double>(NewQty);
    if (FHolding* Holding = FindHolding(Id))
    {
        Holding->Qty = NewQty;
        Holding->AvgCost = NewAvg;
    }
    else
    {
        FHolding NewHolding;
        NewHolding.Id = Id;
        NewHolding.Qty = NewQty;
        NewHolding.AvgCost = NewAvg;
        const int32 NewIndex = Holdings.Add(NewHolding);
        HoldingIndex.Add(Id, NewIndex);
    }
    FBuyResult Result;
    Result.bSuccess = true;
    Result.Cost = Cost;
    Result.NewBalance = Balance - Cost;
    Result.Reason = FString();
    return Result;
}

StockMarket::FSellResult StockMarket::Sell(const FString& Id, int32 Qty)
{
    if (!Index.Contains(Id))
    {
        return FailSell(FString::Printf(TEXT("unknown company: %s"), *Id));
    }
    if (Qty <= 0)
    {
        return FailSell(FString::Printf(TEXT("qty must be positive: %d"), Qty));
    }
    const int32 Held = SharesHeld(Id);
    if (Qty > Held)
    {
        return FailSell(FString::Printf(TEXT("not enough shares: have %d, tried to sell %d"), Held, Qty));
    }
    const int32 UnitPrice = Price(Id);
    const int32 Proceeds = UnitPrice * Qty;
    const int32 RealizedDelta = static_cast<int32>(FMath::RoundToDouble((static_cast<double>(UnitPrice) - AvgCost(Id)) * static_cast<double>(Qty)));
    Realized += RealizedDelta;
    const int32 Remaining = Held - Qty;
    if (Remaining <= 0)
    {
        EraseHolding(Id);
    }
    else if (FHolding* Holding = FindHolding(Id))
    {
        Holding->Qty = Remaining;
    }
    FSellResult Result;
    Result.bSuccess = true;
    Result.Proceeds = Proceeds;
    Result.Realized = RealizedDelta;
    Result.Reason = FString();
    return Result;
}

int32 StockMarket::SharesHeld(const FString& Id) const
{
    const FHolding* Holding = FindHolding(Id);
    return Holding ? Holding->Qty : 0;
}

double StockMarket::AvgCost(const FString& Id) const
{
    const FHolding* Holding = FindHolding(Id);
    return Holding ? Holding->AvgCost : 0.0;
}

int32 StockMarket::PortfolioValue() const
{
    int32 Total = 0;
    for (const FHolding& Holding : Holdings)
    {
        Total += Price(Holding.Id) * Holding.Qty;
    }
    return Total;
}

int32 StockMarket::TotalInvested() const
{
    double Total = 0.0;
    for (const FHolding& Holding : Holdings)
    {
        Total += Holding.AvgCost * static_cast<double>(Holding.Qty);
    }
    return static_cast<int32>(FMath::RoundToDouble(Total));
}

int32 StockMarket::UnrealizedGain() const
{
    return PortfolioValue() - TotalInvested();
}

int32 StockMarket::RealizedGain() const
{
    return Realized;
}

void StockMarket::Fluctuate(FRandomStream& Rng, double Intensity)
{
    if (Intensity <= 0.0)
    {
        return;
    }
    for (FCompany& Company : Companies)
    {
        const double Swing = FluctuateSwing * Intensity * Company.Volatility;
        if (Swing <= 0.0)
        {
            continue;
        }
        const double Delta = Rng.FRandRange(-Swing, Swing);
        Company.MultiplierValue = FMath::Clamp(Company.MultiplierValue + Delta, MultiplierMin, MultiplierMax);
    }
}

void StockMarket::FluctuateNoRng(double /*Intensity*/)
{
}

FRandomStream StockMarket::MakeRng(int32 Seed)
{
    return FRandomStream(Seed);
}

StockMarket::FBuyResult StockMarket::FailBuy(int32 Balance, const FString& Reason)
{
    FBuyResult Result;
    Result.bSuccess = false;
    Result.Cost = 0;
    Result.NewBalance = Balance;
    Result.Reason = Reason;
    return Result;
}

StockMarket::FSellResult StockMarket::FailSell(const FString& Reason)
{
    FSellResult Result;
    Result.bSuccess = false;
    Result.Proceeds = 0;
    Result.Realized = 0;
    Result.Reason = Reason;
    return Result;
}

void StockMarket::Register(const FCompanyDef& Entry)
{
    if (Entry.Id.IsEmpty() || Index.Contains(Entry.Id) || Entry.BasePrice <= 0)
    {
        return;
    }
    FCompany Company;
    Company.Id = Entry.Id;
    Company.Sector = Entry.Sector.IsEmpty() ? FString(TEXT("misc")) : Entry.Sector;
    Company.BasePrice = Entry.BasePrice;
    Company.Volatility = FMath::Clamp(Entry.Volatility, 0.0, 1.0);
    Company.MultiplierValue = 1.0;
    const int32 NewIndex = Companies.Add(Company);
    Index.Add(Company.Id, NewIndex);
}

const StockMarket::FCompany* StockMarket::Find(const FString& Id) const
{
    const int32* FoundIndex = Index.Find(Id);
    return FoundIndex ? &Companies[*FoundIndex] : nullptr;
}

StockMarket::FCompany* StockMarket::Find(const FString& Id)
{
    const int32* FoundIndex = Index.Find(Id);
    return FoundIndex ? &Companies[*FoundIndex] : nullptr;
}

const StockMarket::FHolding* StockMarket::FindHolding(const FString& Id) const
{
    const int32* FoundIndex = HoldingIndex.Find(Id);
    return FoundIndex ? &Holdings[*FoundIndex] : nullptr;
}

StockMarket::FHolding* StockMarket::FindHolding(const FString& Id)
{
    const int32* FoundIndex = HoldingIndex.Find(Id);
    return FoundIndex ? &Holdings[*FoundIndex] : nullptr;
}

void StockMarket::EraseHolding(const FString& Id)
{
    const int32* FoundIndex = HoldingIndex.Find(Id);
    if (!FoundIndex)
    {
        return;
    }
    const int32 Removed = *FoundIndex;
    Holdings.RemoveAt(Removed);
    HoldingIndex.Remove(Id);
    // Reindex: indices after the removed slot shifted down by one.
    for (TPair<FString, int32>& Pair : HoldingIndex)
    {
        if (Pair.Value > Removed)
        {
            Pair.Value -= 1;
        }
    }
}

void StockMarket::Move(const FString& Id, double Magnitude)
{
    FCompany* Company = Find(Id);
    if (!Company)
    {
        return;
    }
    const double Effective = Magnitude * Company->Volatility;
    // Mirror the reference is_zero_approx (CMP_EPSILON = 1e-5): a zero-volatility / zero net
    // move leaves the price untouched.
    if (FMath::Abs(Effective) < 1e-5)
    {
        return;
    }
    Company->MultiplierValue = FMath::Clamp(Company->MultiplierValue * (1.0 + Effective), MultiplierMin, MultiplierMax);
}
