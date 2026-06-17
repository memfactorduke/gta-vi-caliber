// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * FWantedSystem — pure heat/stars model for the wanted/police-response system.
 *
 * UE 5.7 port of the reference `wanted_system.gd` (class WantedSystem, RefCounted). Plain
 * C++ value type (no UObject); a UWantedSubsystem owns one and feeds it crimes and
 * time, so the escalation/decay curve is unit-tested headless (reference behavior
 * game/tests/unit/test_wanted_system.gd, 11 funcs). "Heat" is an internal
 * accumulator; players see it quantised into 0-5 stars.
 *
 * double precision throughout (LWC + the reference float parity): heat, decay and the star
 * thresholds are doubles so the curve matches the the reference oracle.
 *
 * NOTE: distinct from FWantedLevel (Source/GTC_UE5/World/Police/WantedLevel.h), which
 * is the Wave-1 port of a *different* the reference source (wanted_level.gd). No type-name or
 * test-prefix collision (GTC.Systems.Wanted.* here vs GTC.World.Police.* there).
 */
class GTC_UE5_API FWantedSystem
{
public:
    /** Maximum star rating. */
    static constexpr int32 MaxStars = 5;

    /** Heat needed for each of the 1..5 stars (the reference STAR_THRESHOLDS). */
    static const TArray<double>& StarThresholds();

    double Heat = 0.0;
    double DecayRate = 0.4;
    double HeatCap = 20.0;

    /**
     * Construct with a per-second decay rate and a heat cap (Godot _init defaults
     * decay=0.4, cap=20.0).
     */
    explicit FWantedSystem(double Decay = 0.4, double Cap = 20.0)
        : DecayRate(Decay), HeatCap(Cap)
    {
    }

    /** Register a crime of the given severity (negative ignored), clamped to the cap. */
    void AddCrime(double Severity)
    {
        Heat = FMath::Min(Heat + FMath::Max(Severity, 0.0), HeatCap);
    }

    /**
     * Advance one frame. Heat only cools while no crime is in progress, so a
     * sustained rampage holds the level up.
     */
    void Tick(double Delta, bool bCommitting)
    {
        if (!bCommitting)
        {
            Heat = FMath::Max(Heat - DecayRate * Delta, 0.0);
        }
    }

    int32 Stars() const { return StarsFor(Heat); }

    bool IsWanted() const { return Heat > 0.0; }

    /** Stars for a given heat level (pure, so the HUD and tests agree). */
    static int32 StarsFor(double HeatValue)
    {
        int32 Count = 0;
        for (double Threshold : StarThresholds())
        {
            if (HeatValue >= Threshold)
            {
                ++Count;
            }
        }
        return Count;
    }

    /** How many police should be actively responding at a star level. */
    static int32 ResponseUnits(int32 Stars)
    {
        return FMath::Clamp(Stars, 0, MaxStars);
    }
};
