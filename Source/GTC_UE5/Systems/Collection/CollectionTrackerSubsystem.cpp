// Copyright Epic Games, Inc. All Rights Reserved.

#include "CollectionTrackerSubsystem.h"

EGtcCollectionReward UCollectionTrackerSubsystem::MapReward(FCollectionTracker::EReward Reward)
{
    switch (Reward)
    {
    case FCollectionTracker::EReward::Bronze:   return EGtcCollectionReward::Bronze;
    case FCollectionTracker::EReward::Silver:   return EGtcCollectionReward::Silver;
    case FCollectionTracker::EReward::Gold:     return EGtcCollectionReward::Gold;
    case FCollectionTracker::EReward::Platinum: return EGtcCollectionReward::Platinum;
    default:                                    return EGtcCollectionReward::None;
    }
}

EGtcCollectionReward UCollectionTrackerSubsystem::GetReward() const
{
    return MapReward(Model.Reward());
}

void UCollectionTrackerSubsystem::DefineCategory(const FString& CategoryId, int32 Total)
{
    Model.DefineCategory(CategoryId, Total);
    LastReward = MapReward(Model.Reward());
}

bool UCollectionTrackerSubsystem::Collect(const FString& CategoryId, int32 ItemIndex)
{
    const bool bNew = Model.Collect(CategoryId, ItemIndex);
    if (bNew)
    {
        OnCollected.Broadcast(CategoryId, ItemIndex);
        OnCompletionChanged.Broadcast(static_cast<float>(Model.Completion()));

        const EGtcCollectionReward Reward = MapReward(Model.Reward());
        if (Reward != LastReward)
        {
            LastReward = Reward;
            OnRewardChanged.Broadcast(Reward);
        }
    }
    return bNew;
}
