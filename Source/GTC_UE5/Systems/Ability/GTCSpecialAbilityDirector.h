// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "SpecialAbility.h"
#include "GTCSpecialAbilityDirector.generated.h"

/**
 * Self-wiring adapter that makes FSpecialAbility LIVE — the per-protagonist special bar as a
 * driven subsystem. It charges from real play (it binds the existing public
 * UWantedSubsystem::OnStarsChanged so committing crime feeds the bar, and exposes
 * NotifyKill/NotifyStunt entry points the weapon/stunt layers call), drains while active
 * (FTickableGameObject), and exposes the effect factors the rest of the game multiplies in
 * (time dilation, damage in/out, vehicle handling).
 *
 * Reaches into NO contended files: it BINDS UWantedSubsystem's public delegate and OWNS its
 * FSpecialAbility. Input triggers it via ActivateAbility(); the time-dilation / damage /
 * handling consumers read the getters. Mirrors UGTCNotorietyDirector (the proven wiring
 * pattern). Lifetime: UGameInstanceSubsystem (per-lead special persists across sublevels).
 */
UCLASS()
class GTC_UE5_API UGTCSpecialAbilityDirector : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    /** Charge added per extra wanted star (a crime spree pumps adrenaline). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    float ChargePerStar = 0.1f;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return bInitialized; }
    virtual bool IsTickableWhenPaused() const override { return false; }

    /** Bound to UWantedSubsystem::OnStarsChanged — a star rise charges the bar. */
    UFUNCTION()
    void HandleStarsChanged(int32 Stars);

    // --- Charge entry points for the gameplay layers (the documented seams, callable) ---
    UFUNCTION(BlueprintCallable, Category = "Ability")
    void NotifyKill(bool bHeadshot = false);

    UFUNCTION(BlueprintCallable, Category = "Ability")
    void NotifyStunt();

    UFUNCTION(BlueprintCallable, Category = "Ability")
    void NotifyNearMiss();

    /** Pick which protagonist's flavour the bar uses (0 Marksman, 1 Bruiser, 2 Driver). */
    UFUNCTION(BlueprintCallable, Category = "Ability")
    void SetKind(int32 Kind);

    // --- Trigger + query surface ---
    UFUNCTION(BlueprintCallable, Category = "Ability")
    bool ActivateAbility() { return Ability.Activate(); }

    UFUNCTION(BlueprintCallable, Category = "Ability")
    void DeactivateAbility() { Ability.Deactivate(); }

    UFUNCTION(BlueprintCallable, Category = "Ability")
    float ChargeFraction() const { return static_cast<float>(Ability.Fraction()); }

    UFUNCTION(BlueprintCallable, Category = "Ability")
    bool IsAbilityActive() const { return Ability.IsActive(); }

    UFUNCTION(BlueprintCallable, Category = "Ability")
    float TimeDilation() const { return static_cast<float>(Ability.TimeDilation()); }

    UFUNCTION(BlueprintCallable, Category = "Ability")
    float DamageTakenMultiplier() const { return static_cast<float>(Ability.DamageTakenMultiplier()); }

    UFUNCTION(BlueprintCallable, Category = "Ability")
    float DamageDealtMultiplier() const { return static_cast<float>(Ability.DamageDealtMultiplier()); }

    UFUNCTION(BlueprintCallable, Category = "Ability")
    float HandlingBoost() const { return static_cast<float>(Ability.HandlingBoost()); }

    FSpecialAbility& Model() { return Ability; }

private:
    /** Push the current time scale (Ability.TimeDilation() while active, else 1.0) to the world. */
    void ApplyTimeDilation();

    FSpecialAbility Ability;
    int32 LastStars = 0;
    bool bInitialized = false;
    bool bDilationApplied = false; // are we currently overriding global time dilation?
};
