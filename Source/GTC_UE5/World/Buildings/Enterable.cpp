// Copyright (c) 2026 GTC contributors

#include "Enterable.h"

namespace
{
    // OSM building types that get an interior (the reference PUBLIC_TYPES). Membership is
    // the only observable property, so a direct set lookup matches the oracle.
    bool IsPublicType(const FString& Kind)
    {
        return Kind == TEXT("retail")
            || Kind == TEXT("commercial")
            || Kind == TEXT("office")
            || Kind == TEXT("supermarket")
            || Kind == TEXT("hotel")
            || Kind == TEXT("civic")
            || Kind == TEXT("public")
            || Kind == TEXT("hospital")
            || Kind == TEXT("church")
            || Kind == TEXT("government");
    }
}

bool FEnterable::IsEnterable(const FBuilding& Building)
{
    if (!Building.Name.IsEmpty())
    {
        return true;
    }
    return IsPublicType(Building.Kind);
}

FVector2D FEnterable::DoorPoint(const TArray<FVector2D>& Footprint)
{
    const int32 N = Footprint.Num();
    if (N < 2)
    {
        return N == 0 ? FVector2D::ZeroVector : Footprint[0];
    }
    double Best = 0.0;
    FVector2D Door = (Footprint[0] + Footprint[1]) * 0.5;
    for (int32 I = 0; I < N; ++I)
    {
        const FVector2D& A = Footprint[I];
        const FVector2D& B = Footprint[(I + 1) % N];
        const double Length = FVector2D::Distance(A, B);
        if (Length > Best)
        {
            Best = Length;
            Door = (A + B) * 0.5;
        }
    }
    return Door;
}

TArray<FBuilding> FEnterable::Pick(const TArray<FBuilding>& Buildings, int32 MaxCount)
{
    TArray<FBuilding> Out;
    for (const FBuilding& B : Buildings)
    {
        if (IsEnterable(B))
        {
            Out.Add(B);
            if (Out.Num() >= MaxCount)
            {
                break;
            }
        }
    }
    return Out;
}
