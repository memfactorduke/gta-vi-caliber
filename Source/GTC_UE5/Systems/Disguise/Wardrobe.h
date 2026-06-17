// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * FWardrobe — pure clothing-wardrobe model (UE 5.7 port of the reference `wardrobe.gd`, class
 * Wardrobe, RefCounted). Parity oracle game/tests/unit/test_wardrobe.gd (12 funcs).
 *
 * Buy outfits, own them, and wear one per clothing slot (outfit / hair / mask). What you're
 * wearing feeds FDisguise: each worn item has a `Look` value pushed into
 * FDisguise::SetAppearance(slot, look). Distinct from a generic shop: this tracks ownership
 * + the currently-worn look per slot. Each item is {Id, Name, Slot, Price, Look, Owned};
 * malformed entries (missing/empty id, unknown slot, negative price, duplicate id) are dropped.
 *
 * Ordered backing store: the reference _catalogue / _owned / _worn are Dictionaries whose insertion
 * order is observable through ids() / items_in_slot() and the starter-wear loop; we keep a
 * parallel ordered id list (and ordered owned list) so iteration order matches the oracle.
 * buy() never mutates a wallet — it returns the proposed new balance for the caller to apply.
 */

/** One catalogue entry. Mirrors the Godot _catalogue value {name, slot, price, look}. */
struct GTC_UE5_API FWardrobeItem
{
    FString Name;
    FString Slot;
    int32 Price = 0;
    FString Look;
};

/** Result of FWardrobe::Buy. Mirrors the Godot {success, cost, new_balance, reason} Dictionary. */
struct GTC_UE5_API FWardrobeBuyResult
{
    bool bSuccess = false;
    int32 Cost = 0;
    int32 NewBalance = 0;
    FString Reason;
};

class GTC_UE5_API FWardrobe
{
public:
    /** Wearable clothing slots (a subset of Disguise's slots — vehicle isn't clothing). */
    static const TArray<FString>& Slots();

    /** One raw catalogue input entry passed to the constructor (pre-validation). */
    struct FInputItem
    {
        FString Id;
        FString Name;
        FString Slot;
        int32 Price = 0;
        FString Look;
        bool bOwned = false;
        /** Whether Name/Look were explicitly provided (the reference defaults them to Id when absent). */
        bool bHasName = false;
        bool bHasLook = false;
    };

    /** Construct from an explicit item list, or the built-in catalogue when Items is empty. */
    explicit FWardrobe(const TArray<FInputItem>& Items = TArray<FInputItem>());

    /** Built-in catalogue; the starter casual + buzz cut are owned and worn from new. */
    static TArray<FInputItem> DefaultItems();

    int32 ItemCount() const { return _CatalogueOrder.Num(); }
    bool HasItem(const FString& Id) const { return _Catalogue.Contains(Id); }

    /** Catalogue ids in insertion order. */
    TArray<FString> Ids() const { return _CatalogueOrder; }

    /** Catalogue price of an item (-1 if unknown). */
    int32 PriceOf(const FString& Id) const;

    /** The slot an item occupies ("" if unknown). */
    FString SlotOf(const FString& Id) const;

    /** The Disguise appearance value an item maps to ("" if unknown). */
    FString LookOf(const FString& Id) const;

    /** Catalogue ids in a slot, in insertion order. */
    TArray<FString> ItemsInSlot(const FString& Slot) const;

    bool Owns(const FString& Id) const { return _Owned.Contains(Id); }

    /**
     * Buy an item against a wallet balance. Never mutates the wallet. Fails on unknown id,
     * already owned, or insufficient funds.
     */
    FWardrobeBuyResult Buy(const FString& Id, int32 Balance);

    /** Wear an owned item (replaces whatever was in its slot). False if unknown or not owned. */
    bool Wear(const FString& Id);

    /** Take off whatever's in a slot. */
    void TakeOff(const FString& Slot);

    /** The item id worn in a slot ("" if nothing). */
    FString WornIn(const FString& Slot) const;

    /** The Disguise look value worn in a slot ("" if nothing). */
    FString WornLook(const FString& Slot) const;

    /** {slot: look} for every worn slot — push into FDisguise::SetAppearance per slot. */
    TMap<FString, FString> WornLooks() const;

private:
    void Register(const FInputItem& Entry);
    static FWardrobeBuyResult Fail(int32 Balance, const FString& Reason);

    /** id -> entry. */
    TMap<FString, FWardrobeItem> _Catalogue;
    /** Catalogue ids in insertion order (the reference Dictionary key order). */
    TArray<FString> _CatalogueOrder;
    /** Owned ids in insertion order. */
    TSet<FString> _Owned;
    TArray<FString> _OwnedOrder;
    /** slot -> worn item id. */
    TMap<FString, FString> _Worn;
    TArray<FString> _WornOrder;
};
