// Copyright (c) 2026 GTC contributors

#include "HitContract.h"

namespace
{
    FHitContractDef MakeContract(
        const FString& Id, const FString& Target, const FString& CompanyId,
        double Magnitude, double Spillover, int32 Reward, int32 Difficulty)
    {
        FHitContractDef Def;
        Def.Id = Id;
        Def.Target = Target;
        Def.CompanyId = CompanyId;
        Def.Magnitude = Magnitude;
        Def.Spillover = Spillover;
        Def.Reward = Reward;
        Def.Difficulty = Difficulty;
        return Def;
    }
}

HitContract::HitContract(const TArray<FHitContractDef>& InContracts)
{
    const TArray<FHitContractDef>& Source = InContracts.Num() > 0 ? InContracts : DefaultContracts();
    for (const FHitContractDef& Entry : Source)
    {
        Register(Entry);
    }
}

TArray<FHitContractDef> HitContract::DefaultContracts()
{
    return {
        MakeContract(TEXT("tech_takedown"), TEXT("Avery Kohl"), TEXT("bittn_tech"), -0.5, 0.8, 25000, 4),
        MakeContract(TEXT("airline_war"), TEXT("Don Percival"), TEXT("pelican_air"), -0.4, 1.0, 18000, 3),
        MakeContract(TEXT("bank_reckoning"), TEXT("Cliff Lombard"), TEXT("lombank"), -0.3, 0.5, 30000, 5),
    };
}

int32 HitContract::ContractCount() const
{
    return Contracts.Num();
}

bool HitContract::HasContract(const FString& Id) const
{
    return Index.Contains(Id);
}

TArray<FString> HitContract::Ids() const
{
    TArray<FString> Out;
    Out.Reserve(Contracts.Num());
    for (const FHitContractDef& Contract : Contracts)
    {
        Out.Add(Contract.Id);
    }
    return Out;
}

int32 HitContract::RewardOf(const FString& Id) const
{
    const FHitContractDef* Contract = Find(Id);
    return Contract ? Contract->Reward : -1;
}

FString HitContract::TargetOf(const FString& Id) const
{
    const FHitContractDef* Contract = Find(Id);
    return Contract ? Contract->Target : FString();
}

FMarketEffect HitContract::MarketEffectOf(const FString& Id) const
{
    FMarketEffect Effect;
    const FHitContractDef* Contract = Find(Id);
    if (!Contract)
    {
        return Effect;
    }
    Effect.bValid = true;
    Effect.CompanyId = Contract->CompanyId;
    Effect.Magnitude = Contract->Magnitude;
    Effect.Spillover = Contract->Spillover;
    return Effect;
}

TArray<FString> HitContract::Available() const
{
    TArray<FString> Out;
    for (const FHitContractDef& Contract : Contracts)
    {
        if (!Completed.Contains(Contract.Id) && Contract.Id != ActiveId)
        {
            Out.Add(Contract.Id);
        }
    }
    return Out;
}

bool HitContract::Accept(const FString& Id)
{
    if (!Index.Contains(Id) || Completed.Contains(Id) || !ActiveId.IsEmpty())
    {
        return false;
    }
    ActiveId = Id;
    return true;
}

FString HitContract::Active() const
{
    return ActiveId;
}

bool HitContract::HasActive() const
{
    return !ActiveId.IsEmpty();
}

FString HitContract::Abandon()
{
    const FString Prior = ActiveId;
    ActiveId = FString();
    return Prior;
}

FHitCompleteResult HitContract::Complete()
{
    FHitCompleteResult Result;
    if (ActiveId.IsEmpty())
    {
        Result.bSuccess = false;
        Result.Reward = 0;
        Result.Reason = TEXT("no active contract");
        return Result;
    }
    const FString Id = ActiveId;
    const FHitContractDef* Contract = Find(Id);
    const int32 Reward = Contract ? Contract->Reward : 0;
    Result.MarketEffect = MarketEffectOf(Id);
    Completed.Add(Id);
    Earned += Reward;
    ActiveId = FString();
    Result.bSuccess = true;
    Result.Reward = Reward;
    Result.Reason = FString();
    return Result;
}

bool HitContract::IsCompleted(const FString& Id) const
{
    return Completed.Contains(Id);
}

int32 HitContract::CompletedCount() const
{
    return Completed.Num();
}

int32 HitContract::TotalEarned() const
{
    return Earned;
}

void HitContract::Register(const FHitContractDef& Entry)
{
    // Drop malformed rows: empty id, duplicate, or non-positive reward.
    if (Entry.Id.IsEmpty() || Index.Contains(Entry.Id) || Entry.Reward <= 0)
    {
        return;
    }
    FHitContractDef Stored = Entry;
    if (Stored.Target.IsEmpty())
    {
        Stored.Target = TEXT("Unknown");
    }
    Stored.Spillover = FMath::Clamp(Entry.Spillover, 0.0, 1.0);
    Stored.Difficulty = FMath::Clamp(Entry.Difficulty, 1, 5);
    const int32 NewIndex = Contracts.Add(Stored);
    Index.Add(Stored.Id, NewIndex);
}

const FHitContractDef* HitContract::Find(const FString& Id) const
{
    const int32* FoundIndex = Index.Find(Id);
    return FoundIndex ? &Contracts[*FoundIndex] : nullptr;
}
