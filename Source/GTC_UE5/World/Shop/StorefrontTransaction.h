// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FShopCatalogueItem;
class ShopModel;

/**
 * FStorefrontTransaction — the pure, headless decision layer between the in-world
 * storefront ACTORS (AGTCStorefront / AGTCPaySprayShop / AGTCModShop) and the
 * already-tested economy models (ShopModel / PaySpray / VehicleModShop).
 *
 * The economy models answer "can this purchase resolve against a wallet balance?"
 * (ShopModel::Purchase &c.), but a ShopModel result carries only {success, cost,
 * new_balance} — NOT what the buyer receives. This helper adds the one piece of new,
 * non-UObject logic worth pinning in a test: classifying a catalogue item's free-form
 * Category string into a typed grant kind the actor switches on (give a weapon, top up
 * ammo, add armor, hand over a vehicle), and folding the purchase result into a single
 * decision the actor applies.
 *
 * No wallet mutation, no UObject, no world: the caller (the actor) applies the spend
 * via UPlayerStatsComponent and performs the grant. Mirrors the project's pure-core +
 * adapter split (PlayerMotion / FInteraction), so it unit-tests headless
 * (Tests/StorefrontTransactionTest.cpp, GTC.World.Shop.Transaction.*).
 */

/** What a successful shop purchase hands the player — drives the actor's grant switch. */
enum class EStorefrontGrantKind : uint8
{
    Weapon,
    Ammo,
    Armor,
    Vehicle,
    Unknown,
};

/** A resolved shop purchase: the ShopModel outcome plus the typed grant to hand over. */
struct GTC_UE5_API FStorefrontBuyDecision
{
    bool bSuccess = false;
    int32 Cost = 0;
    int32 NewBalance = 0;
    EStorefrontGrantKind Grant = EStorefrontGrantKind::Unknown;
    FString ItemName;
    FString Reason;
};

class GTC_UE5_API FStorefrontTransaction
{
public:
    /**
     * Classify a catalogue Category string into a typed grant kind. Case-insensitive;
     * the default ShopModel vocabulary is weapon / ammo / armor / vehicle. Anything
     * else (incl. empty) is Unknown, so an unrecognised category never silently maps
     * to the wrong grant.
     */
    static EStorefrontGrantKind GrantKindFromCategory(const FString& Category);

    /** Human-readable grant-kind name for logging. */
    static const TCHAR* GrantKindName(EStorefrontGrantKind Kind);

    /**
     * Resolve a buy of `Item` against `Balance` using `Shop`. Delegates the money
     * decision to the (tested) ShopModel::Purchase on Item.Id, then attaches the typed
     * grant (from Item.Category) and the display name. On failure the grant is Unknown,
     * the balance is unchanged, and Reason carries the model's explanation.
     */
    static FStorefrontBuyDecision ResolveBuy(const ShopModel& Shop, const FShopCatalogueItem& Item, int32 Balance);
};
