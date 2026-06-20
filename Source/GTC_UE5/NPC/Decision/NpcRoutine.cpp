// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcRoutine.h"

const FNpcRoutine* FNpcRoutineLibrary::Find(const TArray<FNpcRoutine>& Routines, const FString& Id)
{
    for (const FNpcRoutine& R : Routines)
    {
        if (R.Id.Equals(Id, ESearchCase::IgnoreCase))
        {
            return &R;
        }
    }
    return nullptr;
}

bool FNpcRoutineLibrary::CoversFullDay(const FNpcRoutine& Routine)
{
    if (Routine.Blocks.Num() == 0)
    {
        return false;
    }
    // Sample the clock on a fine grid; every sample must land in some block. A grid
    // of 0.25h (15 min) catches any gap an authored routine would realistically have,
    // and matches the resolution the schedule is authored at.
    for (double H = 0.0; H < 24.0; H += 0.25)
    {
        bool bCovered = false;
        for (const FNpcScheduleBlock& B : Routine.Blocks)
        {
            if (FNpcSchedule::BlockCovers(B, H))
            {
                bCovered = true;
                break;
            }
        }
        if (!bCovered)
        {
            return false;
        }
    }
    return true;
}

int32 FNpcRoutineLibrary::FirstOverlap(const FNpcRoutine& Routine)
{
    // An hour covered by two blocks is ambiguous: the earlier block shadows the later
    // (first-covering-block-wins), so the later one is dead. Report the first such
    // later-block index for the editor to flag.
    for (int32 J = 1; J < Routine.Blocks.Num(); ++J)
    {
        // Does block J overlap any earlier block I? Sample J's window.
        const FNpcScheduleBlock& BJ = Routine.Blocks[J];
        // Sample the midpoint and the endpoints of J's window against earlier blocks.
        const double Mid = FNpcSchedule::WrapHour(BJ.Start + 0.01);
        for (int32 I = 0; I < J; ++I)
        {
            if (FNpcSchedule::BlockCovers(Routine.Blocks[I], Mid))
            {
                return J;
            }
        }
    }
    return INDEX_NONE;
}

FString FNpcRoutineLibrary::PickIdForSeed(const TArray<FNpcRoutine>& Pool, int32 Seed)
{
    if (Pool.Num() == 0)
    {
        return FString();
    }
    // Godot-style positive modulo so the same seed always maps to the same routine.
    const int32 Idx = ((Seed % Pool.Num()) + Pool.Num()) % Pool.Num();
    return Pool[Idx].Id;
}
