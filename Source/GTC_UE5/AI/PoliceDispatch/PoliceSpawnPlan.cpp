// Copyright Epic Games, Inc. All Rights Reserved.

#include "PoliceSpawnPlan.h"
#include "PoliceDispatch.h"

TArray<FPoliceSpawnSlot> FPoliceSpawnPlan::Build(
    int32 Stars,
    int32 Alive,
    int32 MaxAlive,
    int32 MaxPerWave,
    double SpawnRadiusMeters,
    FRandomStream& Rng,
    double RadialJitterMeters,
    double JitterFrac)
{
    TArray<FPoliceSpawnSlot> Out;

    const int32 Count = FPoliceDispatch::SpawnCount(Stars, Alive, MaxAlive, MaxPerWave);
    const TArray<EPoliceUnitType> Units = FPoliceEscalation::ResponseUnits(Stars);
    if (Count <= 0 || Units.Num() == 0)
    {
        return Out; // nothing to add this wave (satisfied, or not wanted)
    }

    Out.Reserve(Count);
    for (int32 Index = 0; Index < Count; ++Index)
    {
        // Two RNG draws per slot mirror the dispatch helpers' (angle jitter, radial
        // jitter) sampling; drawing from the injected stream keeps a seeded run
        // deterministic.
        const double Angle = FPoliceDispatch::RingAngle(Index, Count, Rng.FRand(), JitterFrac);
        const FVector Offset = FPoliceDispatch::RingPosition(
            FVector::ZeroVector, SpawnRadiusMeters, Angle, Rng.FRand(), RadialJitterMeters);

        FPoliceSpawnSlot Slot;
        // Deal the heat's unit mix round-robin so a small wave still reflects the
        // composition (e.g. a 1-of-3 wave isn't always the first listed type).
        Slot.UnitType = Units[Index % Units.Num()];
        Slot.PlanarOffset = Offset;
        Out.Add(Slot);
    }

    return Out;
}
