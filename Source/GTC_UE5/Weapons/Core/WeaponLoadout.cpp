// Copyright Epic Games, Inc. All Rights Reserved.

#include "WeaponLoadout.h"

#include "Math/UnrealMathUtility.h"

const TArray<FString>& WeaponLoadout::Slots()
{
    static const TArray<FString> SlotList = {TEXT("optic"), TEXT("muzzle"), TEXT("magazine"), TEXT("grip")};
    return SlotList;
}

WeaponLoadout::WeaponLoadout(const TArray<FAttachment>& Attachments)
{
    const TArray<FAttachment>& Source = Attachments.Num() > 0 ? Attachments : DefaultAttachments();
    for (const FAttachment& Entry : Source)
    {
        Register(Entry);
    }
}

TArray<WeaponLoadout::FAttachment> WeaponLoadout::DefaultAttachments()
{
    TArray<FAttachment> Defaults;

    FAttachment RedDot;
    RedDot.Id = TEXT("red_dot");
    RedDot.Slot = TEXT("optic");
    RedDot.Mults.Add(TEXT("spread"), 0.85);
    Defaults.Add(RedDot);

    FAttachment Scope;
    Scope.Id = TEXT("scope");
    Scope.Slot = TEXT("optic");
    Scope.Mults.Add(TEXT("spread"), 0.6);
    Scope.Mults.Add(TEXT("range"), 1.25);
    Defaults.Add(Scope);

    FAttachment Suppressor;
    Suppressor.Id = TEXT("suppressor");
    Suppressor.Slot = TEXT("muzzle");
    Suppressor.Mults.Add(TEXT("range"), 0.9);
    Suppressor.bSuppresses = true;
    Defaults.Add(Suppressor);

    FAttachment Compensator;
    Compensator.Id = TEXT("compensator");
    Compensator.Slot = TEXT("muzzle");
    Compensator.Mults.Add(TEXT("recoil"), 0.7);
    Defaults.Add(Compensator);

    FAttachment ExtendedMag;
    ExtendedMag.Id = TEXT("extended_mag");
    ExtendedMag.Slot = TEXT("magazine");
    ExtendedMag.Adds.Add(TEXT("mag"), 12);
    Defaults.Add(ExtendedMag);

    FAttachment Foregrip;
    Foregrip.Id = TEXT("foregrip");
    Foregrip.Slot = TEXT("grip");
    Foregrip.Mults.Add(TEXT("recoil"), 0.8);
    Foregrip.Mults.Add(TEXT("spread"), 0.9);
    Defaults.Add(Foregrip);

    return Defaults;
}

int32 WeaponLoadout::AttachmentCount() const
{
    return Catalogue.Num();
}

bool WeaponLoadout::HasAttachment(const FString& Id) const
{
    return CatalogueIndex.Contains(Id);
}

FString WeaponLoadout::SlotOf(const FString& Id) const
{
    const FAttachment* Attachment = FindAttachment(Id);
    return Attachment != nullptr ? Attachment->Slot : FString();
}

bool WeaponLoadout::Equip(const FString& Id)
{
    const FAttachment* Attachment = FindAttachment(Id);
    if (Attachment == nullptr)
    {
        return false;
    }
    Equipped.Add(Attachment->Slot, Id);
    return true;
}

void WeaponLoadout::Unequip(const FString& Slot)
{
    Equipped.Remove(Slot);
}

FString WeaponLoadout::EquippedIn(const FString& Slot) const
{
    const FString* Id = Equipped.Find(Slot);
    return Id != nullptr ? *Id : FString();
}

bool WeaponLoadout::IsEquipped(const FString& Id) const
{
    const FAttachment* Attachment = FindAttachment(Id);
    if (Attachment == nullptr)
    {
        return false;
    }
    const FString* Current = Equipped.Find(Attachment->Slot);
    return Current != nullptr && *Current == Id;
}

int32 WeaponLoadout::EquippedCount() const
{
    return Equipped.Num();
}

double WeaponLoadout::MultFor(const FString& Stat) const
{
    double Factor = 1.0;
    for (const TPair<FString, FString>& Pair : Equipped)
    {
        const FAttachment* Attachment = FindAttachment(Pair.Value);
        if (Attachment == nullptr)
        {
            continue;
        }
        const double* Mult = Attachment->Mults.Find(Stat);
        if (Mult != nullptr)
        {
            Factor *= *Mult;
        }
    }
    return Factor;
}

int32 WeaponLoadout::AddFor(const FString& Stat) const
{
    int32 Total = 0;
    for (const TPair<FString, FString>& Pair : Equipped)
    {
        const FAttachment* Attachment = FindAttachment(Pair.Value);
        if (Attachment == nullptr)
        {
            continue;
        }
        const int32* Add = Attachment->Adds.Find(Stat);
        if (Add != nullptr)
        {
            Total += *Add;
        }
    }
    return Total;
}

double WeaponLoadout::ApplyMult(double BaseValue, const FString& Stat) const
{
    return BaseValue * MultFor(Stat);
}

int32 WeaponLoadout::MagSize(int32 BaseSize) const
{
    return FMath::Max(0, BaseSize + AddFor(TEXT("mag")));
}

bool WeaponLoadout::IsSuppressed() const
{
    for (const TPair<FString, FString>& Pair : Equipped)
    {
        const FAttachment* Attachment = FindAttachment(Pair.Value);
        if (Attachment != nullptr && Attachment->bSuppresses)
        {
            return true;
        }
    }
    return false;
}

void WeaponLoadout::Clear()
{
    Equipped.Empty();
}

const WeaponLoadout::FAttachment* WeaponLoadout::FindAttachment(const FString& Id) const
{
    const int32* Index = CatalogueIndex.Find(Id);
    return Index != nullptr ? &Catalogue[*Index] : nullptr;
}

void WeaponLoadout::Register(const FAttachment& Entry)
{
    // Malformed entries are silently dropped: missing id/slot, unknown slot, dup id.
    if (Entry.Id.IsEmpty() || Entry.Slot.IsEmpty())
    {
        return;
    }
    if (CatalogueIndex.Contains(Entry.Id) || !Slots().Contains(Entry.Slot))
    {
        return;
    }
    CatalogueIndex.Add(Entry.Id, Catalogue.Add(Entry));
}
