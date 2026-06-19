// Copyright (c) 2026 GTC contributors

#include "MissionReward.h"

void UMissionReward::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UMissionReward::Deinitialize()
{
    UnbindController();
    Super::Deinitialize();
}

void UMissionReward::BindController(UMissionController* Controller)
{
    // Re-bind cleanly so we never double-subscribe (the reference connects once in _ready).
    UnbindController();
    if (Controller == nullptr)
    {
        return;
    }
    BoundController = Controller;
    ObjectiveHandle = Controller->OnObjectiveCompleted.AddUObject(this, &UMissionReward::HandleObjectiveCompleted);
    MissionHandle = Controller->OnMissionCompleted.AddUObject(this, &UMissionReward::HandleMissionCompleted);
}

void UMissionReward::UnbindController()
{
    if (UMissionController* Controller = BoundController.Get())
    {
        if (ObjectiveHandle.IsValid())
        {
            Controller->OnObjectiveCompleted.Remove(ObjectiveHandle);
        }
        if (MissionHandle.IsValid())
        {
            Controller->OnMissionCompleted.Remove(MissionHandle);
        }
    }
    ObjectiveHandle.Reset();
    MissionHandle.Reset();
    BoundController.Reset();
}

void UMissionReward::HandleObjectiveCompleted(const FString& Id)
{
    Pay(ObjectiveReward);
}

void UMissionReward::HandleMissionCompleted()
{
    Pay(MissionBonus);
}

void UMissionReward::Pay(int32 Amount)
{
    // DEFERRED-OWNERSHIP: no PlayerStats write-through. Negative/zero rewards are
    // ignored (a reward only ever adds money). Credit the owned wallet snapshot and
    // accumulate the pending payout intent the Wave 3 UPlayerStatsComponent adapter
    // drains (per docs/W3_WIRING_NOTES.md, f0c0c36 — Stats is canonical money owner).
    if (Amount <= 0)
    {
        return;
    }
    LastReward = Amount;
    Wallet += Amount;
    PendingPayout += Amount;
}
