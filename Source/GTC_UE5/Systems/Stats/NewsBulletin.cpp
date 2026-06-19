// Copyright Epic Games, Inc. All Rights Reserved.

#include "NewsBulletin.h"

const FString FNewsBulletin::Filler = TEXT("And now the weather: another sunny day on the coast.");

const TArray<TPair<FString, TArray<FString>>>& FNewsBulletin::HeadlineTable()
{
    // Mirrors the Godot HEADLINES Dictionary in declaration order; each kind's
    // templates ascend by severity (index = clamp(severity-1, 0, last)). Built via
    // Emplace because TPair has no aggregate brace constructor.
    static const TArray<TPair<FString, TArray<FString>>> Table = []()
    {
        TArray<TPair<FString, TArray<FString>>> T;
        T.Emplace(TEXT("crime"), TArray<FString>{
            TEXT("Petty crime reported in {place}."),
            TEXT("Armed robbery rocks {place}."),
            TEXT("Crime wave grips {place}."),
            TEXT("{place} on edge after a violent spree."),
            TEXT("City-wide manhunt underway across {place}."),
        });
        T.Emplace(TEXT("rampage"), TArray<FString>{
            TEXT("Reports of gunfire in {place}."),
            TEXT("Streets clear as chaos erupts in {place}."),
            TEXT("{place} locked down amid a violent rampage."),
        });
        T.Emplace(TEXT("heist"), TArray<FString>{
            TEXT("Bank alarm tripped in {place}."),
            TEXT("Daring daylight heist nets thousands in {place}."),
            TEXT("Brazen vault robbery stuns {place}."),
            TEXT("Record-breaking heist baffles {place} police."),
            TEXT("The heist of the century leaves {place} reeling."),
        });
        T.Emplace(TEXT("spree"), TArray<FString>{
            TEXT("Police link a suspect to {count} incident(s)."),
            TEXT("Crime spree: {count} offences and counting."),
            TEXT("A {count}-crime rampage shocks the city."),
        });
        T.Emplace(TEXT("escape"), TArray<FString>{
            TEXT("A suspect slips the police cordon."),
            TEXT("Wanted fugitive vanishes into the night."),
            TEXT("Manhunt called off — suspect still at large."),
        });
        T.Emplace(TEXT("stunt"), TArray<FString>{
            TEXT("Daredevil stunt caught on camera in {place}."),
            TEXT("A wild jump stuns onlookers in {place}."),
        });
        return T;
    }();
    return Table;
}

const TArray<FString>* FNewsBulletin::FindTemplates(const FString& Kind)
{
    for (const TPair<FString, TArray<FString>>& Pair : HeadlineTable())
    {
        if (Pair.Key == Kind)
        {
            return &Pair.Value;
        }
    }
    return nullptr;
}

TArray<FString> FNewsBulletin::Kinds() const
{
    TArray<FString> Out;
    Out.Reserve(HeadlineTable().Num());
    for (const TPair<FString, TArray<FString>>& Pair : HeadlineTable())
    {
        Out.Add(Pair.Key);
    }
    return Out;
}

FString FNewsBulletin::Report(const FString& Kind, int32 Severity, const FString& Place, int32 Count)
{
    const TArray<FString>* Templates = FindTemplates(Kind);
    if (Templates == nullptr)
    {
        return FString();
    }
    // Godot clampi(severity - 1, 0, templates.size() - 1).
    const int32 Idx = FMath::Clamp(Severity - 1, 0, Templates->Num() - 1);
    // Godot String.format({"place": ..., "count": ...}); templates carry only one of the
    // two placeholders, so replacing both unconditionally mirrors .format exactly.
    FString Text = (*Templates)[Idx];
    Text = Text.Replace(TEXT("{place}"), *Place);
    Text = Text.Replace(TEXT("{count}"), *FString::FromInt(Count));

    FItem Item;
    Item.Kind = Kind;
    Item.Severity = FMath::Clamp(Severity, 1, 5);
    Item.Text = Text;
    _Queue.Add(Item);

    while (_Queue.Num() > MaxQueue)
    {
        _Queue.RemoveAt(0);
    }
    return Text;
}

FString FNewsBulletin::NextBulletin()
{
    if (_Queue.Num() == 0)
    {
        return Filler;
    }
    const FString Text = _Queue[0].Text;
    _Queue.RemoveAt(0);
    return Text;
}

FString FNewsBulletin::PeekLatest() const
{
    if (_Queue.Num() == 0)
    {
        return FString();
    }
    return _Queue.Last().Text;
}

TArray<FString> FNewsBulletin::Recent(int32 Limit) const
{
    TArray<FString> Out;
    int32 i = _Queue.Num() - 1;
    while (i >= 0 && Out.Num() < Limit)
    {
        Out.Add(_Queue[i].Text);
        --i;
    }
    return Out;
}

void FNewsBulletin::Clear()
{
    _Queue.Reset();
}
