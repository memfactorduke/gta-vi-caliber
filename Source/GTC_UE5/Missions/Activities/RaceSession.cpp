// Copyright Epic Games, Inc. All Rights Reserved.

#include "RaceSession.h"

FRaceSession::FRaceSession(int32 ParticipantCount, const TArray<FVector>& Checkpoints, int32 Laps,
    double CountdownSeconds, int32 InBaseReward)
    : InitialCountdown(FMath::Max(CountdownSeconds, 0.0))
    , BaseReward(InBaseReward)
{
    const int32 Count = FMath::Max(ParticipantCount, 0);
    Racers.Reserve(Count);
    Retired.Reserve(Count);
    for (int32 i = 0; i < Count; ++i)
    {
        Racers.Emplace(Checkpoints, Laps);
        Retired.Add(false);
    }
    Reset();
}

void FRaceSession::StartCountdown(double Seconds)
{
    Countdown = FMath::Max(Seconds, 0.0);
    // Back to the grid, then resolve where a zero/degenerate field actually lands.
    RacePhase = EPhase::Ready;
    if (AllSettled())
    {
        // No one can run (empty field / degenerate track): there is nothing to count down to.
        Countdown = 0.0;
        RacePhase = EPhase::Finished;
    }
    else if (Countdown <= 0.0)
    {
        RacePhase = EPhase::Racing;
    }
}

void FRaceSession::Tick(double Delta)
{
    if (RacePhase == EPhase::Finished || Delta <= 0.0)
    {
        return;
    }

    if (RacePhase == EPhase::Ready)
    {
        if (Delta < Countdown)
        {
            // Still on the grid.
            Countdown -= Delta;
            return;
        }
        // Lights go green this frame: spend the rest of Delta as the first racing slice so a
        // coarse tick doesn't lose race time at the start.
        Delta -= Countdown;
        Countdown = 0.0;
        RacePhase = EPhase::Racing;
        if (Delta <= 0.0)
        {
            return;
        }
    }

    // Racing: clock every participant that is still out on track.
    for (int32 i = 0; i < Racers.Num(); ++i)
    {
        if (IsRunning(i))
        {
            Racers[i].Tick(Delta);
        }
    }

    if (AllSettled())
    {
        RacePhase = EPhase::Finished;
    }
}

bool FRaceSession::Reached(int32 Index, const FVector& Pos, double Radius)
{
    if (RacePhase != EPhase::Racing || !IsRunning(Index))
    {
        return false;
    }
    const bool bHit = Racers[Index].Reached(Pos, Radius);
    if (bHit && AllSettled())
    {
        RacePhase = EPhase::Finished;
    }
    return bHit;
}

bool FRaceSession::Retire(int32 Index)
{
    if (RacePhase == EPhase::Finished || !IsRunning(Index))
    {
        return false;
    }
    Retired[Index] = true;
    if (AllSettled())
    {
        RacePhase = EPhase::Finished;
    }
    return true;
}

double FRaceSession::ProgressOf(int32 Index) const
{
    if (!Racers.IsValidIndex(Index))
    {
        return 0.0;
    }
    return Racers[Index].Progress();
}

int32 FRaceSession::PlacementOf(int32 Index) const
{
    if (!Racers.IsValidIndex(Index))
    {
        return 0;
    }
    // Count how many participants finish ahead of this one in the total result order
    // (RanksAhead, with the same lower-index tie-break Results() uses), plus one — so a
    // live PlacementOf always agrees with the Results() tally.
    int32 Place = 1;
    for (int32 i = 0; i < Racers.Num(); ++i)
    {
        if (i != Index && OutranksForSort(i, Index))
        {
            Place += 1;
        }
    }
    return Place;
}

TArray<FRaceSession::FResult> FRaceSession::Results() const
{
    // Rank participant indices best-first: RanksAhead decides strict order, and an exact
    // tie keeps the lower index ahead (stable), so the player (0) never loses a dead heat.
    TArray<int32> Order;
    Order.Reserve(Racers.Num());
    for (int32 i = 0; i < Racers.Num(); ++i)
    {
        Order.Add(i);
    }
    Order.Sort([this](int32 A, int32 B)
    {
        return OutranksForSort(A, B);
    });

    TArray<FResult> Out;
    Out.Reserve(Order.Num());
    for (int32 Rank = 0; Rank < Order.Num(); ++Rank)
    {
        const int32 Idx = Order[Rank];
        const StreetRace& R = Racers[Idx];
        const bool bDidFinish = R.IsFinished() && !Retired[Idx];

        FResult Result;
        Result.ParticipantIndex = Idx;
        Result.Placement = Rank + 1;
        Result.bFinished = bDidFinish;
        Result.Progress = R.Progress();
        Result.Elapsed = R.Elapsed();
        Result.Reward = bDidFinish ? StreetRace::Reward(Result.Placement, BaseReward) : 0;
        Out.Add(Result);
    }
    return Out;
}

void FRaceSession::Reset()
{
    for (int32 i = 0; i < Racers.Num(); ++i)
    {
        Racers[i].Reset();
        Retired[i] = false;
    }
    // Restore the grid with the original countdown, then resolve a degenerate/zero field.
    StartCountdown(InitialCountdown);
}

// --- helpers ---------------------------------------------------------------

bool FRaceSession::IsRunning(int32 Index) const
{
    return Racers.IsValidIndex(Index) && !Retired[Index] && !Racers[Index].IsFinished();
}

bool FRaceSession::AllSettled() const
{
    for (int32 i = 0; i < Racers.Num(); ++i)
    {
        if (IsRunning(i))
        {
            return false;
        }
    }
    return true; // also true for an empty field
}

bool FRaceSession::OutranksForSort(int32 IndexA, int32 IndexB) const
{
    // Total strict order for ranking: strictly-ahead first, else the exact-tie break by the
    // lower participant index (so the player at index 0 takes a dead heat). Self never
    // outranks self. Shared by Results() (the sort) and PlacementOf() (the count) so the two
    // can never disagree.
    if (RanksAhead(IndexA, IndexB))
    {
        return true;
    }
    if (RanksAhead(IndexB, IndexA))
    {
        return false;
    }
    return IndexA < IndexB;
}

bool FRaceSession::RanksAhead(int32 IndexA, int32 IndexB) const
{
    const StreetRace& A = Racers[IndexA];
    const StreetRace& B = Racers[IndexB];

    // A retired (DNF) racer is below everyone except, for ordering, another retired racer
    // (where neither ranks ahead and the index tie-break applies).
    const bool bRetiredA = Retired[IndexA];
    const bool bRetiredB = Retired[IndexB];
    if (bRetiredA != bRetiredB)
    {
        return bRetiredB; // A ranks ahead iff B is the retired one
    }
    if (bRetiredA && bRetiredB)
    {
        return false; // both DNF: leave the tie to the index break
    }

    // A finisher always ranks ahead of a non-finisher.
    const bool bFinA = A.IsFinished();
    const bool bFinB = B.IsFinished();
    if (bFinA != bFinB)
    {
        return bFinA;
    }

    if (bFinA && bFinB)
    {
        // Both finished: the quicker race clock wins. Equal time -> not strictly ahead.
        return A.Elapsed() < B.Elapsed() - Epsilon;
    }

    // Neither finished: further along the track wins. Equal progress -> not strictly ahead.
    return A.Progress() > B.Progress() + Epsilon;
}
