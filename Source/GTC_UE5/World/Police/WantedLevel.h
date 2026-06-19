// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Wanted/heat model: crimes raise "heat", which maps to a 0-5 star rating and
 * decays over time when the player lies low. Pure and scene-free so the rules
 * unit-test headless (World/Police/Tests/WantedLevelTest.cpp); a HUD/AI layer
 * reads Stars() and reacts.
 *
 * Ported 1:1 from the Godot RefCounted `WantedLevel`
 * (game/scripts/world/wanted_level.gd). Plain value type, no UObject.
 *
 * Double precision throughout (LWC): heat and the star thresholds are doubles to
 * match the Godot oracle's float arithmetic bit-for-bit.
 */
class GTC_UE5_API FWantedLevel
{
public:
    /** Maximum star rating. */
    static constexpr int32 MaxStars = 5;

    /** Heat required to reach each star count (index = stars, 0-5). */
    static const double StarThresholds[6];

    /** Heat lost per second while lying low. */
    double Heat = 0.0;
    double CoolRate = 0.5;

    /** Heat added per crime kind. */
    static const TMap<FString, double>& CrimeHeat();

    void AddHeat(double Amount)
    {
        Heat = FMath::Clamp(Heat + Amount, 0.0, StarThresholds[MaxStars]);
    }

    void AddCrime(const FString& Kind)
    {
        const double* Found = CrimeHeat().Find(Kind);
        AddHeat(Found != nullptr ? *Found : 1.0);
    }

    /** Cool down over `Delta` seconds. */
    void Decay(double Delta)
    {
        Heat = FMath::Max(0.0, Heat - CoolRate * Delta);
    }

    int32 Stars() const
    {
        int32 S = 0;
        for (int32 i = 1; i < UE_ARRAY_COUNT(StarThresholds); ++i)
        {
            if (Heat >= StarThresholds[i])
            {
                S = i;
            }
        }
        return S;
    }

    bool IsWanted() const
    {
        return Stars() > 0;
    }

    void Clear()
    {
        Heat = 0.0;
    }
};
