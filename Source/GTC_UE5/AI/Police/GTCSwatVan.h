// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../../NPC/Vitals/NpcVitals.h"
#include "GTCSwatVan.generated.h"

class UStaticMeshComponent;
class AGTCPoliceOfficer;
class AGTCBarricade;

/**
 * AGTCSwatVan — the heavy ground response: an armoured van that rolls up at high
 * heat and disgorges a SWAT squad. It completes the police fleet (pursuit cruiser
 * for the chase, helicopter for the air, this for boots on the ground) and is the
 * ground twin of the chopper's fast-rope insertion — drive to a standoff, throw the
 * doors, unload, done.
 *
 * Thin AActor: kinematic drive (steer + advance, no Chaos physics — that's the live
 * editor pass, same as the cruiser), FNpcVitals for damage, and it reuses
 * AGTCPoliceOfficer for the disembarking troopers (which run their own tested combat
 * brain and self-retire when the heat clears). Spawned/recalled by AGTCPoliceDirector.
 */
UCLASS()
class GTC_UE5_API AGTCSwatVan : public AActor
{
    GENERATED_BODY()

public:
    AGTCSwatVan();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category = "GTC|Police")
    bool IsDead() const { return bDead; }

    UFUNCTION(BlueprintCallable, Category = "GTC|Police")
    bool HasDeployed() const { return bDeployed; }

    UFUNCTION(BlueprintCallable, Category = "GTC|Police")
    float GetHealthFraction() const;

    virtual float TakeDamage(
        float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    /** Fired once when the van is wrecked — the seam for the explosion/smoke FX. */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|Police|Combat")
    void OnWrecked();

protected:
    /** Cruise speed (cm/s) while rolling up to the player. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Van")
    double ApproachSpeed = 1400.0;

    /** Yaw turn rate (deg/sec). */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Van")
    double TurnRateDeg = 140.0;

    /** Distance (cm) at which the van stops and deploys the squad. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Van")
    double StandoffCm = 1200.0;

    /** Hull health — tough, but the player can wreck it. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Van")
    double MaxHealth = 450.0;

    /** Seconds a wreck lingers before despawning. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Van")
    double WreckLingerSeconds = 8.0;

    /** Troopers unloaded on arrival. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Van")
    int32 SquadSize = 4;

    /** SWAT trooper to unload. Defaults to AGTCPoliceOfficer. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Van")
    TSubclassOf<AGTCPoliceOfficer> SquadOfficerClass;

    /** Destructible barricades dropped as forward cover. Defaults to AGTCBarricade. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Van")
    TSubclassOf<AGTCBarricade> BarricadeClass;

    UPROPERTY(EditAnywhere, Category = "GTC|Police|Van")
    int32 BarricadeCount = 2;

private:
    UPROPERTY(VisibleAnywhere, Category = "GTC|Police|Van", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> Body;

    FNpcVitals Vitals;
    bool bDead = false;
    bool bDeployed = false;
    int32 CachedStars = 0;

    /** Barricades this van dropped — destroyed in EndPlay so they don't outlive it. */
    TArray<TWeakObjectPtr<AGTCBarricade>> DeployedBarricades;

    APawn* ResolveTarget() const;
    int32 ReadStars() const;

    /** Unload the SWAT squad in a fan behind the van's rear doors. */
    void DeploySquad(const FVector& PlayerGround);

    void EnterDeath(bool bByPlayer);
    void ReportCrimeToWanted(bool bKilled) const;
};
