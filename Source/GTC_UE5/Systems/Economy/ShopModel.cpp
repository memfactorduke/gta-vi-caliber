// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShopModel.h"

ShopModel::ShopModel(const TArray<FShopCatalogueItem>& Catalogue)
{
    const TArray<FShopCatalogueItem>& Source = Catalogue.Num() > 0 ? Catalogue : DefaultCatalogue();
    for (const FShopCatalogueItem& Entry : Source)
    {
        Register(Entry);
    }
}

TArray<FShopCatalogueItem> ShopModel::DefaultCatalogue()
{
    return {
        FShopCatalogueItem(TEXT("pistol"), TEXT("Pistol"), 500, TEXT("weapon")),
        FShopCatalogueItem(TEXT("smg"), TEXT("SMG"), 2500, TEXT("weapon")),
        FShopCatalogueItem(TEXT("rifle"), TEXT("Assault Rifle"), 7500, TEXT("weapon")),
        FShopCatalogueItem(TEXT("ammo_box"), TEXT("Ammo Box"), 150, TEXT("ammo")),
        FShopCatalogueItem(TEXT("body_armor"), TEXT("Body Armor"), 1000, TEXT("armor")),
        FShopCatalogueItem(TEXT("sedan"), TEXT("Sedan"), 12000, TEXT("vehicle")),
        FShopCatalogueItem(TEXT("sportscar"), TEXT("Sports Car"), 60000, TEXT("vehicle")),
    };
}

int32 ShopModel::ItemCount() const
{
    return Items.Num();
}

bool ShopModel::HasItem(const FString& Id) const
{
    return Index.Contains(Id);
}

int32 ShopModel::PriceOf(const FString& Id) const
{
    const FShopCatalogueItem* Item = Find(Id);
    return Item ? Item->Price : -1;
}

TArray<FShopCatalogueItem> ShopModel::ItemsInCategory(const FString& Category) const
{
    TArray<FShopCatalogueItem> Out;
    for (const FShopCatalogueItem& Item : Items)
    {
        if (Item.Category == Category)
        {
            Out.Add(Item);
        }
    }
    return Out;
}

bool ShopModel::CanAfford(const FString& Id, int32 Balance) const
{
    const int32 Price = PriceOf(Id);
    if (Price < 0)
    {
        return false;
    }
    return Balance >= Price;
}

int32 ShopModel::SellValue(const FString& Id, double Fraction) const
{
    const int32 Price = PriceOf(Id);
    if (Price < 0)
    {
        return 0;
    }
    const double ClampedFraction = FMath::Clamp(Fraction, 0.0, 1.0);
    return static_cast<int32>(FMath::RoundToDouble(static_cast<double>(Price) * ClampedFraction));
}

FShopPurchase ShopModel::Purchase(const FString& Id, int32 Balance) const
{
    const FShopCatalogueItem* Item = Find(Id);
    if (!Item)
    {
        return Fail(Balance, FString::Printf(TEXT("unknown item: %s"), *Id));
    }
    const int32 Price = Item->Price;
    if (Balance < Price)
    {
        return Fail(Balance, FString::Printf(TEXT("insufficient funds: need %d, have %d"), Price, Balance));
    }
    FShopPurchase Result;
    Result.bSuccess = true;
    Result.Cost = Price;
    Result.NewBalance = Balance - Price;
    Result.Reason = FString();
    return Result;
}

FShopPurchase ShopModel::Fail(int32 Balance, const FString& Reason)
{
    FShopPurchase Result;
    Result.bSuccess = false;
    Result.Cost = 0;
    Result.NewBalance = Balance;
    Result.Reason = Reason;
    return Result;
}

void ShopModel::Register(const FShopCatalogueItem& Entry)
{
    // Drop malformed rows: empty id or negative price. (Godot also drops non-int
    // price strings / non-dict rows; those are unrepresentable with a typed FShopCatalogueItem.)
    if (Entry.Id.IsEmpty() || Entry.Price < 0)
    {
        return;
    }
    FShopCatalogueItem Stored = Entry;
    if (Stored.Name.IsEmpty())
    {
        Stored.Name = Entry.Id;
    }
    if (Stored.Category.IsEmpty())
    {
        Stored.Category = TEXT("misc");
    }
    // Godot's _items[id] = {...} overwrites on a duplicate id (last-wins), keeping the
    // key's original insertion position. Mirror that: update in place if present.
    if (const int32* FoundIndex = Index.Find(Stored.Id))
    {
        Items[*FoundIndex] = Stored;
        return;
    }
    const int32 NewIndex = Items.Add(Stored);
    Index.Add(Stored.Id, NewIndex);
}

const FShopCatalogueItem* ShopModel::Find(const FString& Id) const
{
    const int32* FoundIndex = Index.Find(Id);
    return FoundIndex ? &Items[*FoundIndex] : nullptr;
}
