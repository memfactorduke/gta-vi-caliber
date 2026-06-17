// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure gang turf-control model — districts owned by gangs, with a 0..1 player influence
 * level that rises as fights are won there and falls when contested.
 *
 * Plain C++ value type (no UObject): a world controller owns one and feeds it fight
 * results, so the influence/takeover curve is unit-tested headless. Each district has
 * {Id, Owner, HomeOwner, Influence}; takeover flips owner to "player" once influence is
 * full.
 *
 * Parity note: Godot's Dictionary is insertion-ordered and PlayerDistricts()/Serialize()/
 * ControlledFraction() rely on it. TMap does NOT preserve order, so an explicit ordered
 * backing store (TArray<FEntry> + TMap<FString,int32> index) is used. Influence is stored
 * as double to avoid 32-bit drift.
 */
class GTC_UE5_API GangTerritory
{
public:
    /** Owner string for districts the player has captured. */
    static const FString PlayerOwner;

    /** One district definition consumed by the constructor / Restore(). */
    struct FDistrictDef
    {
        FString Id;
        FString Owner;
        /** When unset (HomeOwner.IsEmpty()), defaults to Owner — matches Godot's dict.get fallback. */
        FString HomeOwner;
        bool bHasHomeOwner = false;
        double Influence = 0.0;

        FDistrictDef() = default;
        FDistrictDef(const FString& InId, const FString& InOwner)
            : Id(InId)
            , Owner(InOwner)
        {
        }
    };

    /** Construct from a list of {Id, Owner}; an empty list uses DefaultDistricts(). */
    explicit GangTerritory(const TArray<FDistrictDef>& Districts = TArray<FDistrictDef>());

    /** The built-in turf map: the city's districts each held by a rival gang, influence 0. */
    static TArray<FDistrictDef> DefaultDistricts();

    int32 DistrictCount() const;

    /** True if the district id exists in the turf map. */
    bool HasDistrict(const FString& DistrictId) const;

    /** Owner gang id of a district, or "" if the id is unknown. */
    FString OwnerOf(const FString& DistrictId) const;

    /** Current 0..1 player influence in a district, or 0 if the id is unknown. */
    double InfluenceIn(const FString& DistrictId) const;

    /** Winning a fight raises player influence (clamped 0..1). Negative / unknown are no-ops. */
    void AddInfluence(const FString& DistrictId, double Amount);

    /** Decay / contested loss lowers player influence (floored at 0). Negative / unknown are no-ops. */
    void LoseInfluence(const FString& DistrictId, double Amount);

    /** True when player influence is above the threshold. Unknown districts are never contested. */
    bool IsContested(const FString& DistrictId, double Threshold) const;

    /** Capture a district: succeeds only at full influence (>= 1.0), flipping owner to "player". */
    bool TakeOver(const FString& DistrictId);

    /** Every district id the player owns, in insertion order (empty if none). */
    TArray<FString> PlayerDistricts() const;

    /** Fraction (0..1) of all districts the player owns; 0 when there are none. */
    double ControlledFraction() const;

    /** True only when the player owns every district (false with no districts). */
    bool AllOwned() const;

    /** Snapshot for save data: a list of {Id, Owner, HomeOwner, Influence}, insertion-ordered. */
    TArray<FDistrictDef> Serialize() const;

    /** Rebuild from a Serialize() snapshot. */
    void Restore(const TArray<FDistrictDef>& Districts);

    /** Restore from a malformed/absent snapshot — leaves an empty map. */
    void RestoreEmpty();

    /** Reset every district back to zero player influence and its original gang owner. */
    void Reset();

private:
    struct FEntry
    {
        FString Id;
        FString Owner;
        FString HomeOwner;
        double Influence = 0.0;
    };

    /** Insertion-ordered storage. */
    TArray<FEntry> Entries;
    /** Id -> index into Entries. */
    TMap<FString, int32> Index;

    void Register(const FDistrictDef& Def);
    void SetInfluence(const FString& DistrictId, double Value);

    const FEntry* Find(const FString& Id) const;
    FEntry* Find(const FString& Id);
};
