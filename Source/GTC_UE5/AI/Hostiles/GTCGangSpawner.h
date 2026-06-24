// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "GTCGangSpawner.generated.h"

class AGTCHostile;
class AGTCTurret;
class AGTCBarricade;

/**
 * AGTCGangSpawner — a gang's turf. Drop one into the level and it keeps a small
 * band of AGTCHostile thugs alive around itself, but only while the player is near
 * (so the city isn't full of idling gunmen). When a member dies it tops the band
 * back up on a timer. This is the placement/streaming counterpart to
 * AGTCPoliceDirector, scoped down to one local pocket of trouble.
 *
 * Thin AActor: it only spawns, prunes, and respawns; the gunfight lives in the
 * AGTCHostile pawns (and the tested FCombatAi behind them).
 */
UCLASS()
class GTC_UE5_API AGTCGangSpawner : public AActor
{
    GENERATED_BODY()

public:
    AGTCGangSpawner();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

protected:
    /** Thug to spawn. Defaults to AGTCHostile. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Gang")
    TSubclassOf<AGTCHostile> HostileClass;

    /** How many live members the turf maintains. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Gang")
    int32 GangSize = 3;

    /** Radius (cm) members spawn within around this actor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Gang")
    double SpawnRadius = 700.0;

    /** The turf only spawns while the player is within this radius (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Gang")
    double ActivationRadius = 7000.0;

    /** Seconds between top-up passes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Gang")
    double RespawnInterval = 6.0;

    /** Fraction [0..1] of the band that spawns as unarmed melee thugs (the rest carry guns). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Gang", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    double MeleeFraction = 0.34;

    /** Deterministic seed for spawn placement. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Gang")
    int32 Seed = 4242;

    /** If true the turf is defended by a turret (respawned if destroyed). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Gang")
    bool bDeployTurret = false;

    /** Turret to mount. Defaults to AGTCTurret. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Gang")
    TSubclassOf<AGTCTurret> TurretClass;

    /** If true the turf is fortified with destructible barricades (placed once). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Gang")
    bool bDeployBarricades = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Gang")
    int32 BarricadeCount = 3;

    /** Barricade to place. Defaults to AGTCBarricade. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Gang")
    TSubclassOf<AGTCBarricade> BarricadeClass;

private:
    UPROPERTY()
    TArray<TWeakObjectPtr<AGTCHostile>> Members;

    /** The turf's mounted turret, when bDeployTurret is set. */
    TWeakObjectPtr<AGTCTurret> Turret;

    /** Barricades placed once when the player nears; tracked so the turf can be torn
     *  down (destroyed) when the player leaves. */
    TArray<TWeakObjectPtr<AGTCBarricade>> Barricades;
    bool bSpawnedBarricades = false;

    FRandomStream Rng;
    double RespawnTimer = 0.0;
    int32 SpawnCounter = 0;

    APawn* GetPlayer() const;
    bool PlayerInRange() const;
    void TopUp();
};
