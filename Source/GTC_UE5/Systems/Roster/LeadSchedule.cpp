// Copyright Epic Games, Inc. All Rights Reserved.

#include "LeadSchedule.h"

double FLeadSchedule::WrapHours(double Hours)
{
    double H = FMath::Fmod(Hours, HoursPerDay);
    if (H < 0.0)
    {
        H += HoursPerDay; // FMath::Fmod keeps the dividend's sign; fold negatives positive
    }
    return H;
}

void FLeadSchedule::SetRoutine(TArray<FBlock> InBlocks)
{
    Blocks.Reset();
    for (FBlock Block : InBlocks)
    {
        Block.StartHour = WrapHours(Block.StartHour);

        // Keep the first block given for any duplicate start hour.
        bool bDuplicate = false;
        for (const FBlock& Existing : Blocks)
        {
            if (FMath::IsNearlyEqual(Existing.StartHour, Block.StartHour))
            {
                bDuplicate = true;
                break;
            }
        }
        if (!bDuplicate)
        {
            Blocks.Add(Block);
        }
    }

    Blocks.Sort([](const FBlock& A, const FBlock& B) { return A.StartHour < B.StartHour; });
}

int32 FLeadSchedule::BlockAt(double Hours) const
{
    if (Blocks.Num() == 0)
    {
        return INDEX_NONE;
    }

    const double H = WrapHours(Hours);

    // Blocks are sorted ascending: the active block is the last one that has already started.
    int32 Active = INDEX_NONE;
    for (int32 Index = 0; Index < Blocks.Num(); ++Index)
    {
        if (Blocks[Index].StartHour <= H)
        {
            Active = Index;
        }
        else
        {
            break; // sorted, so nothing later has started yet
        }
    }

    // Before the first block's start, the day is still in the previous day's last block.
    if (Active == INDEX_NONE)
    {
        Active = Blocks.Num() - 1;
    }
    return Active;
}

FVector FLeadSchedule::PlaceAt(double Hours) const
{
    const int32 Index = BlockAt(Hours);
    return Index == INDEX_NONE ? FVector::ZeroVector : Blocks[Index].Place;
}

FString FLeadSchedule::ActivityAt(double Hours) const
{
    const int32 Index = BlockAt(Hours);
    return Index == INDEX_NONE ? FString() : Blocks[Index].Activity;
}

double FLeadSchedule::TimeUntilNextBlock(double Hours) const
{
    if (Blocks.Num() == 0)
    {
        return 0.0;
    }

    const double H = WrapHours(Hours);
    const int32 Current = BlockAt(H);
    const int32 Next = (Current + 1) % Blocks.Num();

    double Delta = Blocks[Next].StartHour - H;
    if (Delta <= 0.0)
    {
        Delta += HoursPerDay; // the next boundary is tomorrow
    }
    return Delta;
}
