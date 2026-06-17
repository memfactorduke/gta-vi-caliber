// Copyright (c) 2026 GTC contributors

#include "CharacterRoster.h"

FCharacterRoster::FCharacterRoster(const TArray<FRosterCharacter>& Characters)
{
    const TArray<FRosterCharacter>& Source = Characters.Num() > 0 ? Characters : DefaultCharacters();
    for (const FRosterCharacter& Entry : Source)
    {
        Register(Entry);
    }
    if (Order.Num() > 0)
    {
        ActiveId = Order[0];
    }
}

TArray<FRosterCharacter> FCharacterRoster::DefaultCharacters()
{
    return {
        FRosterCharacter(TEXT("mara"), TEXT("Mara"), 2500),
        FRosterCharacter(TEXT("rico"), TEXT("Rico"), 1500),
    };
}

int32 FCharacterRoster::CharacterCount() const
{
    return Order.Num();
}

bool FCharacterRoster::HasCharacter(const FString& Id) const
{
    return Characters.Contains(Id);
}

TArray<FString> FCharacterRoster::Ids() const
{
    return Order;
}

const FString& FCharacterRoster::Active() const
{
    return ActiveId;
}

FString FCharacterRoster::ActiveName() const
{
    return NameOf(ActiveId);
}

FString FCharacterRoster::NameOf(const FString& Id) const
{
    const FState* State = Find(Id);
    return State ? State->Name : FString();
}

bool FCharacterRoster::CanSwitch(const FString& Id, double Now) const
{
    if (!Characters.Contains(Id) || Id == ActiveId)
    {
        return false;
    }
    return Now - LastSwitchAt >= SwitchCooldown;
}

bool FCharacterRoster::SwitchTo(const FString& Id, double Now)
{
    if (!CanSwitch(Id, Now))
    {
        return false;
    }
    ActiveId = Id;
    // A caller that doesn't track time (Now == +inf sentinel) must never be cooldown-blocked
    // on the next switch, so park the stamp at -inf instead of +inf (Godot: is_inf(now) guard).
    const bool bUntimed = Now == TNumericLimits<double>::Max();
    LastSwitchAt = bUntimed ? -TNumericLimits<double>::Max() : Now;
    return true;
}

int32 FCharacterRoster::MoneyOf(const FString& Id) const
{
    const FState* State = Find(Id);
    return State ? State->Money : 0;
}

void FCharacterRoster::AddMoney(const FString& Id, int32 Amount)
{
    if (FState* State = Find(Id))
    {
        State->Money = FMath::Max(0, State->Money + Amount);
    }
}

void FCharacterRoster::SetMoney(const FString& Id, int32 Amount)
{
    if (FState* State = Find(Id))
    {
        State->Money = FMath::Max(0, Amount);
    }
}

int32 FCharacterRoster::WantedOf(const FString& Id) const
{
    const FState* State = Find(Id);
    return State ? State->Wanted : 0;
}

void FCharacterRoster::SetWanted(const FString& Id, int32 Stars)
{
    if (FState* State = Find(Id))
    {
        State->Wanted = FMath::Clamp(Stars, 0, MaxStars);
    }
}

FVector FCharacterRoster::PositionOf(const FString& Id) const
{
    const FState* State = Find(Id);
    return State ? State->Position : FVector::ZeroVector;
}

void FCharacterRoster::SetPosition(const FString& Id, const FVector& Pos)
{
    if (FState* State = Find(Id))
    {
        State->Position = Pos;
    }
}

void FCharacterRoster::Serialize(FString& OutActive, TArray<TPair<FString, FSnapshot>>& OutCharacters) const
{
    OutActive = ActiveId;
    OutCharacters.Reset();
    for (const FString& Id : Order)
    {
        const FState& State = Characters[Id];
        FSnapshot Snapshot;
        Snapshot.Name = State.Name;
        Snapshot.Money = State.Money;
        Snapshot.Wanted = State.Wanted;
        Snapshot.Position = State.Position;
        OutCharacters.Emplace(Id, Snapshot);
    }
}

void FCharacterRoster::Deserialize(const FString& InActive, const TArray<TPair<FString, FSnapshot>>& InCharacters)
{
    for (const TPair<FString, FSnapshot>& Pair : InCharacters)
    {
        // Unknown ids are ignored; known characters updated in place.
        if (FState* State = Find(Pair.Key))
        {
            State->Money = FMath::Max(0, Pair.Value.Money);
            State->Wanted = FMath::Clamp(Pair.Value.Wanted, 0, MaxStars);
            State->Position = Pair.Value.Position;
        }
    }
    if (Characters.Contains(InActive))
    {
        ActiveId = InActive;
    }
}

void FCharacterRoster::Register(const FRosterCharacter& Entry)
{
    // Drop malformed rows: empty id. A duplicate id keeps the first registration
    // (Godot: `if id.is_empty() or _characters.has(id): return`).
    if (Entry.Id.IsEmpty() || Characters.Contains(Entry.Id))
    {
        return;
    }
    FState State;
    State.Name = Entry.Name.IsEmpty() ? Entry.Id : Entry.Name;
    State.Money = FMath::Max(0, Entry.Money);
    State.Wanted = 0;
    State.Position = FVector::ZeroVector;
    Characters.Add(Entry.Id, State);
    Order.Add(Entry.Id);
}

FCharacterRoster::FState* FCharacterRoster::Find(const FString& Id)
{
    return Characters.Find(Id);
}

const FCharacterRoster::FState* FCharacterRoster::Find(const FString& Id) const
{
    return Characters.Find(Id);
}
