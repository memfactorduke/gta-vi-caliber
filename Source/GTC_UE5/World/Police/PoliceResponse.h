// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * The whole police response profile bundled together: how many units pursue, how
 * aggressive they are, how far out they spawn, and whether a helicopter joins.
 *
 * Ported from the the reference `PoliceResponse.profile()` dictionary
 * (game/scripts/world/police_response.gd).
 */
struct FPoliceProfile
{
    int32 Units = 0;
    bool bHelicopter = false;
    double Aggression = 0.0;
    double SpawnRadius = 0.0;
};

/**
 * Maps a wanted-star count to a police response profile: how many units pursue,
 * how aggressive they are, how far out they spawn, and whether a helicopter
 * joins. Pure and scene-free so escalation rules unit-test headless
 * (World/Police/Tests/PoliceResponseTest.cpp); a spawner/AI layer consumes the
 * profile.
 *
 * Ported 1:1 from the the reference RefCounted `PoliceResponse`
 * (game/scripts/world/police_response.gd). All-static plain functions, no
 * UObject.
 *
 * Double precision throughout (LWC): aggression and spawn radius are doubles to
 * match the the reference oracle's float arithmetic bit-for-bit.
 */
class GTC_UE5_API FPoliceResponse
{
public:
    /** Star count at which a helicopter joins the pursuit. */
    static constexpr int32 HelicopterStars = 3;

    /** Pursuing units per star count (index = stars, 0-5). */
    static const int32 UnitsPerStar[6];

    static int32 UnitsFor(int32 Stars)
    {
        return UnitsPerStar[FMath::Clamp(Stars, 0, 5)];
    }

    static bool UsesHelicopter(int32 Stars)
    {
        return Stars >= HelicopterStars;
    }

    /** 0.0 (calm) ... 1.0 (lethal) - drives chase speed, fire rate, ram willingness. */
    static double Aggression(int32 Stars)
    {
        return FMath::Clamp(double(Stars) / 5.0, 0.0, 1.0);
    }

    /** Metres from the player that new units spawn - wider nets at higher heat. */
    static double SpawnRadius(int32 Stars)
    {
        return FMath::Lerp(40.0, 140.0, Aggression(Stars));
    }

    /** Convenience: the whole profile as a struct. */
    static FPoliceProfile Profile(int32 Stars)
    {
        FPoliceProfile P;
        P.Units = UnitsFor(Stars);
        P.bHelicopter = UsesHelicopter(Stars);
        P.Aggression = Aggression(Stars);
        P.SpawnRadius = SpawnRadius(Stars);
        return P;
    }
};
