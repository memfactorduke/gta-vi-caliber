// Copyright (c) 2026 GTC contributors

#include "NpcMind.h"

#include "NpcNeeds.h"
#include "Math/UnrealMathUtility.h"

const TArray<FNpcNeedActivity>& FNpcMind::NeedActivity()
{
    static const TArray<FNpcNeedActivity> Map = {
        {TEXT("energy"), TEXT("sleep"), TEXT("home")},
        {TEXT("hunger"), TEXT("eat"), TEXT("diner")},
        {TEXT("social"), TEXT("socialize"), TEXT("bar")},
        {TEXT("fun"), TEXT("goof_off"), TEXT("park")},
        {TEXT("hygiene"), TEXT("freshen_up"), TEXT("restroom")},
    };
    return Map;
}

FNpcIntent FNpcMind::Decide(
    const TArray<FNpcScheduleBlock>& Blocks,
    double Hour,
    const FNpcNeeds& Needs,
    double Discipline)
{
    const FNpcScheduleBlock Scheduled = FNpcSchedule::ActivityAt(Blocks, Hour);
    const FString Need = Needs.MostUrgent();
    const double Urg = Needs.Urgency(Need);

    const double Disc = FMath::Clamp(Discipline, -1.0, 1.0);
    const double Threshold = FMath::Clamp(OverrideThreshold + Disc * 0.2, 0.4, 0.95);

    if (Urg >= Threshold)
    {
        for (const FNpcNeedActivity& Entry : NeedActivity())
        {
            if (Entry.Need == Need)
            {
                FNpcIntent Out;
                Out.Activity = Entry.Activity;
                Out.Place = Entry.Place;
                Out.Reason = FString::Printf(TEXT("need:%s"), *Need);
                Out.Urgency = Urg;
                return Out;
            }
        }
    }

    FNpcIntent Out;
    Out.Activity = Scheduled.Activity;
    Out.Place = Scheduled.Place;
    Out.Reason = TEXT("schedule");
    Out.Urgency = Urg;
    return Out;
}
