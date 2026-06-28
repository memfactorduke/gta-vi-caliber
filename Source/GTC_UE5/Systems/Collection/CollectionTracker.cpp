// Copyright Epic Games, Inc. All Rights Reserved.

#include "CollectionTracker.h"

const FCollectionTracker::FCategory* FCollectionTracker::FindCategory(const FString& CategoryId) const
{
    for (const FCategory& Category : Categories)
    {
        if (Category.Id == CategoryId)
        {
            return &Category;
        }
    }
    return nullptr;
}

FCollectionTracker::FCategory* FCollectionTracker::FindCategory(const FString& CategoryId)
{
    // Reuse the const lookup, then cast away const on our own storage.
    return const_cast<FCategory*>(static_cast<const FCollectionTracker*>(this)->FindCategory(CategoryId));
}

void FCollectionTracker::DefineCategory(const FString& CategoryId, int32 Total)
{
    if (FCategory* Existing = FindCategory(CategoryId))
    {
        Existing->Total = Total;
        Existing->Found.Reset(); // re-defining resets progress
        return;
    }

    FCategory Category;
    Category.Id = CategoryId;
    Category.Total = Total;
    Categories.Add(MoveTemp(Category));
}

bool FCollectionTracker::Collect(const FString& CategoryId, int32 ItemIndex)
{
    FCategory* Category = FindCategory(CategoryId);
    if (Category == nullptr)
    {
        return false; // unknown category
    }
    if (ItemIndex < 0 || ItemIndex >= Category->Total)
    {
        return false; // out of range (also rejects everything in a non-positive-total category)
    }
    if (Category->Found.Contains(ItemIndex))
    {
        return false; // already found — idempotent
    }

    Category->Found.Add(ItemIndex);
    return true;
}

int32 FCollectionTracker::FoundIn(const FString& CategoryId) const
{
    const FCategory* Category = FindCategory(CategoryId);
    return Category ? Category->Found.Num() : 0;
}

int32 FCollectionTracker::TotalIn(const FString& CategoryId) const
{
    const FCategory* Category = FindCategory(CategoryId);
    return Category ? FMath::Max(0, Category->Total) : 0;
}

double FCollectionTracker::FractionIn(const FString& CategoryId) const
{
    const FCategory* Category = FindCategory(CategoryId);
    if (Category == nullptr)
    {
        return 0.0;
    }
    if (Category->Total <= 0)
    {
        return 1.0; // nothing to collect -> trivially complete
    }
    return static_cast<double>(Category->Found.Num()) / static_cast<double>(Category->Total);
}

bool FCollectionTracker::IsCategoryComplete(const FString& CategoryId) const
{
    const FCategory* Category = FindCategory(CategoryId);
    if (Category == nullptr)
    {
        return false;
    }
    return Category->Total <= 0 || Category->Found.Num() >= Category->Total;
}

int32 FCollectionTracker::TotalFound() const
{
    int32 Sum = 0;
    for (const FCategory& Category : Categories)
    {
        Sum += Category.Found.Num();
    }
    return Sum;
}

int32 FCollectionTracker::GrandTotal() const
{
    int32 Sum = 0;
    for (const FCategory& Category : Categories)
    {
        Sum += FMath::Max(0, Category.Total);
    }
    return Sum;
}

double FCollectionTracker::Completion() const
{
    const int32 Total = GrandTotal();
    if (Total <= 0)
    {
        return 1.0; // nothing in the world to collect -> 100% by default
    }
    return static_cast<double>(TotalFound()) / static_cast<double>(Total);
}

bool FCollectionTracker::IsComplete() const
{
    return TotalFound() >= GrandTotal();
}

FCollectionTracker::EReward FCollectionTracker::Reward() const
{
    const double C = Completion();
    if (C >= 1.0)
    {
        return EReward::Platinum;
    }
    if (C >= 0.75)
    {
        return EReward::Gold;
    }
    if (C >= 0.5)
    {
        return EReward::Silver;
    }
    if (C >= 0.25)
    {
        return EReward::Bronze;
    }
    return EReward::None;
}
