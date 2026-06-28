// Copyright Epic Games, Inc. All Rights Reserved.

#include "LeadScheduleComponent.h"

ULeadScheduleComponent::ULeadScheduleComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void ULeadScheduleComponent::SetRoutine(const TArray<FGtcLeadBlock>& InBlocks)
{
    TArray<FLeadSchedule::FBlock> Blocks;
    Blocks.Reserve(InBlocks.Num());
    for (const FGtcLeadBlock& In : InBlocks)
    {
        FLeadSchedule::FBlock Block;
        Block.StartHour = static_cast<double>(In.StartHour);
        Block.Place = In.Place;
        Block.Activity = In.Activity;
        Blocks.Add(MoveTemp(Block));
    }
    Model.SetRoutine(MoveTemp(Blocks));
}
