// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../../NPC/Vitals/NpcVitals.h"
#include "GTCPoliceHelicopter.generated.h"

class UGTCWeaponComponent;
class UStaticMeshComponent;
class USpotLightComponent;
class AGTCPoliceOfficer;

/**
 * AGTCPoliceHelicopter — the air-support actor that EMBODIES FHelicopterPursuit:
 * the police chopper that joins the chase at high heat. It orbits above the
 * player, sweeps a searchlight onto them, and a door gunner fires down once the
 * player is lit. It is the airborne sibling of AGTCPoliceOfficer — a thin AActor
 * shell over the same tested cores (FHelicopterPursuit for the orbit/spotlight,
 * FNpcVitals for damage, the reused UGTCWeaponComponent for the gun).
 *
 * FRAME NOTE: FHelicopterPursuit is the Godot frame (ground = XZ, altitude = +Y);
 * Unreal is Z-up (ground = XY, altitude = +Z). The orbit position is remapped at
 * the boundary (swap Y<->Z); the spotlight-footprint scalars and the ground-plane
 * "is the target lit" test are frame-agnostic and used directly. Pure-core radii
 * are metres -> *100 for cm.
 *
 * Spawned/recalled by AGTCPoliceDirector (a singleton air unit, separate from the
 * foot-officer waves); destroyed when shot down (a serious crime — reports a kill).
 */
UCLASS()
class GTC_UE5_API AGTCPoliceHelicopter : public AActor
{
    GENERATED_BODY()

public:
    AGTCPoliceHelicopter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    /** True once killed — counting down to the wreck despawn. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Police")
    bool IsDead() const { return bDead; }

    /** Current hull health as a 0..1 fraction. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Police")
    float GetHealthFraction() const;

    virtual float TakeDamage(
        float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    /** Fired once when shot down — the seam for the explosion/smoke/fall FX. The
     *  C++ has already entered the death state and scheduled the despawn. */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|Police|Combat")
    void OnShotDown();

protected:
    /** Orbit radius (cm) the chopper circles the player at. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Heli")
    double OrbitRadiusCm = 2800.0;

    /** Orbit altitude (cm) above the player. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Heli")
    double AltitudeCm = 3200.0;

    /** Orbit angular speed (radians/sec). */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Heli")
    double AngularSpeed = 0.5;

    /** How fast the body eases toward its orbit point (interp speed). */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Heli")
    double MoveInterpSpeed = 2.5;

    /** Hull health — much tougher than a foot officer. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Heli")
    double MaxHealth = 600.0;

    /** Seconds a downed wreck lingers before despawning. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Heli")
    double WreckLingerSeconds = 8.0;

    /** SWAT trooper the chopper fast-ropes in. Defaults to AGTCPoliceOfficer. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Heli")
    TSubclassOf<AGTCPoliceOfficer> DropOfficerClass;

    /** Most troopers a single chopper inserts over its life. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Heli")
    int32 MaxDrops = 3;

    /** Seconds between fast-rope insertions. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Heli")
    double DropIntervalSec = 7.0;

    /** Stars at/above which the chopper inserts troops (SWAT-grade response). */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Heli")
    int32 DropMinStars = 4;

private:
    UPROPERTY(VisibleAnywhere, Category = "GTC|Police|Heli", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> Body;

    UPROPERTY(VisibleAnywhere, Category = "GTC|Police|Heli", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USpotLightComponent> Searchlight;

    /** Door-gunner weapon (reused player weapon component, fired via aim override). */
    UPROPERTY(VisibleAnywhere, Category = "GTC|Police|Heli", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UGTCWeaponComponent> Weapon;

    FNpcVitals Vitals;
    bool bDead = false;
    double OrbitTime = 0.0;
    double FireTimer = 0.0;
    int32 CachedStars = 0;

    /** Fast-rope drop cadence + remaining budget. */
    double DropTimer = 0.0;
    int32 DropsRemaining = 0;

    APawn* ResolveTarget() const;
    int32 ReadStars() const;

    /** Fast-rope one SWAT trooper down to the player's position (NavMesh-projected). */
    void DropTrooper(const FVector& TargetGround);

    /** Point the body + searchlight at the player and resolve "is the player lit". */
    bool AimAndLight(const FVector& TargetGround, float DeltaSeconds);

    void EnterDeath(bool bByPlayer);
    void ReportCrimeToWanted(bool bKilled) const;
};
