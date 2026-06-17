// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure shop/store economy model — spends the money the player earns on weapons,
 * ammo, armor, and vehicles. Plain C++ value type (no UObject): a purchase resolves
 * against a wallet balance the caller passes in, returning the result so the caller
 * applies the spend. Headless-testable (parity oracle test_shop_model.gd).
 *
 * Each catalogue entry is {Id, Name, Price, Category}. Garbage entries (missing id,
 * negative price) are dropped at construction.
 *
 * Parity note: Godot's catalogue Dictionary is insertion-ordered. items_in_category
 * iterates it; an explicit ordered backing store (TArray + index map) preserves that
 * observable order. Godot's typed-Variant garbage filtering (non-int price strings,
 * non-dict rows) has no analogue under a typed FShopCatalogueItem — those rows can't be
 * constructed here, so the corresponding oracle rows are covered by the negative-price
 * / empty-id paths that ARE representable.
 */
struct GTC_UE5_API FShopCatalogueItem
{
    FString Id;
    FString Name;
    int32 Price = 0;
    FString Category;

    FShopCatalogueItem() = default;
    FShopCatalogueItem(const FString& InId, const FString& InName, int32 InPrice, const FString& InCategory)
        : Id(InId)
        , Name(InName)
        , Price(InPrice)
        , Category(InCategory)
    {
    }
};

/** Result of a purchase attempt — mirrors Godot's {success, cost, new_balance, reason}. */
struct GTC_UE5_API FShopPurchase
{
    bool bSuccess = false;
    int32 Cost = 0;
    int32 NewBalance = 0;
    FString Reason;
};

class GTC_UE5_API ShopModel
{
public:
    /** Fraction of an item's price recovered when selling it back (50%). */
    static constexpr double DefaultSellFraction = 0.5;

    /** Construct from a catalogue; an empty list uses DefaultCatalogue(). */
    explicit ShopModel(const TArray<FShopCatalogueItem>& Catalogue = TArray<FShopCatalogueItem>());

    /** The built-in stock used when an empty catalogue is passed. */
    static TArray<FShopCatalogueItem> DefaultCatalogue();

    int32 ItemCount() const;
    bool HasItem(const FString& Id) const;

    /** Price of an item, or -1 if the id is unknown. */
    int32 PriceOf(const FString& Id) const;

    /** Every item in a category (empty if none / unknown category), insertion-ordered. */
    TArray<FShopCatalogueItem> ItemsInCategory(const FString& Category) const;

    /** True when the balance covers the item's price. Unknown id is never affordable. */
    bool CanAfford(const FString& Id, int32 Balance) const;

    /** What an item sells back for at the given fraction (clamped [0,1]). Unknown id -> 0. */
    int32 SellValue(const FString& Id, double Fraction = DefaultSellFraction) const;

    /** Resolve a purchase against a wallet balance; never mutates state. */
    FShopPurchase Purchase(const FString& Id, int32 Balance) const;

private:
    /** Insertion-ordered catalogue. */
    TArray<FShopCatalogueItem> Items;
    /** Id -> index into Items. */
    TMap<FString, int32> Index;

    static FShopPurchase Fail(int32 Balance, const FString& Reason);

    void Register(const FShopCatalogueItem& Entry);
    const FShopCatalogueItem* Find(const FString& Id) const;
};
