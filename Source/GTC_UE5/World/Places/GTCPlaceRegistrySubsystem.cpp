// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCPlaceRegistrySubsystem.h"

#include "Engine/World.h"
#include "NavigationSystem.h"

int32 UGTCPlaceRegistrySubsystem::RegisterPlace(FName Kind, const FVector& Location, int32 Capacity)
{
    if (Kind.IsNone())
    {
        return 0;
    }
    FGTCPlace Place;
    Place.Kind = FName(*Kind.ToString().ToLower());
    Place.Location = Location;
    Place.Capacity = FMath::Max(0, Capacity);

    const int32 Handle = NextHandle++;
    Places.Add(Handle, Place);
    return Handle;
}

void UGTCPlaceRegistrySubsystem::UnregisterPlace(int32 Handle)
{
    Places.Remove(Handle);
}

int32 UGTCPlaceRegistrySubsystem::CountOfKind(FName Kind) const
{
    const FName Lower(*Kind.ToString().ToLower());
    int32 Count = 0;
    for (const TPair<int32, FGTCPlace>& Pair : Places)
    {
        if (Pair.Value.Kind == Lower)
        {
            ++Count;
        }
    }
    return Count;
}

FGTCPlaceQueryResult UGTCPlaceRegistrySubsystem::FindNearest(FName Kind, const FVector& From) const
{
    FGTCPlaceQueryResult Result;
    if (Kind.IsNone())
    {
        return Result; // bValid stays false.
    }
    const FName Lower(*Kind.ToString().ToLower());

    const FGTCPlace* Best = nullptr;
    double BestDistSq = TNumericLimits<double>::Max();
    for (const TPair<int32, FGTCPlace>& Pair : Places)
    {
        if (Pair.Value.Kind != Lower)
        {
            continue;
        }
        const double DistSq = FVector::DistSquared(Pair.Value.Location, From);
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Best = &Pair.Value;
        }
    }

    Result.bValid = true;
    if (Best)
    {
        Result.Location = Best->Location;
        Result.bSynthesized = false;
        Result.Capacity = Best->Capacity;
        const int32* Used = Occupancy.Find(OccupancyKey(Lower, Best->Location));
        Result.Occupancy = Used ? *Used : 0;
    }
    else
    {
        Result.Location = SynthesizeLocation(Lower, From);
        Result.bSynthesized = true;
    }
    return Result;
}

FGTCPlaceQueryResult UGTCPlaceRegistrySubsystem::FindAvailable(FName Kind, const FVector& From) const
{
    FGTCPlaceQueryResult Result;
    if (Kind.IsNone())
    {
        return Result;
    }
    const FName Lower(*Kind.ToString().ToLower());

    const FGTCPlace* Best = nullptr;
    const FGTCPlace* BestAny = nullptr; // nearest regardless of capacity (fallback)
    double BestDistSq = TNumericLimits<double>::Max();
    double BestAnyDistSq = TNumericLimits<double>::Max();

    for (const TPair<int32, FGTCPlace>& Pair : Places)
    {
        const FGTCPlace& P = Pair.Value;
        if (P.Kind != Lower)
        {
            continue;
        }
        const double DistSq = FVector::DistSquared(P.Location, From);
        if (DistSq < BestAnyDistSq)
        {
            BestAnyDistSq = DistSq;
            BestAny = &P;
        }
        // Capacity 0 == unlimited. Otherwise honour live occupancy.
        if (P.Capacity > 0)
        {
            const int32* Used = Occupancy.Find(OccupancyKey(Lower, P.Location));
            if (Used && *Used >= P.Capacity)
            {
                continue; // full — skip for the "available" pass.
            }
        }
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Best = &P;
        }
    }

    Result.bValid = true;
    const FGTCPlace* Chosen = Best ? Best : BestAny;
    if (Best)
    {
        Result.Location = Best->Location;
        Result.bSynthesized = false;
    }
    else if (BestAny)
    {
        // All same-kind POIs are full: send them to the nearest anyway (a queue
        // forms — more lifelike than teleporting to a stand-in).
        Result.Location = BestAny->Location;
        Result.bSynthesized = false;
    }
    if (Chosen)
    {
        Result.Capacity = Chosen->Capacity;
        const int32* Used = Occupancy.Find(OccupancyKey(Lower, Chosen->Location));
        Result.Occupancy = Used ? *Used : 0;
    }
    else
    {
        Result.Location = SynthesizeLocation(Lower, From);
        Result.bSynthesized = true;
    }
    return Result;
}

void UGTCPlaceRegistrySubsystem::NotePlaceClaimed(FName Kind, const FVector& Location)
{
    const FName Lower(*Kind.ToString().ToLower());
    int32& Used = Occupancy.FindOrAdd(OccupancyKey(Lower, Location));
    ++Used;
}

void UGTCPlaceRegistrySubsystem::NotePlaceReleased(FName Kind, const FVector& Location)
{
    const FName Lower(*Kind.ToString().ToLower());
    if (int32* Used = Occupancy.Find(OccupancyKey(Lower, Location)))
    {
        *Used = FMath::Max(0, *Used - 1);
        if (*Used == 0)
        {
            Occupancy.Remove(OccupancyKey(Lower, Location));
        }
    }
}

void UGTCPlaceRegistrySubsystem::EnsureSeeded(const FVector& Center)
{
    if (bSeeded || !bAutoSeedCity)
    {
        return;
    }
    bSeeded = true; // Mark first so a failed projection can't loop us back in.

    // A whole district's worth of amenities scattered across the WALKABLE MAP, not a
    // tight ring around the anchor. The spread is the point: when destinations sit a
    // few hundred metres apart, citizens stream in near the player and then head off
    // to genuinely distant places — so the city reads as people living their own
    // lives, not a crowd orbiting whoever's watching. Several of each kind let
    // FindAvailable fan the crowd out instead of piling everyone onto one spot.
    struct FKindSeed { const TCHAR* Kind; int32 Count; int32 Capacity; };
    static const FKindSeed Kinds[] = {
        {TEXT("home"),     14, 0},
        {TEXT("office"),    6, 0},
        {TEXT("diner"),     5, 8},
        {TEXT("bar"),       4, 12},
        {TEXT("park"),      3, 0},
        {TEXT("gym"),       3, 10},
        {TEXT("restroom"),  5, 2},
        {TEXT("street"),   24, 0},
    };

    UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    // ~350 m: scatter POIs across the whole walkable district, not a bubble around
    // the anchor, so citizens head off to genuinely distant places. A local constant
    // (not a UPROPERTY) keeps this a pure-.cpp change Live Coding can hot-patch.
    const double Spread = 35000.0;

    for (const FKindSeed& K : Kinds)
    {
        for (int32 i = 0; i < K.Count; ++i)
        {
            FVector Point;
            bool bGotPoint = false;

            // Prefer a real, reachable navmesh point somewhere across the district, so
            // the POI sits on walkable ground the crowd can actually path to.
            if (Nav)
            {
                FNavLocation Rand;
                if (Nav->GetRandomReachablePointInRadius(Center, (float)Spread, Rand))
                {
                    Point = Rand.Location;
                    bGotPoint = true;
                }
            }

            // Fallback (no navmesh): a deterministic scatter keyed off kind+index, so a
            // navmesh-less map still gets spread-out destinations rather than a stack
            // on the anchor. Stable across runs (no RNG state touched).
            if (!bGotPoint)
            {
                const uint32 H = HashCombine(GetTypeHash(FName(K.Kind)), GetTypeHash(i * 2654435761u));
                const double Angle = (static_cast<double>(H % 3600u) / 3600.0) * UE_DOUBLE_TWO_PI;
                const double Radius = Spread * (0.15 + 0.85 * (static_cast<double>((H / 7u) % 1000u) / 1000.0));
                Point = Center + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0);
            }

            RegisterPlace(FName(K.Kind), Point, K.Capacity);
        }
    }
}

FVector UGTCPlaceRegistrySubsystem::SynthesizeLocation(FName Kind, const FVector& From) const
{
    // A stable point on a ring around the *snapped* query origin. Snapping to a
    // grid means an NPC wandering within a cell keeps the same stand-in "home" /
    // "diner" instead of chasing a destination that drifts with its own position.
    const double Cell = FMath::Max(1.0, SynthesizedCellSize);
    const double CellX = FMath::GridSnap(From.X, Cell);
    const double CellY = FMath::GridSnap(From.Y, Cell);

    // Hash the kind into a stable bearing so different kinds sit at different
    // points of the ring (home north-ish, diner east-ish, ... — deterministic).
    const uint32 KindHash = GetTypeHash(Kind);
    const double Angle = (static_cast<double>(KindHash % 3600u) / 3600.0) * UE_DOUBLE_TWO_PI;

    const FVector Center(CellX, CellY, From.Z);
    return Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0) * SynthesizedRingRadius;
}

uint32 UGTCPlaceRegistrySubsystem::OccupancyKey(FName Kind, const FVector& Location)
{
    // Snap to a coarse grid so floating-point jitter in a registered location
    // can't fragment occupancy across near-identical keys.
    const FIntVector Snapped(
        FMath::RoundToInt(Location.X / 100.0),
        FMath::RoundToInt(Location.Y / 100.0),
        FMath::RoundToInt(Location.Z / 100.0));
    uint32 Key = GetTypeHash(Kind);
    Key = HashCombine(Key, GetTypeHash(Snapped));
    return Key;
}
