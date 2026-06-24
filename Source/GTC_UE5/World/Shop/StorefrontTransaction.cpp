// Copyright Epic Games, Inc. All Rights Reserved.

#include "StorefrontTransaction.h"

#include "../../Systems/Economy/ShopModel.h"

EStorefrontGrantKind FStorefrontTransaction::GrantKindFromCategory(const FString& Category)
{
    const FString C = Category.ToLower();
    if (C == TEXT("weapon"))
    {
        return EStorefrontGrantKind::Weapon;
    }
    if (C == TEXT("ammo"))
    {
        return EStorefrontGrantKind::Ammo;
    }
    if (C == TEXT("armor"))
    {
        return EStorefrontGrantKind::Armor;
    }
    if (C == TEXT("vehicle"))
    {
        return EStorefrontGrantKind::Vehicle;
    }
    return EStorefrontGrantKind::Unknown;
}

const TCHAR* FStorefrontTransaction::GrantKindName(EStorefrontGrantKind Kind)
{
    switch (Kind)
    {
    case EStorefrontGrantKind::Weapon:
        return TEXT("weapon");
    case EStorefrontGrantKind::Ammo:
        return TEXT("ammo");
    case EStorefrontGrantKind::Armor:
        return TEXT("armor");
    case EStorefrontGrantKind::Vehicle:
        return TEXT("vehicle");
    default:
        return TEXT("unknown");
    }
}

FStorefrontBuyDecision FStorefrontTransaction::ResolveBuy(
    const ShopModel& Shop, const FShopCatalogueItem& Item, int32 Balance)
{
    // The money decision is the tested ShopModel::Purchase (keyed on the item id);
    // this layer only attaches the typed grant + display name on top of it.
    const FShopPurchase Purchase = Shop.Purchase(Item.Id, Balance);

    FStorefrontBuyDecision Decision;
    Decision.bSuccess = Purchase.bSuccess;
    Decision.Cost = Purchase.Cost;
    Decision.NewBalance = Purchase.NewBalance;
    Decision.Reason = Purchase.Reason;
    Decision.ItemName = Item.Name;
    Decision.Grant = Purchase.bSuccess ? GrantKindFromCategory(Item.Category) : EStorefrontGrantKind::Unknown;
    return Decision;
}
