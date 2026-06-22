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

    // A believable little district laid out around the town anchor: a couple of
    // each amenity kind so FindAvailable can spread the crowd, plus several homes
    // and street hubs (the most-visited kinds). Offsets are in cm; r ~ a few city
    // blocks. Each is projected onto the NavMesh when one exists so citizens walk
    // to walkable ground rather than into the surf or a wall.
    struct FSeed { const TCHAR* Kind; double Angle; double Radius; int32 Capacity; };
    static const FSeed Seeds[] = {
        {TEXT("home"),     0.20, 9000.0,  0},
        {TEXT("home"),     2.40, 11000.0, 0},
        {TEXT("home"),     4.10, 8000.0,  0},
        {TEXT("home"),     5.50, 12000.0, 0},
        {TEXT("office"),   0.90, 6000.0,  0},
        {TEXT("office"),   3.30, 7000.0,  0},
        {TEXT("diner"),    1.20, 4500.0,  8},
        {TEXT("diner"),    4.70, 5200.0,  8},
        {TEXT("bar"),      2.10, 5000.0,  12},
        {TEXT("bar"),      5.90, 6300.0,  12},
        {TEXT("park"),     1.80, 7500.0,  0},
        {TEXT("gym"),      3.80, 4800.0,  10},
        {TEXT("shop"),     2.60, 5500.0,  0},
        {TEXT("shop"),     5.10, 6100.0,  0},
        {TEXT("restroom"), 0.50, 3500.0,  2},
        {TEXT("restroom"), 3.10, 4000.0,  2},
        {TEXT("street"),   1.00, 3000.0,  0},
        {TEXT("street"),   2.80, 3600.0,  0},
        {TEXT("street"),   4.40, 3200.0,  0},
        {TEXT("street"),   6.00, 3800.0,  0},
    };

    UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

    for (const FSeed& S : Seeds)
    {
        FVector Point = Center + FVector(FMath::Cos(S.Angle) * S.Radius, FMath::Sin(S.Angle) * S.Radius, 0.0);
        if (Nav)
        {
            FNavLocation Projected;
            if (Nav->ProjectPointToNavigation(Point, Projected, FVector(800.0, 800.0, 2000.0)))
            {
                Point = Projected.Location;
            }
        }
        RegisterPlace(FName(S.Kind), Point, S.Capacity);
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
