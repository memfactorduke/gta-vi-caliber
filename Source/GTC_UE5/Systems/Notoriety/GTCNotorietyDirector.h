// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "Notoriety.h"
#include "GTCNotorietyDirector.generated.h"

/**
 * Self-wiring adapter that makes FNotoriety LIVE. Unlike the pure model, this is an actual
 * driven subsystem: on construction it binds the existing UWantedSubsystem::OnStarsChanged
 * delegate (a public, BlueprintAssignable signal already broadcast when the player's star
 * count changes), so a real crime spree accrues street infamy; it bleeds that infamy off
 * every frame (FTickableGameObject); and when the infamy is high enough and the cooldown has
 * elapsed it broadcasts OnBountyHunters so a spawner (Blueprint or native) can send a hit
 * squad after the player.
 *
 * It reaches into NO contended files: it only BINDS UWantedSubsystem's public delegate and
 * OWNS its FNotoriety. Hunter spawning + the HUD readout are left to whatever binds
 * OnBountyHunters / reads NotorietyTier()/CurrentBounty() — the clean seam.
 *
 * Lifetime: a UGameInstanceSubsystem, matching UWantedSubsystem (player-global infamy that
 * outlives streamed sublevels).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBountyHunters, int32, WaveSize, float, Bounty);

UCLASS()
class GTC_UE5_API UGTCNotorietyDirector : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    /** Fires when a bounty-hunter wave should spawn (a spawner binds this). */
    UPROPERTY(BlueprintAssignable, Category = "Notoriety")
    FOnBountyHunters OnBountyHunters;

    // USubsystem lifecycle: bind / unbind the wanted signal.
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // FTickableGameObject: bleed infamy + fire hunter waves.
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return bInitialized; }
    virtual bool IsTickableWhenPaused() const override { return false; }

    /** Bound to UWantedSubsystem::OnStarsChanged — a rise in stars adds infamy. */
    UFUNCTION()
    void HandleStarsChanged(int32 Stars);

    // --- HUD / query surface ---
    UFUNCTION(BlueprintCallable, Category = "Notoriety")
    float CurrentInfamy() const { return static_cast<float>(Notoriety.Notoriety); }

    UFUNCTION(BlueprintCallable, Category = "Notoriety")
    float CurrentBounty() const { return static_cast<float>(Notoriety.BountyAmount()); }

    UFUNCTION(BlueprintCallable, Category = "Notoriety")
    int32 NotorietyTier() const { return static_cast<int32>(Notoriety.Tier()); }

    /** Direct model access for tests / advanced callers. */
    FNotoriety& Model() { return Notoriety; }

private:
    FNotoriety Notoriety;
    int32 LastStars = 0;
    bool bInitialized = false;
};
