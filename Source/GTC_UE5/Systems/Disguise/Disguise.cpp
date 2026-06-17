// Copyright (c) 2026 GTC contributors

#include "Disguise.h"

const FString FDisguise::DefaultLook = TEXT("default");

const TArray<FString>& FDisguise::OrderedSlots()
{
    // the reference WEIGHTS key order: outfit, mask, vehicle, hair.
    static const TArray<FString> Slots = {
        TEXT("outfit"), TEXT("mask"), TEXT("vehicle"), TEXT("hair")};
    return Slots;
}

double FDisguise::WeightOf(const FString& Slot)
{
    // the reference WEIGHTS: outfit 0.3, mask 0.4, vehicle 0.2, hair 0.1 (sum 1.0).
    if (Slot == TEXT("outfit")) { return 0.3; }
    if (Slot == TEXT("mask")) { return 0.4; }
    if (Slot == TEXT("vehicle")) { return 0.2; }
    if (Slot == TEXT("hair")) { return 0.1; }
    return 0.0;
}

FDisguise::FDisguise()
{
    for (const FString& Slot : OrderedSlots())
    {
        _Current.Add(Slot, DefaultLook);
    }
}

TArray<FString> FDisguise::Slots() const
{
    return OrderedSlots();
}

FString FDisguise::Current(const FString& Slot) const
{
    if (const FString* Value = _Current.Find(Slot))
    {
        return *Value;
    }
    return FString();
}

void FDisguise::SetAppearance(const FString& Slot, const FString& Value)
{
    if (_Current.Contains(Slot))
    {
        _Current[Slot] = Value;
    }
}

void FDisguise::LogSighting()
{
    _WantedLook = _Current;
    _HasWantedLook = true;
}

bool FDisguise::HasDescription() const
{
    return _HasWantedLook;
}

double FDisguise::Recognition() const
{
    if (!_HasWantedLook)
    {
        return 0.0;
    }
    double Score = 0.0;
    for (const FString& Slot : OrderedSlots())
    {
        const FString* Cur = _Current.Find(Slot);
        const FString* Wanted = _WantedLook.Find(Slot);
        const bool bCurMatch = (Cur != nullptr);
        const bool bWantedMatch = (Wanted != nullptr);
        // the reference compares _current.get(slot) == _wanted_look.get(slot); both null -> equal.
        if (bCurMatch == bWantedMatch && (!bCurMatch || *Cur == *Wanted))
        {
            Score += WeightOf(Slot);
        }
    }
    return Score;
}

double FDisguise::EvasionSpeedup() const
{
    return 1.0 + (1.0 - Recognition()) * (MaxEvasionSpeedup - 1.0);
}

bool FDisguise::IsRecognized(double Threshold) const
{
    return Recognition() >= Threshold;
}

int32 FDisguise::ChangedSlots() const
{
    if (!_HasWantedLook)
    {
        return 0;
    }
    int32 Count = 0;
    for (const FString& Slot : OrderedSlots())
    {
        const FString* Cur = _Current.Find(Slot);
        const FString* Wanted = _WantedLook.Find(Slot);
        const bool bCurMatch = (Cur != nullptr);
        const bool bWantedMatch = (Wanted != nullptr);
        // Differs when one side is missing, or both present and unequal.
        if (bCurMatch != bWantedMatch || (bCurMatch && *Cur != *Wanted))
        {
            ++Count;
        }
    }
    return Count;
}

void FDisguise::ResetToClean()
{
    _WantedLook.Empty();
    _HasWantedLook = false;
}
