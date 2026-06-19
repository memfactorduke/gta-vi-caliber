// Copyright (c) 2026 GTC contributors

#include "NpcNeeds.h"

#include "Math/UnrealMathUtility.h"

const TArray<FString>& FNpcNeeds::Needs()
{
    static const TArray<FString> CanonicalNeeds = {
        TEXT("energy"),
        TEXT("hunger"),
        TEXT("social"),
        TEXT("fun"),
        TEXT("hygiene"),
    };
    return CanonicalNeeds;
}

FNpcNeeds::FNpcNeeds(double Initial)
{
    const double V = FMath::Clamp(Initial, 0.0, 1.0);
    for (const FString& N : Needs())
    {
        Values.Add(N, V);
    }
}

void FNpcNeeds::Decay(double Hours, const TMap<FString, double>& Rates)
{
    for (const FString& N : Needs())
    {
        const double R = Rates.Contains(N) ? Rates[N] : 0.0;
        Values[N] = FMath::Clamp(Values[N] - R * Hours, 0.0, 1.0);
    }
}

void FNpcNeeds::Satisfy(const FString& Need, double Amount)
{
    if (Values.Contains(Need))
    {
        Values[Need] = FMath::Clamp(Values[Need] + Amount, 0.0, 1.0);
    }
}

double FNpcNeeds::Urgency(const FString& Need) const
{
    const double* Found = Values.Find(Need);
    return 1.0 - (Found ? *Found : 1.0);
}

FString FNpcNeeds::MostUrgent() const
{
    FString Worst;
    double WorstV = TNumericLimits<double>::Max();
    for (const FString& N : Needs())
    {
        const double V = Values[N];
        if (V < WorstV)
        {
            WorstV = V;
            Worst = N;
        }
    }
    return Worst;
}

double FNpcNeeds::PeakUrgency() const
{
    return Urgency(MostUrgent());
}
