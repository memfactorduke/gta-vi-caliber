// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCStreetRaceComponent.h"

#include "GameFramework/Actor.h"
#include "Math/UnrealMathUtility.h"

// EGTCRacePhase mirrors FRaceSession::EPhase; the tick bridges them with a static_cast, so pin the
// values — reordering either enum breaks the build instead of silently mis-reporting the phase.
static_assert((uint8)FRaceSession::EPhase::Ready == (uint8)EGTCRacePhase::Ready, "race phase mirror drift");
static_assert((uint8)FRaceSession::EPhase::Racing == (uint8)EGTCRacePhase::Racing, "race phase mirror drift");
static_assert((uint8)FRaceSession::EPhase::Finished == (uint8)EGTCRacePhase::Finished, "race phase mirror drift");

UGTCStreetRaceComponent::UGTCStreetRaceComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

FVector UGTCStreetRaceComponent::RacerLocation() const
{
    if (const AActor* Tracked = Racer)
    {
        return Tracked->GetActorLocation();
    }
    if (const AActor* Owner = GetOwner())
    {
        return Owner->GetActorLocation();
    }
    return FVector::ZeroVector;
}

void UGTCStreetRaceComponent::MaybeAnnounceGreen()
{
    // Fire the green flag exactly once — on the tick the countdown ends, or right away in StartRace
    // for a 0s countdown (the session is already Racing before the first tick).
    if (!bAnnouncedGreen && Session.IsSet() && Session->IsRacing())
    {
        bAnnouncedGreen = true;
        OnRaceGreen.Broadcast();
    }
}

void UGTCStreetRaceComponent::StartRace()
{
    if (Checkpoints.Num() == 0)
    {
        return; // no course to race
    }

    // Snapshot the config once so live edits to the (BlueprintReadWrite) UPROPERTYs can't desync or
    // over-index the running race — the FRaceSession is built from exactly these values.
    CourseWaypoints = Checkpoints;
    RivalPaces.Reset();
    RivalPaces.Reserve(RivalPaceSeconds.Num());
    for (float P : RivalPaceSeconds)
    {
        RivalPaces.Add((double)P);
    }
    const int32 LapsSnap = FMath::Max(1, Laps);
    CourseGateCount = CourseWaypoints.Num() * LapsSnap;

    // Map the authored UE checkpoints into the race cores' XZ ground-plane convention.
    TArray<FVector> RaceCheckpoints;
    RaceCheckpoints.Reserve(CourseWaypoints.Num());
    for (const FVector& C : CourseWaypoints)
    {
        RaceCheckpoints.Add(FRaceProgressResolver::ToRaceSpace(C));
    }

    Session.Emplace(1 + RivalPaces.Num(), RaceCheckpoints, LapsSnap,
        FMath::Max(0.0f, CountdownSeconds), FMath::Max(0, BaseReward));

    RivalProgress.Init(0.0, RivalPaces.Num());
    bRacing = true;
    bReportedFinish = false;
    bAnnouncedGreen = false;
    Publish();
    MaybeAnnounceGreen();
}

void UGTCStreetRaceComponent::AbortRace()
{
    bRacing = false;
    bAnnouncedGreen = false;
    bReportedFinish = false;
    Session.Reset();
}

void UGTCStreetRaceComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bRacing || !Session.IsSet())
    {
        return; // no active race
    }
    if (DeltaTime <= 0.0f)
    {
        return; // a frozen frame passes no time
    }

    FRaceSession& S = Session.GetValue();
    S.Tick((double)DeltaTime);
    MaybeAnnounceGreen();

    if (S.IsRacing())
    {
        const int32 Total = CourseGateCount;
        const double PlayerProg = S.ProgressOf(0);

        // Drive each rival by its pace clock: advance its gates until its cleared count catches
        // where the pacer says it should be.
        for (int32 r = 0; r < RivalPaces.Num(); ++r)
        {
            const int32 Idx = r + 1;
            const FRaceProgressResolver::FRivalTick RT = FRaceProgressResolver::RivalTick(
                RivalProgress[r], S.Participant(Idx).Elapsed(), RivalPaces[r],
                PlayerProg, (double)RubberBandStrength, (double)RubberBandMax, Total);
            RivalProgress[r] = RT.Progress;

            int32 Guard = 0;
            while (Guard++ <= Total
                && !S.Participant(Idx).IsFinished()
                && (Total - S.Participant(Idx).CheckpointsRemaining()) < RT.TargetCleared)
            {
                if (!S.Reached(Idx, S.Participant(Idx).CurrentCheckpoint(), (double)CheckpointRadius))
                {
                    break;
                }
            }
        }

        // The player crosses gates by driving through them. Credit every gate it is within radius
        // of this tick (so a fast car / tight gates don't tunnel), but don't announce the wrap-to-0
        // that clearing the final gate performs at the finish line.
        const FVector PlayerRacePos = FRaceProgressResolver::ToRaceSpace(RacerLocation());
        int32 Crossings = 0;
        while (Crossings++ <= Total && S.Reached(0, PlayerRacePos, (double)CheckpointRadius))
        {
            if (!S.Participant(0).IsFinished())
            {
                OnCheckpoint.Broadcast(S.Participant(0).CheckpointIndex());
            }
        }
    }

    Publish();

    // The player's race is over the moment THEY finish — don't wait for the slowest rival's pace
    // clock to run out. Their placement is already final: a still-running rival can only settle
    // behind a finisher.
    if (!bReportedFinish && S.Participant(0).IsFinished())
    {
        bReportedFinish = true;
        bRacing = false;
        const int32 PlayerPlace = S.PlacementOf(0);
        OnRaceFinished.Broadcast(PlayerPlace, StreetRace::Reward(PlayerPlace, FMath::Max(0, BaseReward)));
    }
}

void UGTCStreetRaceComponent::Publish()
{
    if (!Session.IsSet())
    {
        return;
    }
    const FRaceSession& S = Session.GetValue();
    RacePhase = static_cast<EGTCRacePhase>((uint8)S.Phase());
    FieldSize = S.NumParticipants();
    PlayerPlacement = S.PlacementOf(0);
    PlayerProgress = (float)S.ProgressOf(0);
    CountdownRemaining = (float)S.CountdownRemaining();

    const StreetRace& P = S.Participant(0);
    CurrentLap = P.CurrentLap();
    TotalLaps = P.TotalLaps();
    ElapsedSeconds = (float)P.Elapsed();
    BestLapSeconds = (float)P.BestLap();

    // HUD waypoint: the player's current gate, from the snapshot of the authored UE ring (the
    // session holds the race-space copies), by within-lap index.
    const int32 CpIdx = P.CheckpointIndex();
    NextCheckpointLocation = (!S.IsFinished() && CourseWaypoints.IsValidIndex(CpIdx))
        ? CourseWaypoints[CpIdx]
        : FVector::ZeroVector;
}
