// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerStats.generated.h"

/**
 * Player stats / vitals store — ported from Godot `PlayerStats` (a Node in the
 * "player_stats" group): the reactive HUD store for the vitals no other system
 * owns yet — body armor, the wallet, and the active objective. Health lives in
 * PlayerHealth and heat in WantedTracker; this fills the remaining gaps.
 *
 * Two layers, mirroring the Wave 2 two-layer approach:
 *
 *  - FPlayerStats  : the pure data/logic value type. Holds the stat state and
 *                    all clamp/absorb/derived maths, exactly 1:1 with the Godot
 *                    source and its parity oracle (test_player_stats.gd). No
 *                    UObject, headless-testable, no engine context required.
 *                    The static helpers `Absorb` / `Fraction` are the unit-tested
 *                    pure maths; the mutators (SoakDamage/AddArmor/AddMoney/
 *                    SpendMoney/SetObjective/...) carry the Godot Node mutator
 *                    semantics minus the signal emission.
 *
 *  - UPlayerStatsComponent : a plain replicated stats UActorComponent that OWNS
 *                    an FPlayerStats and exposes clamped accessors + change
 *                    delegates. This is the UE analogue of the Godot Node's
 *                    self-wiring + signals.
 *
 * APPROVED design (option 2): this is a *plain replicated stats component*, NOT
 * a full GAS attribute set. GAS is a possible LATER adoption, so the component's
 * interface is deliberately kept "GAS-migration-friendly": every stat is modelled
 * as a named clamped getter/setter plus a change delegate (Armor/Money/Objective),
 * rather than ad-hoc public fields. A future GAS UAttributeSet can mirror this 1:1
 * — each accessor becomes an FGameplayAttribute getter, each delegate becomes the
 * attribute's PostGameplayEffectExecute change broadcast, and the clamp moves into
 * PreAttributeChange. Keep that shape if you extend this.
 *
 * Parity notes vs Godot:
 *  - `double` throughout for armor maths (Godot `float` == 64-bit). No Z-up remap.
 *  - The objective waypoint is an FVector (Godot Vector3) but is NEVER remapped —
 *    it is opaque store-and-return data, so axis order is irrelevant to parity.
 *  - serialize()/restore() round-trip preserved, including the "garbage keeps the
 *    current value" rule of SaveData.number_or (non-numeric Variant -> fallback).
 */

USTRUCT()
struct GTC_UE5_API FPlayerStats
{
    GENERATED_BODY()

    // --- Tunables (Godot @export defaults) -------------------------------------

    /** Armor ceiling. Godot `@export var max_armor := 100.0`. */
    double MaxArmor = 100.0;

    /** Starting wallet so the HUD reads as a live game from frame 1. Godot 1500. */
    int32 StartingMoney = 1500;

    /** Seed objective so the quest tracker isn't empty on a fresh world. */
    FString StartingObjective = TEXT("Explore the city");

    // --- Live state ------------------------------------------------------------

    /** Current armor pool, always in [0, MaxArmor]. */
    double Armor = 0.0;

    /** Wallet balance, floored at 0. Initialised from StartingMoney in InitDefaults(). */
    int32 Money = 0;

    FString ObjectiveTitle;
    FVector ObjectiveWaypoint = FVector::ZeroVector;

    bool bHasWaypoint = false;

    /**
     * Mirror of Godot `_ready()`: seed the wallet from StartingMoney and, if no
     * objective is set yet, seed the starting objective. Pure data step, no
     * group join / signal — the component drives delegates separately.
     */
    void InitDefaults();

    // --- Mutators (gameplay pushes changes in; HUD only reads) -----------------

    /**
     * Soak incoming damage with armor first; returns the overflow that should
     * reach health. Mutates Armor. Returns the (non-negative) overflow.
     * `bOutArmorChanged` reports whether Armor actually moved (drives the delegate).
     */
    double SoakDamage(double Amount, bool& bOutArmorChanged);
    /** Convenience overload when the caller doesn't care if armor changed. */
    double SoakDamage(double Amount);

    /** Add armor (negatives ignored), clamped to [0, MaxArmor]. */
    void AddArmor(double Amount);

    /** Adjust the wallet, floored at 0 (a large fine can't drive it negative). */
    void AddMoney(int32 Amount);

    /** Spend if affordable; true on success. Non-positive amount never succeeds. */
    bool SpendMoney(int32 Amount);

    /** Store-and-return objective; waypoint is opaque (no axis remap). */
    void SetObjective(const FString& Title, const FVector& Waypoint = FVector::ZeroVector, bool bInHasWaypoint = false);

    /** Clear the objective (empty title, no waypoint). */
    void ClearObjective();

    bool HasWaypoint() const { return bHasWaypoint; }

    // --- Persistence (SaveManager analogue) ------------------------------------

    /** Snapshot for the save system — mirrors Godot {"money", "armor"}. */
    void Serialize(int32& OutMoney, double& OutArmor) const;

    /**
     * Rebuild from a Serialize() snapshot. `bMoneyValid`/`bArmorValid` model the
     * SaveData.number_or "is the field numeric?" check: a false flag keeps the
     * current value (garbage survives), a true flag applies the clamped value.
     */
    void Restore(bool bMoneyValid, double InMoney, bool bArmorValid, double InArmor);

    // --- Pure helpers (unit-tested) -------------------------------------------

    /**
     * Apply `Damage` against an `ArmorValue` pool. Returns the pair
     * [RemainingArmor, OverflowToHealth] via out-params: armor absorbs up to its
     * value, the rest spills over. Negatives are floored at 0.
     */
    static void Absorb(double ArmorValue, double Damage, double& OutRemainingArmor, double& OutOverflow);

    /** Bar fill fraction in [0,1], safe against a zero/negative maximum. */
    static double Fraction(double Value, double Maximum);
};

/** Armor changed -> (Armor, MaxArmor). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGtcArmorChanged, float, Armor, float, MaxArmor);
/** Money changed -> (Amount). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGtcMoneyChanged, int32, Amount);
/** Objective changed -> (Title, bHasWaypoint). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGtcObjectiveChanged, const FString&, Title, bool, bHasWaypoint);

/**
 * UPlayerStatsComponent — the UE analogue of the Godot `PlayerStats` Node.
 *
 * Owns an FPlayerStats and re-exposes its mutators as clamped accessors that
 * also broadcast change delegates (the UE replacement for Godot signals). The
 * heavy parity maths live in FPlayerStats; this layer is the thin owner +
 * lifecycle + broadcast shell, so the data can be parity-tested without a world.
 *
 * NEEDS_DECISION (ownership/lifecycle, NEW): this is the FIRST UCLASS in the
 * GTC_UE5 module — every prior Wave 1/2 unit is plain C++. Open questions for the
 * lead before this is wired into a pawn:
 *   (a) who SPAWNS/owns the component (the player Pawn vs a PlayerState) — Godot
 *       had it as a free-standing Node in the "player_stats" group, with no owner;
 *   (b) the replication model: COND_OwnerOnly vs everyone, and whether Money is
 *       server-authoritative (it almost certainly must be). Defaults below are a
 *       single-player-safe starting point (replicated, no RPC guard) and are
 *       flagged for that decision.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UPlayerStatsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPlayerStatsComponent();

    // --- Change delegates (Godot signals) --------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "GTC|Player|Stats")
    FGtcArmorChanged OnArmorChanged;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Player|Stats")
    FGtcMoneyChanged OnMoneyChanged;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Player|Stats")
    FGtcObjectiveChanged OnObjectiveChanged;

    // --- Clamped accessors (GAS-migration-friendly stat interface) -------------

    UFUNCTION(BlueprintPure, Category = "GTC|Player|Stats")
    float GetArmor() const { return static_cast<float>(Stats.Armor); }

    UFUNCTION(BlueprintPure, Category = "GTC|Player|Stats")
    float GetMaxArmor() const { return static_cast<float>(Stats.MaxArmor); }

    UFUNCTION(BlueprintPure, Category = "GTC|Player|Stats")
    int32 GetMoney() const { return Stats.Money; }

    UFUNCTION(BlueprintPure, Category = "GTC|Player|Stats")
    FString GetObjectiveTitle() const { return Stats.ObjectiveTitle; }

    UFUNCTION(BlueprintPure, Category = "GTC|Player|Stats")
    bool HasWaypoint() const { return Stats.HasWaypoint(); }

    UFUNCTION(BlueprintPure, Category = "GTC|Player|Stats")
    FVector GetObjectiveWaypoint() const { return Stats.ObjectiveWaypoint; }

    // --- Mutators (broadcast on change) ----------------------------------------

    /** Soak damage through armor; returns overflow to route to health. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Stats")
    float SoakDamage(float Amount);

    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Stats")
    void AddArmor(float Amount);

    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Stats")
    void AddMoney(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Stats")
    bool SpendMoney(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Stats")
    void SetObjective(const FString& Title, FVector Waypoint, bool bInHasWaypoint);

    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Stats")
    void ClearObjective();

    /** Direct read access to the owned pure data (tests / sibling systems). */
    const FPlayerStats& GetStats() const { return Stats; }

    // --- Replication -----------------------------------------------------------

    /**
     * Armor + Money replicated COND_OwnerOnly (owner = PlayerState — this
     * component is owned by AGTCPlayerState per the W3 armor-ownership
     * resolution). Armor is the SOLE armor pool (the health model's armor is
     * neutralized with ArmorMax=0). Mutators are server-authoritative: the armor
     * pool drains on the server (damage soak), money changes on the server
     * (economy), and the clamped values replicate down to the owning client.
     */
    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;

private:
    /** The owned pure-data store carrying all parity maths. */
    UPROPERTY()
    FPlayerStats Stats;

    /** Replicated mirror of Stats.Armor (the SOLE armor pool), COND_OwnerOnly. */
    UPROPERTY(ReplicatedUsing = OnRep_Armor)
    float RepArmor = 0.0f;

    /** Replicated mirror of Stats.Money, COND_OwnerOnly. */
    UPROPERTY(ReplicatedUsing = OnRep_Money)
    int32 RepMoney = 0;

    UFUNCTION()
    void OnRep_Armor();

    UFUNCTION()
    void OnRep_Money();

    void BroadcastArmor();
    void BroadcastMoney();
    void BroadcastObjective();
};
