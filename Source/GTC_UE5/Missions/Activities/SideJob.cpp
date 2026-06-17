// Copyright (c) 2026 GTC contributors

#include "SideJob.h"

SideJob::FJob SideJob::MakeJob(int32 Kind, const FVector& Pickup, const FVector& Dropoff, int32 BaseReward)
{
    FJob Job;
    Job.Kind = Kind;
    Job.Pickup = Pickup;
    Job.Dropoff = Dropoff;
    Job.BaseReward = FMath::Max(BaseReward, 0);
    return Job;
}

FString SideJob::KindName(int32 Kind)
{
    switch (static_cast<EKind>(Kind))
    {
    case EKind::Taxi:
        return TEXT("taxi");
    case EKind::Delivery:
        return TEXT("delivery");
    case EKind::Vigilante:
        return TEXT("vigilante");
    case EKind::Towing:
        return TEXT("towing");
    default:
        return FString();
    }
}

int32 SideJob::Fare(double Distance, int32 BaseReward, double PerMeter)
{
    const int32 Base = FMath::Max(BaseReward, 0);
    const double Dist = FMath::Max(Distance, 0.0);
    const double Rate = FMath::Max(PerMeter, 0.0);
    return Base + FMath::RoundToInt(Dist * Rate);
}

int32 SideJob::VigilanteReward(int32 Targets, int32 BaseReward, int32 PerTarget)
{
    const int32 Base = FMath::Max(BaseReward, 0);
    const int32 Kills = FMath::Max(Targets, 0);
    const int32 Rate = FMath::Max(PerTarget, 0);
    return Base + Kills * Rate;
}

int32 SideJob::TimeBonus(double TimeTaken, double ParTime, int32 Bonus)
{
    const int32 Reward = FMath::Max(Bonus, 0);
    if (ParTime <= 0.0 || Reward == 0)
    {
        return 0;
    }
    const double Taken = FMath::Max(TimeTaken, 0.0);
    if (Taken <= ParTime)
    {
        return Reward; // at or under par -> full bonus
    }
    if (Taken >= 2.0 * ParTime)
    {
        return 0; // at/over 2x par -> nothing
    }
    // Between par and 2x par: shrink linearly to 0.
    const double Frac = 1.0 - (Taken - ParTime) / ParTime;
    return FMath::Max(FMath::RoundToInt(static_cast<double>(Reward) * Frac), 0);
}

int32 SideJob::Payout(const FJob& Job, double Distance, double TimeTaken, double ParTime)
{
    const int32 Base = FMath::Max(Job.BaseReward, 0);
    int32 Core = 0;
    if (static_cast<EKind>(Job.Kind) == EKind::Vigilante)
    {
        // Distance carries the kill count for vigilante contracts.
        Core = VigilanteReward(FMath::RoundToInt(FMath::Max(Distance, 0.0)), Base, DefaultPerTarget);
    }
    else
    {
        Core = Fare(Distance, Base, DefaultPerMeter);
    }
    const int32 Bonus = TimeBonus(TimeTaken, ParTime, Base);
    return FMath::Max(Core + Bonus, 0);
}

double SideJob::ChainMultiplier(int32 ConsecutiveCompleted)
{
    const int32 Streak = FMath::Max(ConsecutiveCompleted, 0);
    return FMath::Min(1.0 + static_cast<double>(Streak) * ChainStep, MaxChainMultiplier);
}

// --- Stateful active-job tracker -------------------------------------------

SideJob::SideJob()
    : bActive(false)
    , CurrentStage(EStage::Done)
    , CompletedJobs(0)
{
}

void SideJob::Start(const FJob& Job)
{
    Active = Job;
    bActive = true;
    CurrentStage = EStage::Pickup;
}

bool SideJob::HasActive() const
{
    return bActive && CurrentStage != EStage::Done;
}

int32 SideJob::ActiveKind() const
{
    if (!HasActive())
    {
        return -1;
    }
    return Active.Kind;
}

SideJob::EStage SideJob::Stage() const
{
    return CurrentStage;
}

bool SideJob::IsPickupDone() const
{
    return HasActive() && CurrentStage == EStage::Dropoff;
}

void SideJob::AdvanceStage()
{
    if (!HasActive())
    {
        return;
    }
    if (CurrentStage == EStage::Pickup)
    {
        CurrentStage = EStage::Dropoff;
    }
}

void SideJob::Complete()
{
    if (!HasActive())
    {
        return;
    }
    CompletedJobs += 1;
    bActive = false;
    CurrentStage = EStage::Done;
}

void SideJob::Cancel()
{
    if (!HasActive())
    {
        return;
    }
    bActive = false;
    CurrentStage = EStage::Done;
}

int32 SideJob::CompletedCount() const
{
    return CompletedJobs;
}
