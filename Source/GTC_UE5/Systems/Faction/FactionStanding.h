// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure faction-reputation model — how each gang/faction feels about the player on a
 * -100 (hostile) .. +100 (allied) scale. Helping a faction raises its standing (and,
 * via rivalry, lowers its enemy's); attacking it lowers it. Standing gates behaviour:
 * a hostile faction's NPCs attack on sight, an allied faction's assist in a fight.
 * Faction ids line up with GangTerritory's gangs, so turf and reputation share one set
 * of factions (turf = who holds ground, standing = how they treat you).
 *
 * Plain C++ value type (no UObject): an AI/world controller owns one, adjusts standing
 * on player actions, and reads WillAttack/WillAssist to drive NPC behaviour — so the
 * standing/tier/rivalry math stays unit-tested headless.
 *
 * Each faction is defined by {Id, Rival}; Rival "" means no rivalry. Malformed entries
 * (missing/empty id) and duplicate ids are dropped.
 *
 * Parity note: Godot's Dictionary is insertion-ordered and Ids()/ToMap() rely on it.
 * TMap does NOT preserve order, so an explicit ordered backing store (TArray<FEntry> +
 * TMap<FString,int32> index) is used.
 */
class GTC_UE5_API FactionStanding
{
public:
    /** Standing range. */
    static constexpr int32 MinStanding = -100;
    static constexpr int32 MaxStanding = 100;

    /** Tier thresholds (standing <= / >=). */
    static constexpr int32 HostileAt = -40;
    static constexpr int32 WaryAt = -10;
    static constexpr int32 FriendlyAt = 10;
    static constexpr int32 AlliedAt = 40;

    /** Default fraction of an adjustment that bleeds (inverted) onto a rival. */
    static constexpr double DefaultSpillover = 0.5;

    /** One faction definition: an id, an optional rival, and a current standing. */
    struct FFactionDef
    {
        FString Id;
        FString Rival;

        FFactionDef() = default;
        FFactionDef(const FString& InId, const FString& InRival)
            : Id(InId)
            , Rival(InRival)
        {
        }
    };

    /** Construct from a list of {Id, Rival}; an empty list uses DefaultFactions(). */
    explicit FactionStanding(const TArray<FFactionDef>& Factions = TArray<FFactionDef>());

    /** Built-in factions, ids aligned with GangTerritory's gangs plus the police. */
    static TArray<FFactionDef> DefaultFactions();

    int32 FactionCount() const;
    bool HasFaction(const FString& Id) const;

    /** Faction ids in insertion order. */
    TArray<FString> Ids() const;

    /** The faction's rival id ("" if none / unknown). */
    FString RivalOf(const FString& Id) const;

    /** Standing in [-100, 100] (0 / neutral if unknown). */
    int32 StandingOf(const FString& Id) const;

    /** Named tier for the current standing ("" if unknown). */
    FString TierOf(const FString& Id) const;

    bool IsHostile(const FString& Id) const;
    bool IsAllied(const FString& Id) const;

    /** Whether this faction's NPCs attack the player on sight. */
    bool WillAttack(const FString& Id) const;

    /** Whether this faction's NPCs will help the player in a fight. */
    bool WillAssist(const FString& Id) const;

    /** Set standing directly (loading a save / story beat), clamped. No-op if unknown. */
    void SetStanding(const FString& Id, int32 Value);

    /**
     * Shift a faction's standing by Delta (clamped). A non-zero Spillover applies the
     * inverse, scaled, to the faction's rival (helping one gang angers its enemy).
     * No-op for an unknown faction.
     */
    void Adjust(const FString& Id, int32 Delta, double Spillover = DefaultSpillover);

    /** Flatten to {Id -> standing} for the save system, in insertion order. */
    TArray<TPair<FString, int32>> ToMap() const;

    /** Restore from {Id -> standing}. Unknown ids ignored; known clamped. */
    void LoadMap(const TArray<TPair<FString, int32>>& Data);

private:
    struct FEntry
    {
        FString Id;
        FString Rival;
        int32 Standing = 0;
    };

    /** Insertion-ordered storage. */
    TArray<FEntry> Entries;
    /** Id -> index into Entries. */
    TMap<FString, int32> Index;

    void Register(const FFactionDef& Def);

    const FEntry* Find(const FString& Id) const;
    FEntry* Find(const FString& Id);
};
