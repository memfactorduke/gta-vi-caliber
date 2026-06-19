// Copyright (c) 2026 GTC contributors

#include "PlayerSkills.h"

const TArray<FPlayerSkills::FTier>& FPlayerSkills::Tiers()
{
    static const TArray<FTier> Table = {
        { 0.0,  TEXT("novice") },
        { 20.0, TEXT("competent") },
        { 40.0, TEXT("skilled") },
        { 60.0, TEXT("expert") },
        { 85.0, TEXT("master") },
    };
    return Table;
}

TArray<FSkillDef> FPlayerSkills::DefaultSkills()
{
    return {
        FSkillDef(TEXT("driving"), 1.0),
        FSkillDef(TEXT("shooting"), 0.8),
        FSkillDef(TEXT("stamina"), 1.2),
        FSkillDef(TEXT("strength"), 0.9),
        FSkillDef(TEXT("stealth"), 0.7),
        FSkillDef(TEXT("lung_capacity"), 0.6),
        FSkillDef(TEXT("flying"), 0.5),
    };
}

FPlayerSkills::FPlayerSkills(const TArray<FSkillDef>& Skills)
{
    const TArray<FSkillDef>& Source = Skills.Num() > 0 ? Skills : DefaultSkills();
    for (const FSkillDef& Entry : Source)
    {
        Register(Entry);
    }
}

void FPlayerSkills::Register(const FSkillDef& Entry)
{
    if (Entry.Id.IsEmpty() || _Index.Contains(Entry.Id))
    {
        return;
    }
    if (Entry.Rate <= 0.0)
    {
        return;
    }
    FSkillDef Stored;
    Stored.Id = Entry.Id;
    Stored.Rate = Entry.Rate;
    Stored.Value = FMath::Clamp(Entry.Value, 0.0, MaxSkill);
    const int32 NewIndex = _Order.Add(Stored);
    _Index.Add(Stored.Id, NewIndex);
}

TArray<FString> FPlayerSkills::Skills() const
{
    TArray<FString> Keys;
    Keys.Reserve(_Order.Num());
    for (const FSkillDef& Entry : _Order)
    {
        Keys.Add(Entry.Id);
    }
    return Keys;
}

double FPlayerSkills::Level(const FString& Id) const
{
    const int32* Idx = _Index.Find(Id);
    if (Idx == nullptr)
    {
        return 0.0;
    }
    return _Order[*Idx].Value;
}

double FPlayerSkills::Train(const FString& Id, double Effort)
{
    const int32* Idx = _Index.Find(Id);
    if (Idx == nullptr || Effort <= 0.0)
    {
        return 0.0;
    }
    FSkillDef& Skill = _Order[*Idx];
    const double Value = Skill.Value;
    const double Headroom = 1.0 - Value / MaxSkill;
    if (Headroom <= 0.0)
    {
        return 0.0;
    }
    const double Gain = Effort * Skill.Rate * Headroom;
    const double NewValue = FMath::Clamp(Value + Gain, 0.0, MaxSkill);
    Skill.Value = NewValue;
    return NewValue - Value;
}

FString FPlayerSkills::Tier(const FString& Id) const
{
    const int32* Idx = _Index.Find(Id);
    if (Idx == nullptr)
    {
        return FString();
    }
    const double Value = _Order[*Idx].Value;
    FString Label;
    for (const FTier& Band : Tiers())
    {
        if (Value >= Band.Floor)
        {
            Label = Band.Label;
        }
    }
    return Label;
}

void FPlayerSkills::SetLevel(const FString& Id, double Value)
{
    const int32* Idx = _Index.Find(Id);
    if (Idx == nullptr)
    {
        return;
    }
    _Order[*Idx].Value = FMath::Clamp(Value, 0.0, MaxSkill);
}

double FPlayerSkills::OverallMastery() const
{
    if (_Order.Num() == 0)
    {
        return 0.0;
    }
    double Sum = 0.0;
    for (const FSkillDef& Entry : _Order)
    {
        Sum += Entry.Value;
    }
    return Sum / (static_cast<double>(_Order.Num()) * MaxSkill);
}

TArray<TPair<FString, double>> FPlayerSkills::ToDict() const
{
    TArray<TPair<FString, double>> Out;
    Out.Reserve(_Order.Num());
    for (const FSkillDef& Entry : _Order)
    {
        Out.Emplace(Entry.Id, Entry.Value);
    }
    return Out;
}

void FPlayerSkills::LoadDict(const TArray<TPair<FString, double>>& Data)
{
    for (const TPair<FString, double>& Pair : Data)
    {
        if (!_Index.Contains(Pair.Key))
        {
            continue;
        }
        SetLevel(Pair.Key, Pair.Value);
    }
}
