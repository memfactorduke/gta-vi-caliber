// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerHealthModel.h"
#include "PlayerHealthComponent.generated.h"

/**
 * UPlayerHealthComponent — the UE analogue of the Godot `PlayerHealth` Node
 * (`game/scripts/systems/player_health.gd`), which wrapped a PlayerHealthModel.
 *
 * Two layers, mirroring the Wave 2 two-layer approach:
 *
 *  - FPlayerHealthModel : the pure data/logic value type carrying every
 *    parity math (damage/armor soak/regen curve/heal/revive), 1:1 with the
 *    Godot source and its parity oracle test_player_health_model.gd (18 funcs).
 *    No UObject, headless-testable.
 *
 *  - UPlayerHealthComponent : a plain UActorComponent that OWNS an
 *    FPlayerHealthModel and re-exposes it as clamped accessors plus change
 *    delegates (the UE replacement for Godot signals). Heavy maths stay in the
 *    model; this is the thin owner + lifecycle + broadcast shell, so the data
 *    can be parity-tested without a world.
 *
 * --------------------------------------------------------------------------
 * DEFERRED-OWNERSHIP (lead-signed, option 1 — own-state, defer write-through)
 * --------------------------------------------------------------------------
 * The Godot design has a HEALTH-FROM-WANTED coupling: the wanted/heat system can
 * influence health, regen, and respawn. That coupling is INTENTIONALLY NOT
 * implemented here. Per the approved option 1, this component OWNS its own
 * health state and exposes it ONLY via getters/delegates; it does NOT include,
 * reference, or depend on any Wanted type (that lives in a sibling worktree).
 * The write-through / integration (a Wanted subsystem reading or modulating this
 * component's regen/respawn) is DEFERRED to a Wave 3 integration pass. This is a
 * deliberate lead-signed deferred coupling, recorded here so it is approved
 * knowingly rather than silently dropped.
 *
 * Likewise PlayerStats integration is runtime/deferred: this component does NOT
 * include or depend on FPlayerStats/UPlayerStatsComponent. (Both this component
 * and PlayerStats model an armor pool 1:1 from their respective Godot sources;
 * reconciling the two armor owners is a runtime-wiring decision for Wave 3, not
 * part of this parity port.)
 *
 * --------------------------------------------------------------------------
 * NEEDS_DECISION (ownership/lifecycle): who SPAWNS/owns this component (the
 * player Pawn vs a PlayerState), and the replication model (COND_OwnerOnly vs
 * everyone; health/armor must almost certainly be server-authoritative). The
 * default below is a single-player-safe starting point (replicated, no RPC
 * guard) and is flagged for that decision before wiring into a pawn.
 *
 * Parity notes vs Godot:
 *  - `double` model maths; accessors down-cast to float at the Blueprint boundary.
 *  - No Z-up remap (no spatial data in this system).
 */

/** Health changed -> (Health, MaxHealth). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGtcHealthChanged, float, Health, float, MaxHealth);
/** Armor changed -> (Armor, MaxArmor). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGtcHealthArmorChanged, float, Armor, float, MaxArmor);
/** Player died (the hit that dropped health to zero). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGtcPlayerDied);

UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UPlayerHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPlayerHealthComponent();

    // --- Change delegates (Godot signals) --------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "GTC|Player|Health")
    FGtcHealthChanged OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Player|Health")
    FGtcHealthArmorChanged OnArmorChanged;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Player|Health")
    FGtcPlayerDied OnDied;

    // --- Clamped accessors -----------------------------------------------------

    UFUNCTION(BlueprintPure, Category = "GTC|Player|Health")
    float GetHealth() const { return static_cast<float>(Model.Health); }

    UFUNCTION(BlueprintPure, Category = "GTC|Player|Health")
    float GetMaxHealth() const { return static_cast<float>(Model.MaxHealth); }

    UFUNCTION(BlueprintPure, Category = "GTC|Player|Health")
    float GetArmor() const { return static_cast<float>(Model.Armor); }

    UFUNCTION(BlueprintPure, Category = "GTC|Player|Health")
    float GetMaxArmor() const { return static_cast<float>(Model.MaxArmor); }

    UFUNCTION(BlueprintPure, Category = "GTC|Player|Health")
    bool IsDead() const { return Model.IsDead(); }

    /** Health bar fill fraction in [0,1]. */
    UFUNCTION(BlueprintPure, Category = "GTC|Player|Health")
    float GetHealthFraction() const { return static_cast<float>(Model.Fraction()); }

    /** Armor bar fill fraction in [0,1]. */
    UFUNCTION(BlueprintPure, Category = "GTC|Player|Health")
    float GetArmorFraction() const { return static_cast<float>(Model.ArmorFraction()); }

    // --- Mutators (broadcast on change) ----------------------------------------

    /** Apply damage through armor then health; returns true if this hit killed. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Health")
    bool ApplyDamage(float Amount);

    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Health")
    void AddArmor(float Amount);

    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Health")
    void Heal(float Amount);

    /** Advance the regen timer / regeneration by one frame's worth of time. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Health")
    void TickHealth(float DeltaSeconds);

    /** Full revive: restores health, clears armor, resets the regen timer. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Health")
    void Revive();

    /** Direct read access to the owned pure data (tests / sibling systems). */
    const FPlayerHealthModel& GetModel() const { return Model; }

    // --- Replication -----------------------------------------------------------

    /**
     * Health is replicated COND_OwnerOnly (owner = PlayerState — see header
     * resolution). Server-authoritative mutators are damage/heal/revive (no
     * armor: armor is owned solely by UPlayerStatsComponent per the W3
     * armor-ownership resolution).
     */
    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;

private:
    /**
     * The owned pure-data store carrying all parity maths. Constructed with
     * ArmorMax = 0 so the model's armor pool is NEUTRALIZED (W3 armor-ownership
     * resolution): AddArmor clamps to 0 and Apply soaks 0 — this component is
     * health-only. The 18 FPlayerHealthModel parity tests stay green untouched.
     */
    FPlayerHealthModel Model{100.0, 10.0, 5.0, /*ArmorMax=*/0.0};

    /** Replicated mirror of Model.Health (COND_OwnerOnly). */
    UPROPERTY(ReplicatedUsing = OnRep_Health)
    float RepHealth = 100.0f;

    UFUNCTION()
    void OnRep_Health();

    void BroadcastHealth();
    void BroadcastArmor();
};
