// Copyright (c) 2026 GTC contributors

#include "BusinessVenture.h"

namespace
{
    FVentureDef MakeVenture(
        const FString& Id, const FString& Name, double ProductPerDay, double ProductPerSupply,
        double MaxProduct, int32 SaleValue, int32 MaxStaff, int32 MaxTier)
    {
        FVentureDef Def;
        Def.Id = Id;
        Def.Name = Name;
        Def.ProductPerDay = ProductPerDay;
        Def.ProductPerSupply = ProductPerSupply;
        Def.MaxProduct = MaxProduct;
        Def.SaleValue = SaleValue;
        Def.MaxStaff = MaxStaff;
        Def.MaxTier = MaxTier;
        return Def;
    }
}

BusinessVenture::BusinessVenture(const TArray<FVentureDef>& Ventures)
{
    const TArray<FVentureDef>& Source = Ventures.Num() > 0 ? Ventures : DefaultVentures();
    for (const FVentureDef& Entry : Source)
    {
        Register(Entry);
    }
}

TArray<FVentureDef> BusinessVenture::DefaultVentures()
{
    return {
        MakeVenture(TEXT("coke_lab"), TEXT("Cocaine Lockup"), 10.0, 2.0, 200.0, 2000, 6, 3),
        MakeVenture(TEXT("cash_factory"), TEXT("Counterfeit Cash Factory"), 15.0, 1.5, 300.0, 1200, 8, 3),
        MakeVenture(TEXT("nightclub"), TEXT("Vice Nightclub"), 8.0, 1.0, 150.0, 1500, 10, 2),
        MakeVenture(TEXT("weed_farm"), TEXT("Everglades Weed Farm"), 12.0, 2.5, 250.0, 900, 5, 3),
    };
}

int32 BusinessVenture::VentureCount() const
{
    return Catalogue.Num();
}

bool BusinessVenture::HasVenture(const FString& Id) const
{
    return Index.Contains(Id);
}

TArray<FString> BusinessVenture::Ids() const
{
    TArray<FString> Out;
    Out.Reserve(Catalogue.Num());
    for (const FCatalogueRow& Row : Catalogue)
    {
        Out.Add(Row.Id);
    }
    return Out;
}

bool BusinessVenture::Owns(const FString& Id) const
{
    return OwnedIndex.Contains(Id);
}

TArray<FString> BusinessVenture::OwnedIds() const
{
    TArray<FString> Out;
    Out.Reserve(Owned.Num());
    for (const FOwnedOp& Op : Owned)
    {
        Out.Add(Op.Id);
    }
    Out.Sort();
    return Out;
}

double BusinessVenture::SupplyIn(const FString& Id) const
{
    const FOwnedOp* Op = FindOwned(Id);
    return Op ? Op->Supply : 0.0;
}

double BusinessVenture::ProductIn(const FString& Id) const
{
    const FOwnedOp* Op = FindOwned(Id);
    return Op ? Op->Product : 0.0;
}

int32 BusinessVenture::StaffIn(const FString& Id) const
{
    const FOwnedOp* Op = FindOwned(Id);
    return Op ? Op->Staff : 0;
}

int32 BusinessVenture::TierIn(const FString& Id) const
{
    const FOwnedOp* Op = FindOwned(Id);
    return Op ? Op->Tier : 0;
}

FVentureMoneyResult BusinessVenture::Acquire(const FString& Id, int32 Cost, int32 Balance)
{
    if (!Index.Contains(Id))
    {
        return Fail(Balance, FString::Printf(TEXT("unknown venture: %s"), *Id));
    }
    if (OwnedIndex.Contains(Id))
    {
        return Fail(Balance, FString::Printf(TEXT("already owned: %s"), *Id));
    }
    if (Cost < 0)
    {
        return Fail(Balance, TEXT("negative cost"));
    }
    if (Balance < Cost)
    {
        return Fail(Balance, FString::Printf(TEXT("insufficient funds: need %d, have %d"), Cost, Balance));
    }
    FOwnedOp Op;
    Op.Id = Id;
    AddOwned(Op);
    FVentureMoneyResult Result;
    Result.bSuccess = true;
    Result.Cost = Cost;
    Result.NewBalance = Balance - Cost;
    Result.Reason = FString();
    return Result;
}

FVentureMoneyResult BusinessVenture::BuySupplies(const FString& Id, int32 Units, int32 UnitCost, int32 Balance)
{
    FOwnedOp* Op = FindOwned(Id);
    if (!Op)
    {
        return Fail(Balance, FString::Printf(TEXT("not owned: %s"), *Id));
    }
    if (Units <= 0)
    {
        return Fail(Balance, TEXT("non-positive units"));
    }
    if (UnitCost <= 0)
    {
        return Fail(Balance, TEXT("non-positive unit_cost"));
    }
    const double Ceiling = SupplyCeiling(Id);
    const double Current = Op->Supply;
    const double Added = FMath::Clamp(Ceiling - Current, 0.0, static_cast<double>(Units));
    if (Added <= 0.0)
    {
        return Fail(Balance, FString::Printf(TEXT("already fully stocked: %s"), *Id));
    }
    const int32 Cost = static_cast<int32>(FMath::RoundToDouble(Added * static_cast<double>(UnitCost)));
    if (Balance < Cost)
    {
        return Fail(Balance, FString::Printf(TEXT("insufficient funds: need %d, have %d"), Cost, Balance));
    }
    Op->Supply = Current + Added;
    FVentureMoneyResult Result;
    Result.bSuccess = true;
    Result.Cost = Cost;
    Result.NewBalance = Balance - Cost;
    Result.Reason = FString();
    return Result;
}

FVentureMoneyResult BusinessVenture::Upgrade(const FString& Id, int32 Cost, int32 Balance)
{
    FOwnedOp* Op = FindOwned(Id);
    if (!Op)
    {
        return Fail(Balance, FString::Printf(TEXT("not owned: %s"), *Id));
    }
    const FCatalogueRow* Row = Find(Id);
    const int32 MaxTier = Row ? Row->MaxTier : 0;
    if (Op->Tier >= MaxTier)
    {
        return Fail(Balance, TEXT("already at max tier"));
    }
    if (Cost < 0)
    {
        return Fail(Balance, TEXT("negative cost"));
    }
    if (Balance < Cost)
    {
        return Fail(Balance, FString::Printf(TEXT("insufficient funds: need %d, have %d"), Cost, Balance));
    }
    Op->Tier += 1;
    FVentureMoneyResult Result;
    Result.bSuccess = true;
    Result.Cost = Cost;
    Result.NewBalance = Balance - Cost;
    Result.Reason = FString();
    return Result;
}

bool BusinessVenture::Hire(const FString& Id)
{
    FOwnedOp* Op = FindOwned(Id);
    if (!Op)
    {
        return false;
    }
    const FCatalogueRow* Row = Find(Id);
    const int32 MaxStaff = Row ? Row->MaxStaff : 0;
    if (Op->Staff >= MaxStaff)
    {
        return false;
    }
    Op->Staff += 1;
    return true;
}

bool BusinessVenture::Fire(const FString& Id)
{
    FOwnedOp* Op = FindOwned(Id);
    if (!Op)
    {
        return false;
    }
    if (Op->Staff <= 0)
    {
        return false;
    }
    Op->Staff -= 1;
    return true;
}

double BusinessVenture::ProductionRate(const FString& Id) const
{
    const FOwnedOp* Op = FindOwned(Id);
    if (!Op)
    {
        return 0.0;
    }
    if (Op->Supply <= 0.0)
    {
        return 0.0;
    }
    const FCatalogueRow* Row = Find(Id);
    if (!Row)
    {
        return 0.0;
    }
    return Row->ProductPerDay * StaffFactor(Op->Staff) * TierFactor(Op->Tier);
}

void BusinessVenture::Accrue(double DeltaDays)
{
    if (DeltaDays <= 0.0)
    {
        return;
    }
    for (FOwnedOp& Op : Owned)
    {
        const double Rate = ProductionRate(Op.Id);
        if (Rate <= 0.0)
        {
            continue;
        }
        const FCatalogueRow* Row = Find(Op.Id);
        if (!Row)
        {
            continue;
        }
        const double PerSupply = Row->ProductPerSupply;
        const double MaxProduct = Row->MaxProduct;
        const double Wanted = Rate * DeltaDays;
        const double Headroom = FMath::Max(MaxProduct - Op.Product, 0.0);
        const double FromSupply = PerSupply > 0.0 ? Op.Supply / PerSupply : Wanted;
        const double Made = FMath::Min(Wanted, FMath::Min(Headroom, FromSupply));
        Op.Product = Op.Product + Made;
        Op.Supply = FMath::Max(Op.Supply - Made * PerSupply, 0.0);
    }
}

int32 BusinessVenture::SalePrice(const FString& Id, double Demand, double Heat) const
{
    const FCatalogueRow* Row = Find(Id);
    if (!Row)
    {
        return 0;
    }
    const double D = FMath::Clamp(Demand, DemandMin, DemandMax);
    const double H = FMath::Clamp(Heat, 0.0, 1.0);
    return FMath::Max(1, static_cast<int32>(FMath::RoundToDouble(static_cast<double>(Row->SaleValue) * D * (1.0 - HeatDiscount * H))));
}

FVentureSellResult BusinessVenture::Sell(const FString& Id, int32 Units, double Demand, double Heat)
{
    FOwnedOp* Op = FindOwned(Id);
    if (!Op)
    {
        return SellFail(FString::Printf(TEXT("not owned: %s"), *Id));
    }
    if (Units <= 0)
    {
        return SellFail(TEXT("non-positive units"));
    }
    const int32 Available = static_cast<int32>(FMath::FloorToDouble(Op->Product));
    if (Available <= 0)
    {
        return SellFail(TEXT("empty stockpile"));
    }
    const int32 Sold = FMath::Min(Units, Available);
    const int32 Proceeds = Sold * SalePrice(Id, Demand, Heat);
    Op->Product = Op->Product - static_cast<double>(Sold);
    Gross += Proceeds;
    FVentureSellResult Result;
    Result.bSuccess = true;
    Result.Proceeds = Proceeds;
    Result.Sold = Sold;
    Result.Reason = FString();
    return Result;
}

int32 BusinessVenture::TotalProduct() const
{
    double Sum = 0.0;
    for (const FOwnedOp& Op : Owned)
    {
        Sum += Op.Product;
    }
    return static_cast<int32>(FMath::FloorToDouble(Sum));
}

int32 BusinessVenture::GrossEarned() const
{
    return Gross;
}

FVentureSaveData BusinessVenture::Serialize() const
{
    FVentureSaveData Data;
    Data.bValid = true;
    Data.Gross = Gross;
    for (const FOwnedOp& Op : Owned)
    {
        FVentureState State;
        State.Id = Op.Id;
        State.Supply = Op.Supply;
        State.Product = Op.Product;
        State.Staff = Op.Staff;
        State.Tier = Op.Tier;
        Data.Owned.Add(State);
    }
    return Data;
}

void BusinessVenture::Restore(const FVentureSaveData& Data)
{
    Owned.Reset();
    OwnedIndex.Reset();
    Gross = static_cast<int32>(FMath::Max(static_cast<double>(Data.Gross), 0.0));
    if (!Data.bValid)
    {
        return;
    }
    for (const FVentureState& State : Data.Owned)
    {
        const FCatalogueRow* Row = Find(State.Id);
        if (!Row)
        {
            continue;
        }
        // Clamp ALL operational fields so a crafted save can't load past the caps.
        FOwnedOp Op;
        Op.Id = State.Id;
        Op.Supply = FMath::Clamp(State.Supply, 0.0, SupplyCeiling(State.Id));
        Op.Product = FMath::Clamp(State.Product, 0.0, Row->MaxProduct);
        Op.Staff = FMath::Clamp(State.Staff, 0, Row->MaxStaff);
        Op.Tier = FMath::Clamp(State.Tier, 0, Row->MaxTier);
        AddOwned(Op);
    }
}

double BusinessVenture::StaffFactor(int32 Staff)
{
    return 1.0 + static_cast<double>(FMath::Max(Staff, 0)) * StaffGain;
}

double BusinessVenture::TierFactor(int32 Tier)
{
    return 1.0 + static_cast<double>(FMath::Max(Tier, 0)) * TierGain;
}

FVentureMoneyResult BusinessVenture::Fail(int32 Balance, const FString& Reason)
{
    FVentureMoneyResult Result;
    Result.bSuccess = false;
    Result.Cost = 0;
    Result.NewBalance = Balance;
    Result.Reason = Reason;
    return Result;
}

FVentureSellResult BusinessVenture::SellFail(const FString& Reason)
{
    FVentureSellResult Result;
    Result.bSuccess = false;
    Result.Proceeds = 0;
    Result.Sold = 0;
    Result.Reason = Reason;
    return Result;
}

void BusinessVenture::Register(const FVentureDef& Entry)
{
    if (Entry.Id.IsEmpty() || Index.Contains(Entry.Id))
    {
        return;
    }
    if (Entry.ProductPerDay <= 0.0 || Entry.SaleValue <= 0)
    {
        return;
    }
    FCatalogueRow Row;
    Row.Id = Entry.Id;
    Row.Name = Entry.Name.IsEmpty() ? Entry.Id : Entry.Name;
    Row.ProductPerDay = Entry.ProductPerDay;
    Row.ProductPerSupply = Entry.ProductPerSupply > 0.0 ? Entry.ProductPerSupply : DefaultProductPerSupply;
    Row.MaxProduct = FMath::Clamp(Entry.MaxProduct, 1.0, MaxReasonableProduct);
    Row.SaleValue = FMath::Min(Entry.SaleValue, MaxReasonableSaleValue);
    Row.MaxStaff = FMath::Max(Entry.MaxStaff, 0);
    Row.MaxTier = FMath::Max(Entry.MaxTier, 0);
    const int32 NewIndex = Catalogue.Add(Row);
    Index.Add(Row.Id, NewIndex);
}

const BusinessVenture::FCatalogueRow* BusinessVenture::Find(const FString& Id) const
{
    const int32* FoundIndex = Index.Find(Id);
    return FoundIndex ? &Catalogue[*FoundIndex] : nullptr;
}

const BusinessVenture::FOwnedOp* BusinessVenture::FindOwned(const FString& Id) const
{
    const int32* FoundIndex = OwnedIndex.Find(Id);
    return FoundIndex ? &Owned[*FoundIndex] : nullptr;
}

BusinessVenture::FOwnedOp* BusinessVenture::FindOwned(const FString& Id)
{
    const int32* FoundIndex = OwnedIndex.Find(Id);
    return FoundIndex ? &Owned[*FoundIndex] : nullptr;
}

void BusinessVenture::AddOwned(const FOwnedOp& Op)
{
    const int32 NewIndex = Owned.Add(Op);
    OwnedIndex.Add(Op.Id, NewIndex);
}

double BusinessVenture::SupplyCeiling(const FString& Id) const
{
    const FCatalogueRow* Row = Find(Id);
    if (!Row)
    {
        return 0.0;
    }
    return Row->MaxProduct * Row->ProductPerSupply;
}
