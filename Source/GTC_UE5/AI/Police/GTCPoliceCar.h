// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../../NPC/Vitals/NpcVitals.h"
#include "GTCPoliceCar.generated.h"

class UGTCWeaponComponent;
class UBoxComponent;
class UStaticMeshComponent;

/**
 * AGTCPoliceCar — the ground pursuit vehicle that EMBODIES FPursuitTactics: a cop
 * cruiser that leads-and-intercepts a fleeing player, rams and PITs when the heat
 * authorises it, and a passenger gunner fires when in range. It is the wheeled
 * sibling of AGTCPoliceOfficer / AGTCPoliceHelicopter — a thin AActor over the
 * tested cores (FPursuitTactics for the driving decision, FNpcVitals for damage,
 * the reused UGTCWeaponComponent for the gun).
 *
 * FRAME/UNIT NOTE: FPursuitTactics is the Godot XZ frame (ground = XZ, Y ignored),
 * METRES; Unreal is Z-up (ground = XY), centimetres. Positions/velocities are
 * remapped + scaled at the boundary; the returned aim point comes back to Unreal
 * cm on the ground plane.
 *
 * The motion is intentionally KINEMATIC (steer toward the aim point, drive forward
 * at the tactic's desired speed) — functional pursuit + ram without full Chaos
 * vehicle physics, which is the live-editor polish pass. Spawned/recalled by
 * AGTCPolicedirector; shot up and it's a crime (reports a kill on death).
 */
UCLASS()
class GTC_UE5_API AGTCPoliceCar : public AActor
{
    GENERATED_BODY()

public:
    AGTCPoliceCar();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "GTC|Police")
    bool IsDead() const { return bDead; }

    UFUNCTION(BlueprintCallable, Category = "GTC|Police")
    float GetHealthFraction() const;

    virtual float TakeDamage(
        float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    /** Fired once when the cruiser is wrecked — the seam for the crash/smoke FX. */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|Police|Combat")
    void OnWrecked();

protected:
    /** Cruising top speed (cm/s) the pursuit ramps toward. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Car")
    double MaxSpeed = 1800.0;

    /** Speed (cm/s) held when already on the target's bumper. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Car")
    double BaseSpeed = 700.0;

    /** Yaw turn rate (deg/s) as the cruiser steers toward its aim point. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Car")
    double TurnRateDeg = 160.0;

    /** Range (cm) within which a Ram/contact lands on the player. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Car")
    double RamRangeCm = 350.0;

    /** Damage a ram does to the player on contact. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Car")
    double RamDamage = 28.0;

    /** Hull health — soaks a lot of fire before it's wrecked. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Car")
    double MaxHealth = 400.0;

    /** Seconds a wreck lingers before despawning. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police|Car")
    double WreckLingerSeconds = 12.0;

private:
    UPROPERTY(VisibleAnywhere, Category = "GTC|Police|Car", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UBoxComponent> Body;

    UPROPERTY(VisibleAnywhere, Category = "GTC|Police|Car", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> Mesh;

    /** Passenger gunner — the reused weapon component, fired via aim override. */
    UPROPERTY(VisibleAnywhere, Category = "GTC|Police|Car", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UGTCWeaponComponent> Weapon;

    FNpcVitals Vitals;
    bool bDead = false;
    double FireTimer = 0.0;
    double RamCooldown = 0.0;
    int32 CachedStars = 0;

    /** Pursuit memory: where the player was last seen + how long out of sight, so a
     *  broken sightline sends the cruiser to the last-known spot (FPursuitMemory). */
    FVector LastKnownPos = FVector::ZeroVector;
    double TimeUnseen = 0.0;
    bool bHasLastKnown = false;

    APawn* ResolveTarget() const;
    int32 ReadStars() const;
    /** Clear line from the car to the player chest (no geometry between)? */
    bool HasLineOfSight(const APawn* Target) const;

    void EnterDeath(bool bByPlayer);
    void ReportCrimeToWanted(bool bKilled) const;
};
