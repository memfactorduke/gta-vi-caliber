// Copyright (c) 2026 GTC contributors

#include "Wardrobe.h"

const TArray<FString>& FWardrobe::Slots()
{
    static const TArray<FString> S = {TEXT("outfit"), TEXT("hair"), TEXT("mask")};
    return S;
}

TArray<FWardrobe::FInputItem> FWardrobe::DefaultItems()
{
    TArray<FInputItem> Items;

    auto Add = [&Items](const FString& Id, const FString& Name, const FString& Slot,
                        int32 Price, const FString& Look, bool bOwned)
    {
        FInputItem Item;
        Item.Id = Id;
        Item.Name = Name;
        Item.bHasName = true;
        Item.Slot = Slot;
        Item.Price = Price;
        Item.Look = Look;
        Item.bHasLook = true;
        Item.bOwned = bOwned;
        Items.Add(Item);
    };

    Add(TEXT("street_casual"), TEXT("Street Casual"), TEXT("outfit"), 0, TEXT("casual"), true);
    Add(TEXT("sharp_suit"), TEXT("Sharp Suit"), TEXT("outfit"), 1500, TEXT("suit"), false);
    Add(TEXT("track_suit"), TEXT("Track Suit"), TEXT("outfit"), 400, TEXT("tracksuit"), false);
    Add(TEXT("buzz_cut"), TEXT("Buzz Cut"), TEXT("hair"), 0, TEXT("buzz"), true);
    Add(TEXT("blonde_dye"), TEXT("Blonde Dye"), TEXT("hair"), 300, TEXT("blonde"), false);
    Add(TEXT("ski_mask"), TEXT("Ski Mask"), TEXT("mask"), 250, TEXT("ski_mask"), false);

    return Items;
}

FWardrobe::FWardrobe(const TArray<FInputItem>& Items)
{
    const TArray<FInputItem>& Source = Items.Num() > 0 ? Items : DefaultItems();
    for (const FInputItem& Entry : Source)
    {
        Register(Entry);
    }
    // Wear the first owned starter in each slot (Godot iterates _owned in insertion order).
    for (const FString& Id : _OwnedOrder)
    {
        const FString& Slot = _Catalogue[Id].Slot;
        if (!_Worn.Contains(Slot))
        {
            _Worn.Add(Slot, Id);
            _WornOrder.Add(Slot);
        }
    }
}

void FWardrobe::Register(const FInputItem& Entry)
{
    // Godot: drop non-dict / missing id handled by caller; here every FInputItem has an Id field,
    // but an absent id in Godot maps to an empty Id here (rejected below).
    const FString& Id = Entry.Id;
    const FString& Slot = Entry.Slot;
    const int32 Price = Entry.Price;
    if (Id.IsEmpty() || _Catalogue.Contains(Id) || !Slots().Contains(Slot) || Price < 0)
    {
        return;
    }
    FWardrobeItem Item;
    Item.Name = Entry.bHasName ? Entry.Name : Id;
    Item.Slot = Slot;
    Item.Price = Price;
    Item.Look = Entry.bHasLook ? Entry.Look : Id;
    _Catalogue.Add(Id, Item);
    _CatalogueOrder.Add(Id);
    if (Entry.bOwned)
    {
        _Owned.Add(Id);
        _OwnedOrder.Add(Id);
    }
}

int32 FWardrobe::PriceOf(const FString& Id) const
{
    if (const FWardrobeItem* Item = _Catalogue.Find(Id))
    {
        return Item->Price;
    }
    return -1;
}

FString FWardrobe::SlotOf(const FString& Id) const
{
    if (const FWardrobeItem* Item = _Catalogue.Find(Id))
    {
        return Item->Slot;
    }
    return FString();
}

FString FWardrobe::LookOf(const FString& Id) const
{
    if (const FWardrobeItem* Item = _Catalogue.Find(Id))
    {
        return Item->Look;
    }
    return FString();
}

TArray<FString> FWardrobe::ItemsInSlot(const FString& Slot) const
{
    TArray<FString> Out;
    for (const FString& Id : _CatalogueOrder)
    {
        if (_Catalogue[Id].Slot == Slot)
        {
            Out.Add(Id);
        }
    }
    return Out;
}

FWardrobeBuyResult FWardrobe::Fail(int32 Balance, const FString& Reason)
{
    FWardrobeBuyResult R;
    R.bSuccess = false;
    R.Cost = 0;
    R.NewBalance = Balance;
    R.Reason = Reason;
    return R;
}

FWardrobeBuyResult FWardrobe::Buy(const FString& Id, int32 Balance)
{
    if (!_Catalogue.Contains(Id))
    {
        return Fail(Balance, FString::Printf(TEXT("no such item: %s"), *Id));
    }
    if (_Owned.Contains(Id))
    {
        return Fail(Balance, TEXT("already owned"));
    }
    const int32 Cost = _Catalogue[Id].Price;
    if (Balance < Cost)
    {
        return Fail(Balance, FString::Printf(TEXT("insufficient funds: need %d, have %d"), Cost, Balance));
    }
    _Owned.Add(Id);
    _OwnedOrder.Add(Id);
    FWardrobeBuyResult R;
    R.bSuccess = true;
    R.Cost = Cost;
    R.NewBalance = Balance - Cost;
    R.Reason = FString();
    return R;
}

bool FWardrobe::Wear(const FString& Id)
{
    if (!_Owned.Contains(Id))
    {
        return false;
    }
    const FString& Slot = _Catalogue[Id].Slot;
    if (!_Worn.Contains(Slot))
    {
        _WornOrder.Add(Slot);
    }
    _Worn.Add(Slot, Id);
    return true;
}

void FWardrobe::TakeOff(const FString& Slot)
{
    if (_Worn.Remove(Slot) > 0)
    {
        _WornOrder.Remove(Slot);
    }
}

FString FWardrobe::WornIn(const FString& Slot) const
{
    if (const FString* Id = _Worn.Find(Slot))
    {
        return *Id;
    }
    return FString();
}

FString FWardrobe::WornLook(const FString& Slot) const
{
    const FString Id = WornIn(Slot);
    return Id.IsEmpty() ? FString() : LookOf(Id);
}

TMap<FString, FString> FWardrobe::WornLooks() const
{
    TMap<FString, FString> Out;
    for (const FString& Slot : _WornOrder)
    {
        Out.Add(Slot, LookOf(_Worn[Slot]));
    }
    return Out;
}
