// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * A building footprint record: a display name and an OSM kind. Mirrors the Godot
 * Dictionary the classifier reads (`name`, `kind` keys via .get with "" default).
 *
 * Typed-port note: the the reference source reads an untyped Dictionary and defaults
 * missing keys to "". In typed C++ both fields always exist; an absent the reference key
 * is modelled by an empty string, which reproduces the surviving valid-input
 * behaviour exactly (no malformed-input branch to port).
 */
struct GTC_UE5_API FBuilding
{
    FString Name;
    FString Kind;

    FBuilding() = default;
    FBuilding(const FString& InName, const FString& InKind)
        : Name(InName)
        , Kind(InKind)
    {
    }
};

/**
 * Decides which real buildings the player can enter and where their door is — the
 * data layer beneath interiors/shops. A building is enterable if it is named or
 * has a public-facing type (shops, offices, civic). The door is placed at the
 * midpoint of the footprint's longest edge (its likely street frontage).
 *
 * Ported 1:1 from the the reference RefCounted `Enterable`
 * (game/scripts/world/enterable.gd). Pure static helpers, no UObject — unit
 * tested headless via the reference behavior (World/Buildings/Tests/EnterableTest.cpp).
 *
 * Double precision: `FVector2D` is double-precision under UE5 LWC, matching the
 * the reference float frame for the door-edge geometry.
 *
 * NOTE: no Godot->UE Z-up remap — footprints stay in the Godot 2D ground frame.
 * Porting the axis convention to UE's Z-up space is a deferred Wave 3 concern.
 */
class GTC_UE5_API FEnterable
{
public:
    /** True when the building is named or has a public-facing (interior) type. */
    static bool IsEnterable(const FBuilding& Building);

    /**
     * Midpoint of the longest footprint edge (in local metres) — the street door.
     * An empty footprint yields the origin; a single point yields that point.
     */
    static FVector2D DoorPoint(const TArray<FVector2D>& Footprint);

    /**
     * Enterable subset of a district's buildings, capped to keep interiors
     * bounded. Preserves input order and stops once MaxCount are collected.
     */
    static TArray<FBuilding> Pick(const TArray<FBuilding>& Buildings, int32 MaxCount);
};
