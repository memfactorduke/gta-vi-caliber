// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure owned-weapon / ammo model for the player's arsenal and weapon-wheel.
 * Direct port of the Godot `WeaponInventory` (RefCounted) at
 * `game/scripts/weapons/weapon_inventory.gd`.
 *
 * No scene access — a node (e.g. WeaponController) owns one and asks it what is
 * equipped and whether a shot can go out, so the slot/ammo/reload logic is
 * unit-tested (Tests/WeaponInventoryTest.cpp, prefix GTC.Weapons.Core.WeaponInventory).
 *
 * Each owned weapon is one slot keyed by a string id, holding its own magazine
 * (chambered rounds) and reserve (spare) ammo independently. Acquiring a weapon
 * already owned just tops up its reserve instead of adding a second slot.
 *
 * The inventory starts with the always-present unarmed "fists" slot equipped, so
 * CurrentId() is never empty and the wheel always has at least one entry.
 *
 * Parity note: Godot's Array/Dictionary iteration is insertion-ordered and
 * OwnedIds()/cycling rely on it, so the slots are stored as an ordered TArray
 * with a companion TMap for O(1) id lookup (the GarageStorage pattern).
 */
class GTC_UE5_API WeaponInventory
{
public:
    static const FString UnarmedId;

    /** Starts owning only the unarmed "fists" slot, equipped. */
    WeaponInventory();

    /**
     * Acquire a weapon. If already owned, top up its reserve ammo instead of adding
     * a second slot (MagSize of the existing slot is kept). Negative inputs clamp to
     * zero. The newly acquired weapon starts with a full magazine. Empty id is a no-op.
     */
    void AddWeapon(const FString& Id, int32 MagSize = 0, int32 Ammo = 0);

    /** Add spare (reserve) ammo to an owned weapon. No-op if unowned or Amount <= 0. */
    void AddAmmo(const FString& Id, int32 Amount);

    bool HasWeapon(const FString& Id) const;

    int32 WeaponCount() const;

    /** Owned weapon ids in wheel order (a copy, so callers cannot mutate state). */
    TArray<FString> OwnedIds() const;

    /**
     * Equip an owned weapon. Returns false and leaves the current weapon unchanged
     * if the id is not owned.
     */
    bool Equip(const FString& Id);

    FString CurrentId() const;

    /** Cycle to the next owned weapon (wraps). Returns the newly equipped id. */
    FString NextWeapon();

    /** Cycle to the previous owned weapon (wraps). Returns the newly equipped id. */
    FString PreviousWeapon();

    /** Whether the current weapon has a round chambered (mag not empty). */
    bool CanFire() const;

    /**
     * Consume one round from the current magazine. Returns whether a shot went out;
     * false (and consumes nothing) when the magazine is empty.
     */
    bool Fire();

    int32 AmmoInMag() const;

    int32 ReserveAmmo() const;

    /**
     * Refill the current magazine from reserve up to MagSize. Returns how many rounds
     * were loaded: a partial reload when reserve is low, 0 when already full or the
     * reserve is empty.
     */
    int32 Reload();

private:
    struct FSlot
    {
        FString Id;
        int32 MagSize = 0;
        int32 Mag = 0;
        int32 Reserve = 0;
    };

    /** Insertion-ordered owned weapons; drives weapon-wheel cycling. */
    TArray<FSlot> Slots;
    /** Weapon id -> index into Slots. */
    TMap<FString, int32> SlotIndex;
    FString Current;

    const FSlot* FindSlot(const FString& Id) const;
    FSlot* FindSlot(const FString& Id);
    int32 MagOf(const FString& Id) const;
    FString Cycle(int32 Step);
};
