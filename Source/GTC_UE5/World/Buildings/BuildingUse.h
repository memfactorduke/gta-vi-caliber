// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * One catalogue entry for a building's shop stock: {Id, Name, Price, Category}.
 *
 * Mirrors the Godot ShopModel catalogue Dictionary
 * (game/scripts/systems/shop_model.gd default_catalogue). Plain value type, no
 * UObject. Price is an integer money unit, matching the Godot int prices.
 */
struct GTC_UE5_API FShopItem
{
    FString Id;
    FString Name;
    int32 Price = 0;
    FString Category;

    FShopItem() = default;
    FShopItem(const FString& InId, const FString& InName, int32 InPrice, const FString& InCategory)
        : Id(InId)
        , Name(InName)
        , Price(InPrice)
        , Category(InCategory)
    {
    }
};

/**
 * Classifies a building's OSM kind into how the player interacts with it — the
 * seam between "what a building is" and "what geometry represents it". OSM
 * footprints today and real 3D building assets later both carry a kind, so the
 * same classifier drives interaction wiring straight across the asset swap.
 *
 * Ported 1:1 from the Godot RefCounted `BuildingUse`
 * (game/scripts/world/building_use.gd). Pure static helpers, no UObject — unit
 * tested headless via the parity oracle (World/Buildings/Tests/BuildingUseTest.cpp).
 *
 * The catalogue lives here (not in a separate ShopModel port): `CatalogueFor`
 * returns the general-store default stock that the Godot source hands back for
 * every shop kind, so this port is self-contained and parity-literal with the
 * oracle.
 */
class GTC_UE5_API FBuildingUse
{
public:
    /** True when a building of this kind should host a street-level shop. */
    static bool IsShop(const FString& Kind);

    /**
     * The shop stock for a kind, as a catalogue Array. For now every shop carries
     * the general-store stock; the Kind parameter is reserved for future per-kind
     * variation (matching the Godot `_kind` placeholder).
     */
    static TArray<FShopItem> CatalogueFor(const FString& Kind);

    /** The built-in general-store stock (Godot ShopModel.default_catalogue). */
    static TArray<FShopItem> DefaultCatalogue();
};
