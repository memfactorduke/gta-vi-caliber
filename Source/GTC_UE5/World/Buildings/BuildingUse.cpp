// Copyright Epic Games, Inc. All Rights Reserved.

#include "BuildingUse.h"

namespace
{
    // Public-facing kinds that read as storefronts (Godot SHOP_KINDS). Membership
    // is the only observable property, so a simple set lookup matches the oracle.
    bool IsShopKind(const FString& Kind)
    {
        return Kind == TEXT("retail")
            || Kind == TEXT("commercial")
            || Kind == TEXT("supermarket");
    }
}

bool FBuildingUse::IsShop(const FString& Kind)
{
    return IsShopKind(Kind);
}

TArray<FShopItem> FBuildingUse::CatalogueFor(const FString& /*Kind*/)
{
    return DefaultCatalogue();
}

TArray<FShopItem> FBuildingUse::DefaultCatalogue()
{
    return {
        FShopItem(TEXT("pistol"), TEXT("Pistol"), 500, TEXT("weapon")),
        FShopItem(TEXT("smg"), TEXT("SMG"), 2500, TEXT("weapon")),
        FShopItem(TEXT("rifle"), TEXT("Assault Rifle"), 7500, TEXT("weapon")),
        FShopItem(TEXT("ammo_box"), TEXT("Ammo Box"), 150, TEXT("ammo")),
        FShopItem(TEXT("body_armor"), TEXT("Body Armor"), 1000, TEXT("armor")),
        FShopItem(TEXT("sedan"), TEXT("Sedan"), 12000, TEXT("vehicle")),
        FShopItem(TEXT("sportscar"), TEXT("Sports Car"), 60000, TEXT("vehicle")),
    };
}
