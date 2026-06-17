// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure weapon-attachment / loadout model — one attachment per slot (optic, muzzle,
 * magazine, grip) that tunes a weapon's stats: a scope tightens spread and extends
 * range, a suppressor quiets the shot and trims range, an extended mag adds rounds,
 * a foregrip tames recoil. Direct port of the Godot `WeaponLoadout` (RefCounted) at
 * `game/scripts/weapons/weapon_loadout.gd`.
 *
 * No nodes, no scene access: a weapon controller owns one, asks for the combined
 * multipliers/bonuses, and applies them to its own ballistics — so the modifier
 * algebra stays unit-tested headless (Tests/WeaponLoadoutTest.cpp, prefix
 * GTC.Weapons.Core.WeaponLoadout). The aggregated modifiers feed FWeaponBallistics.
 *
 * Each attachment carries per-stat multipliers (combine by product), per-stat
 * integer bonuses (combine by sum), and a suppresses flag. Malformed catalogue
 * entries (missing id/slot, unknown slot, duplicate id) are dropped.
 *
 * Parity note: the catalogue preserves insertion order (the GarageStorage pattern),
 * matching Godot's Dictionary iteration; equipped multipliers/bonuses combine
 * commutatively so equip iteration order is not observable.
 */
class GTC_UE5_API WeaponLoadout
{
public:
    /** The four attachment slots; one attachment may occupy each. */
    static const TArray<FString>& Slots();

    /**
     * One catalogue attachment definition: which slot it occupies, its per-stat
     * multipliers/bonuses, and whether it suppresses the shot.
     */
    struct FAttachment
    {
        FString Id;
        FString Slot;
        TMap<FString, double> Mults;
        TMap<FString, int32> Adds;
        bool bSuppresses = false;
    };

    /**
     * Construct from a catalogue. An empty list loads the built-in
     * DefaultAttachments(); malformed entries are silently dropped.
     */
    explicit WeaponLoadout(const TArray<FAttachment>& Attachments = TArray<FAttachment>());

    /** Built-in attachment catalogue spanning every slot. */
    static TArray<FAttachment> DefaultAttachments();

    int32 AttachmentCount() const;

    /** True if the attachment id exists in the catalogue. */
    bool HasAttachment(const FString& Id) const;

    /** Slot an attachment occupies, or "" if the id is unknown. */
    FString SlotOf(const FString& Id) const;

    /**
     * Equip an attachment, replacing whatever was in its slot. Returns false for an
     * unknown id.
     */
    bool Equip(const FString& Id);

    /** Clear a slot. No-op for an unknown/empty slot. */
    void Unequip(const FString& Slot);

    /** The id equipped in a slot, or "" if the slot is empty. */
    FString EquippedIn(const FString& Slot) const;

    /** True if a specific attachment is currently equipped. */
    bool IsEquipped(const FString& Id) const;

    int32 EquippedCount() const;

    /**
     * Combined multiplier for a stat across all equipped attachments (product; 1.0
     * when none affect it). Apply to spread/range/recoil/etc.
     */
    double MultFor(const FString& Stat) const;

    /**
     * Combined integer bonus for a stat across all equipped attachments (sum; 0 when
     * none affect it). Apply to magazine size etc.
     */
    int32 AddFor(const FString& Stat) const;

    /** Convenience: a base stat scaled by the combined multiplier. */
    double ApplyMult(double BaseValue, const FString& Stat) const;

    /** Magazine size after additive bonuses, floored at 0. */
    int32 MagSize(int32 BaseSize) const;

    /** True if any equipped attachment suppresses the shot. */
    bool IsSuppressed() const;

    /** Strip every attachment. */
    void Clear();

private:
    /** Insertion-ordered catalogue. */
    TArray<FAttachment> Catalogue;
    /** Attachment id -> index into Catalogue. */
    TMap<FString, int32> CatalogueIndex;
    /** Slot -> equipped attachment id. */
    TMap<FString, FString> Equipped;

    const FAttachment* FindAttachment(const FString& Id) const;
    void Register(const FAttachment& Entry);
};
