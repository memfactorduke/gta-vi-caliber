// Copyright (c) 2026 GTC contributors

#include "PlayerProgression.h"

const TArray<FPlayerProgression::FUnlockGate>& FPlayerProgression::UnlockTable()
{
    // Insertion-ordered mirror of the Godot UNLOCK_TABLE Dictionary.
    static const TArray<FUnlockGate> Table = {
        { 2,  { TEXT("pistol_slot") } },
        { 5,  { TEXT("sports_car"), TEXT("ammo_discount") } },
        { 10, { TEXT("smg_slot"), TEXT("garage_slot") } },
        { 20, { TEXT("helicopter"), TEXT("armor_discount") } },
        { 35, { TEXT("rifle_slot") } },
        { 50, { TEXT("heist_crew") } },
    };
    return Table;
}

FPlayerProgression::FPlayerProgression()
{
    Reset();
}

void FPlayerProgression::AddXp(int32 Amount)
{
    if (Amount <= 0)
    {
        return;
    }
    _TotalXp += Amount;
    if (_Level >= MaxLevel)
    {
        _XpIntoLevel = 0;
        return;
    }
    _XpIntoLevel += Amount;
    while (_Level < MaxLevel && _XpIntoLevel >= XpForNext())
    {
        _XpIntoLevel -= XpForNext();
        _Level += 1;
    }
    if (_Level >= MaxLevel)
    {
        _XpIntoLevel = 0;
    }
}

void FPlayerProgression::Reset()
{
    _Level = StartLevel;
    _XpIntoLevel = 0;
    _TotalXp = 0;
}

int32 FPlayerProgression::XpForNext() const
{
    if (_Level >= MaxLevel)
    {
        return 0;
    }
    return XpPerLevelStep * _Level;
}

double FPlayerProgression::LevelProgress() const
{
    if (_Level >= MaxLevel)
    {
        return 1.0;
    }
    const int32 Need = XpForNext();
    if (Need <= 0)
    {
        return 1.0;
    }
    return FMath::Clamp(static_cast<double>(_XpIntoLevel) / static_cast<double>(Need), 0.0, 1.0);
}

int32 FPlayerProgression::XpToReach(int32 TargetLevel)
{
    const int32 Clamped = FMath::Clamp(TargetLevel, StartLevel, MaxLevel);
    const int32 LevelsClimbed = Clamped - StartLevel;
    // Sum of 100*1 + 100*2 + ... + 100*LevelsClimbed (costs of levels 1..n).
    return XpPerLevelStep * LevelsClimbed * (LevelsClimbed + 1) / 2;
}

TArray<FString> FPlayerProgression::UnlocksAt(int32 TargetLevel)
{
    for (const FUnlockGate& Gate : UnlockTable())
    {
        if (Gate.GateLevel == TargetLevel)
        {
            return Gate.Features;
        }
    }
    return TArray<FString>();
}

bool FPlayerProgression::IsUnlocked(const FString& FeatureId) const
{
    for (const FUnlockGate& Gate : UnlockTable())
    {
        if (Gate.GateLevel <= _Level && Gate.Features.Contains(FeatureId))
        {
            return true;
        }
    }
    return false;
}
