// Copyright (c) 2026 GTC contributors

#include "MissionChain.h"

MissionChain::MissionChain(const TArray<FMissionDef>& InMissions)
    : Missions(InMissions)
{
}

int32 MissionChain::Count() const
{
    return Missions.Num();
}

int32 MissionChain::ActiveIndex() const
{
    return Index;
}

MissionChain::FMissionDef MissionChain::Current() const
{
    if (Index < 0 || Index >= Missions.Num())
    {
        return FMissionDef();
    }
    return Missions[Index];
}

FString MissionChain::CurrentId() const
{
    const FMissionDef Mission = Current();
    return Mission.IsEmpty() ? FString() : Mission.Id;
}

void MissionChain::CompleteCurrent()
{
    if (Index < Missions.Num())
    {
        ++Index;
    }
}

bool MissionChain::IsCampaignComplete() const
{
    return Index >= Missions.Num();
}

int32 MissionChain::Remaining() const
{
    return FMath::Max(Missions.Num() - Index, 0);
}

int32 MissionChain::Completed() const
{
    return FMath::Min(Index, Missions.Num());
}

double MissionChain::Progress() const
{
    if (Missions.Num() == 0)
    {
        return 1.0;
    }
    return FMath::Clamp(static_cast<double>(Index) / static_cast<double>(Missions.Num()), 0.0, 1.0);
}

void MissionChain::Reset()
{
    Index = 0;
}
