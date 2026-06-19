// Copyright (c) 2026 GTC contributors

#include "Mission.h"

Mission::Mission(const FString& MissionTitle, const FString& ObjectiveText, int32 Target, double Limit)
    : Title(MissionTitle)
    , Objective(ObjectiveText)
    , Required(FMath::Max(Target, 1))
    , TimeLimit(FMath::Max(Limit, 0.0))
    , TimeLeft(FMath::Max(Limit, 0.0))
{
}

bool Mission::IsActive() const
{
    return Status == EStatus::Active;
}

void Mission::Record(int32 Amount)
{
    if (Status != EStatus::Active)
    {
        return;
    }
    Progress = FMath::Min(Progress + FMath::Max(Amount, 0), Required);
    if (Progress >= Required)
    {
        Status = EStatus::Complete;
    }
}

void Mission::Tick(double Delta)
{
    if (Status != EStatus::Active || TimeLimit <= 0.0)
    {
        return;
    }
    TimeLeft = FMath::Max(TimeLeft - Delta, 0.0);
    if (TimeLeft <= 0.0)
    {
        Status = EStatus::Failed;
    }
}

double Mission::Fraction() const
{
    return static_cast<double>(Progress) / static_cast<double>(Required);
}

void Mission::Reset()
{
    Progress = 0;
    Status = EStatus::Active;
    TimeLeft = TimeLimit;
}
