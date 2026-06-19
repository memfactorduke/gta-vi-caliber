// Copyright (c) 2026 GTC contributors

#include "WeaponInventory.h"

#include "Math/UnrealMathUtility.h"

const FString WeaponInventory::UnarmedId = TEXT("fists");

WeaponInventory::WeaponInventory()
{
    // Fists are unarmed: a zero-capacity slot that can never fire or hold ammo.
    FSlot Fists;
    Fists.Id = UnarmedId;
    Fists.MagSize = 0;
    Fists.Mag = 0;
    Fists.Reserve = 0;
    SlotIndex.Add(UnarmedId, Slots.Add(Fists));
    Current = UnarmedId;
}

void WeaponInventory::AddWeapon(const FString& Id, int32 MagSize, int32 Ammo)
{
    if (Id.IsEmpty())
    {
        return;
    }
    const int32 Size = FMath::Max(MagSize, 0);
    const int32 Spare = FMath::Max(Ammo, 0);
    if (SlotIndex.Contains(Id))
    {
        AddAmmo(Id, Spare);
        return;
    }
    FSlot Slot;
    Slot.Id = Id;
    Slot.MagSize = Size;
    Slot.Mag = Size;
    Slot.Reserve = Spare;
    SlotIndex.Add(Id, Slots.Add(Slot));
}

void WeaponInventory::AddAmmo(const FString& Id, int32 Amount)
{
    if (Amount <= 0)
    {
        return;
    }
    FSlot* Slot = FindSlot(Id);
    if (Slot == nullptr)
    {
        return;
    }
    Slot->Reserve += Amount;
}

bool WeaponInventory::HasWeapon(const FString& Id) const
{
    return SlotIndex.Contains(Id);
}

int32 WeaponInventory::WeaponCount() const
{
    return Slots.Num();
}

TArray<FString> WeaponInventory::OwnedIds() const
{
    TArray<FString> Ids;
    Ids.Reserve(Slots.Num());
    for (const FSlot& Slot : Slots)
    {
        Ids.Add(Slot.Id);
    }
    return Ids;
}

bool WeaponInventory::Equip(const FString& Id)
{
    if (!SlotIndex.Contains(Id))
    {
        return false;
    }
    Current = Id;
    return true;
}

FString WeaponInventory::CurrentId() const
{
    return Current;
}

FString WeaponInventory::NextWeapon()
{
    return Cycle(1);
}

FString WeaponInventory::PreviousWeapon()
{
    return Cycle(-1);
}

bool WeaponInventory::CanFire() const
{
    return MagOf(Current) > 0;
}

bool WeaponInventory::Fire()
{
    if (!CanFire())
    {
        return false;
    }
    FindSlot(Current)->Mag -= 1;
    return true;
}

int32 WeaponInventory::AmmoInMag() const
{
    return MagOf(Current);
}

int32 WeaponInventory::ReserveAmmo() const
{
    const FSlot* Slot = FindSlot(Current);
    return Slot != nullptr ? Slot->Reserve : 0;
}

int32 WeaponInventory::Reload()
{
    FSlot* Slot = FindSlot(Current);
    if (Slot == nullptr)
    {
        return 0;
    }
    const int32 Needed = Slot->MagSize - Slot->Mag;
    if (Needed <= 0)
    {
        return 0;
    }
    const int32 Loaded = FMath::Min(Needed, Slot->Reserve);
    Slot->Mag += Loaded;
    Slot->Reserve -= Loaded;
    return Loaded;
}

const WeaponInventory::FSlot* WeaponInventory::FindSlot(const FString& Id) const
{
    const int32* Index = SlotIndex.Find(Id);
    return Index != nullptr ? &Slots[*Index] : nullptr;
}

WeaponInventory::FSlot* WeaponInventory::FindSlot(const FString& Id)
{
    const int32* Index = SlotIndex.Find(Id);
    return Index != nullptr ? &Slots[*Index] : nullptr;
}

int32 WeaponInventory::MagOf(const FString& Id) const
{
    const FSlot* Slot = FindSlot(Id);
    return Slot != nullptr ? Slot->Mag : 0;
}

FString WeaponInventory::Cycle(int32 Step)
{
    if (Slots.Num() == 0)
    {
        return Current;
    }
    int32 Index = INDEX_NONE;
    const int32* Found = SlotIndex.Find(Current);
    if (Found != nullptr)
    {
        Index = *Found;
    }
    if (Index == INDEX_NONE)
    {
        Index = 0;
    }
    const int32 Count = Slots.Num();
    const int32 NextIndex = ((Index + Step) % Count + Count) % Count;
    Current = Slots[NextIndex].Id;
    return Current;
}
