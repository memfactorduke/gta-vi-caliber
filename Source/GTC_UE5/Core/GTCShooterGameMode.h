// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GTCShooterRules.h"
#include "GTCShooterGameMode.generated.h"

/**
 * AGTCShooterGameMode — GTC's own GTA-style game mode, so the project owns the
 * rules a stock GameMode can't carry: a starting arsenal/loadout, starting cash &
 * health, respawn timing and spawn protection — with headroom for the GTA loop
 * (wanted-level dispatch, mission state, on-foot <-> vehicle hand-off) to grow on
 * top.
 *
 * The pawn / controller / HUD are assigned in the Blueprint child
 * (Content/GTCShooter/BP_GTCShooterGameMode) because they reference the paid,
 * gitignored Third-Person-Shooter-Kit assets, which must never enter this public
 * repo. This C++ base therefore keeps NO hard reference to those assets: the
 * StartingWeapons list is soft and empty by default, and the Blueprint fills it
 * with the kit / MarketplaceBlockout PDA_WeaponDefinition entries.
 */
UCLASS()
class GTC_UE5_API AGTCShooterGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AGTCShooterGameMode();

    /** Cash a brand-new player starts a session with. */
    UPROPERTY(EditDefaultsOnly, Category = "GTC|Shooter")
    int32 StartingCash = 500;

    /** Health the player (re)spawns with. */
    UPROPERTY(EditDefaultsOnly, Category = "GTC|Shooter")
    float StartingHealth = 100.0f;

    /** Seconds between death and respawn. */
    UPROPERTY(EditDefaultsOnly, Category = "GTC|Shooter")
    float RespawnDelaySeconds = 3.0f;

    /** Seconds of post-spawn damage immunity. */
    UPROPERTY(EditDefaultsOnly, Category = "GTC|Shooter")
    float SpawnProtectionSeconds = 2.0f;

    /** Hand the player the starting arsenal on spawn. */
    UPROPERTY(EditDefaultsOnly, Category = "GTC|Shooter")
    bool bArmPlayerOnSpawn = true;

    /**
     * Starting arsenal, as SOFT references so this committable C++ never hard-links
     * the paid kit weapon-definition data assets. The Blueprint child fills these
     * with the kit / MarketplaceBlockout PDA_WeaponDefinition entries.
     */
    UPROPERTY(EditDefaultsOnly, Category = "GTC|Shooter")
    TArray<TSoftObjectPtr<UObject>> StartingWeapons;

    /** Normalized, validated rules in effect this match (built at BeginPlay). */
    const FGTCShooterRules& GetRules() const { return Rules; }

protected:
    virtual void BeginPlay() override;

private:
    /** Designer config, clamped to playable ranges once at BeginPlay. */
    FGTCShooterRules Rules;
};
