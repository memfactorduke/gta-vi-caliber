// Copyright Epic Games, Inc. All Rights Reserved.

#include "MissionObjectiveTypes.h"

FString MissionObjectiveTypes::KindName(int32 Kind)
{
    switch (static_cast<EKind>(Kind))
    {
    case EKind::Reach:
        return TEXT("reach");
    case EKind::Collect:
        return TEXT("collect");
    case EKind::Eliminate:
        return TEXT("eliminate");
    case EKind::Escort:
        return TEXT("escort");
    case EKind::Survive:
        return TEXT("survive");
    case EKind::Defend:
        return TEXT("defend");
    default:
        return FString();
    }
}

bool MissionObjectiveTypes::ReachSatisfied(const FVector& PlayerPos, const FVector& TargetPos, double Radius)
{
    return FVector::Distance(PlayerPos, TargetPos) <= FMath::Max(Radius, 0.0);
}

double MissionObjectiveTypes::CollectProgress(int32 Collected, int32 Required)
{
    if (Required <= 0)
    {
        return 1.0;
    }
    return FMath::Clamp(
        static_cast<double>(FMath::Max(Collected, 0)) / static_cast<double>(Required), 0.0, 1.0);
}

bool MissionObjectiveTypes::CollectSatisfied(int32 Collected, int32 Required)
{
    if (Required <= 0)
    {
        return true;
    }
    return FMath::Max(Collected, 0) >= Required;
}

bool MissionObjectiveTypes::EliminateSatisfied(int32 TargetsRemaining)
{
    return FMath::Max(TargetsRemaining, 0) <= 0;
}

bool MissionObjectiveTypes::EscortFailed(double EscorteeHealth)
{
    return EscorteeHealth <= 0.0;
}

bool MissionObjectiveTypes::EscortSatisfied(const FVector& EscorteePos, const FVector& DestPos, double Radius)
{
    return FVector::Distance(EscorteePos, DestPos) <= FMath::Max(Radius, 0.0);
}

double MissionObjectiveTypes::SurviveProgress(double TimeSurvived, double Duration)
{
    if (Duration <= 0.0)
    {
        return 1.0;
    }
    return FMath::Clamp(FMath::Max(TimeSurvived, 0.0) / Duration, 0.0, 1.0);
}

bool MissionObjectiveTypes::SurviveSatisfied(double TimeSurvived, double Duration)
{
    if (Duration <= 0.0)
    {
        return true;
    }
    return TimeSurvived >= Duration;
}

bool MissionObjectiveTypes::DefendFailed(double StructureHealth)
{
    return StructureHealth <= 0.0;
}

// --- Counter ---------------------------------------------------------------

MissionObjectiveTypes::Counter::Counter(int32 Target)
    : TargetCount(FMath::Max(Target, 0))
{
}

int32 MissionObjectiveTypes::Counter::Add(int32 N)
{
    if (N > 0)
    {
        CurrentCount += N;
    }
    return CurrentCount;
}

int32 MissionObjectiveTypes::Counter::Count() const
{
    return CurrentCount;
}

int32 MissionObjectiveTypes::Counter::Target() const
{
    return TargetCount;
}

int32 MissionObjectiveTypes::Counter::Remaining() const
{
    return FMath::Max(TargetCount - CurrentCount, 0);
}

bool MissionObjectiveTypes::Counter::IsDone() const
{
    return CurrentCount >= TargetCount;
}

double MissionObjectiveTypes::Counter::Progress() const
{
    return MissionObjectiveTypes::CollectProgress(CurrentCount, TargetCount);
}

void MissionObjectiveTypes::Counter::Reset()
{
    CurrentCount = 0;
}
