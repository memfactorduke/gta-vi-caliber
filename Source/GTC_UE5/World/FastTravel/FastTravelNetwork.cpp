// Copyright Epic Games, Inc. All Rights Reserved.

#include "FastTravelNetwork.h"

namespace GTCFastTravel
{
    // cm per kilometre: 100 cm/m * 1000 m/km.
    static constexpr double CmPerKm = 100000.0;

    // Nearest qualifying hub by a caller-supplied predicate; ties keep the lower index.
    template <typename PredT>
    static int32 NearestWhere(const TArray<FFastTravelNetwork::FHub>& Hubs, const FVector& From,
        double MaxRangeCm, PredT&& Pred)
    {
        int32 Best = FFastTravelNetwork::None;
        double BestDist = 0.0;
        const bool bLimited = MaxRangeCm > 0.0;
        for (int32 I = 0; I < Hubs.Num(); ++I)
        {
            const FFastTravelNetwork::FHub& H = Hubs[I];
            if (!H.bUnlocked || !Pred(H))
            {
                continue;
            }
            const double Dist = (H.Location - From).Size();
            if (bLimited && Dist > MaxRangeCm)
            {
                continue;
            }
            if (Best == FFastTravelNetwork::None || Dist < BestDist)
            {
                Best = I;
                BestDist = Dist;
            }
        }
        return Best;
    }
}

int32 FFastTravelNetwork::NearestHub(const TArray<FHub>& Hubs, const FVector& From, double MaxRangeCm)
{
    return GTCFastTravel::NearestWhere(Hubs, From, MaxRangeCm, [](const FHub&) { return true; });
}

int32 FFastTravelNetwork::NearestHubOfKind(const TArray<FHub>& Hubs, const FVector& From, EHubKind Kind, double MaxRangeCm)
{
    return GTCFastTravel::NearestWhere(Hubs, From, MaxRangeCm, [Kind](const FHub& H) { return H.Kind == Kind; });
}

int32 FFastTravelNetwork::Fare(const FHub& From, const FHub& To, double PerKm, int32 BaseFare)
{
    const double Km = (To.Location - From.Location).Size() / GTCFastTravel::CmPerKm;
    const double Rate = FMath::Max(0.0, PerKm);
    const int32 Base = BaseFare > 0 ? BaseFare : 0;
    const int32 Variable = static_cast<int32>(Rate * Km + 0.5); // round, non-negative
    return Base + Variable;
}

double FFastTravelNetwork::TravelSeconds(const FHub& From, const FHub& To, double AssumedSpeedCmS)
{
    const double Speed = FMath::Max(1.0, AssumedSpeedCmS); // avoid divide-by-zero; 1 cm/s floor
    return (To.Location - From.Location).Size() / Speed;
}

bool FFastTravelNetwork::CanDepart(bool bWanted, double NearestThreatM, double SafeRangeM)
{
    if (bWanted)
    {
        return false;
    }
    return NearestThreatM >= SafeRangeM;
}
