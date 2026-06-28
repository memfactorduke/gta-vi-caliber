// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CollectionTracker.h"
#include "CollectionTrackerSubsystem.generated.h"

/** Blueprint mirror of FCollectionTracker::EReward. */
UENUM(BlueprintType)
enum class EGtcCollectionReward : uint8
{
    None,
    Bronze,
    Silver,
    Gold,
    Platinum
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGtcOnCollected, const FString&, CategoryId, int32, ItemIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGtcOnCompletionChanged, float, Completion);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGtcOnCollectionRewardChanged, EGtcCollectionReward, Reward);

/**
 * UCollectionTrackerSubsystem — the 100%-completion ledger (UE adapter over
 * FCollectionTracker).
 *
 * A WorldSubsystem: collectibles belong to the loaded world. DefineCategory() per
 * collectible KIND the map ships, Collect(category, index) as the player finds each
 * (idempotent — duplicates are ignored). Fires OnCollected on a NEW find and
 * OnRewardChanged as completion crosses 25/50/75/100%.
 */
UCLASS()
class GTC_UE5_API UCollectionTrackerSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "GTC|Collection")
    FGtcOnCollected OnCollected;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Collection")
    FGtcOnCompletionChanged OnCompletionChanged;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Collection")
    FGtcOnCollectionRewardChanged OnRewardChanged;

    /** Register (or reset) a collectible category with Total items. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Collection")
    void DefineCategory(const FString& CategoryId, int32 Total);

    /** Mark item ItemIndex of CategoryId found; returns true only on a NEW find. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Collection")
    bool Collect(const FString& CategoryId, int32 ItemIndex);

    UFUNCTION(BlueprintPure, Category = "GTC|Collection")
    int32 GetCategoryCount() const { return Model.CategoryCount(); }

    UFUNCTION(BlueprintPure, Category = "GTC|Collection")
    int32 FoundIn(const FString& CategoryId) const { return Model.FoundIn(CategoryId); }

    UFUNCTION(BlueprintPure, Category = "GTC|Collection")
    int32 TotalIn(const FString& CategoryId) const { return Model.TotalIn(CategoryId); }

    UFUNCTION(BlueprintPure, Category = "GTC|Collection")
    float FractionIn(const FString& CategoryId) const { return static_cast<float>(Model.FractionIn(CategoryId)); }

    UFUNCTION(BlueprintPure, Category = "GTC|Collection")
    bool IsCategoryComplete(const FString& CategoryId) const { return Model.IsCategoryComplete(CategoryId); }

    UFUNCTION(BlueprintPure, Category = "GTC|Collection")
    int32 GetTotalFound() const { return Model.TotalFound(); }

    UFUNCTION(BlueprintPure, Category = "GTC|Collection")
    int32 GetGrandTotal() const { return Model.GrandTotal(); }

    /** Overall completion, 0..1. */
    UFUNCTION(BlueprintPure, Category = "GTC|Collection")
    float GetCompletion() const { return static_cast<float>(Model.Completion()); }

    UFUNCTION(BlueprintPure, Category = "GTC|Collection")
    bool IsComplete() const { return Model.IsComplete(); }

    UFUNCTION(BlueprintPure, Category = "GTC|Collection")
    EGtcCollectionReward GetReward() const;

private:
    FCollectionTracker Model;
    EGtcCollectionReward LastReward = EGtcCollectionReward::None;

    static EGtcCollectionReward MapReward(FCollectionTracker::EReward Reward);
};
