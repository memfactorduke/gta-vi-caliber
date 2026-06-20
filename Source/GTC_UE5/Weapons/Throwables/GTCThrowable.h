// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTCThrowable.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UProjectileMovementComponent;
class AGTCFire;

/**
 * AGTCThrowable — the Wave-3 actor that EMBODIES the thrown-explosive cores: it
 * lobs on the arc FThrowable describes, cooks a fuse, and on detonation resolves
 * the blast through FExplosionModel. It is the throwable counterpart to the
 * police officer over FPoliceCombat: a thin engine shell over scene-free, already
 * unit-tested math (FThrowable for the launch velocity + cook fuse, FExplosionModel
 * for radial damage + knockback).
 *
 * FRAME NOTE: FThrowable works in the adapter's own units — UE centimetres, +Z up —
 * so its velocity/arc feed the engine directly. FExplosionModel's distance-based
 * damage/falloff are frame-agnostic (used as-is), but its knockback biases "up"
 * along the Godot +Y axis; this adapter instead applies the lift on Unreal +Z using
 * the tested Falloff curve + UpwardBias constant, so bodies pop into the air
 * correctly in the engine frame.
 *
 * The arc/bounce is driven by a UProjectileMovementComponent (engine physics);
 * this actor only holds the live cook timer, gathers blast targets via a world
 * sphere overlap, applies damage/impulse, and fires OnDetonate for the FX layer.
 */
UCLASS()
class GTC_UE5_API AGTCThrowable : public AActor
{
    GENERATED_BODY()

public:
    AGTCThrowable();

    virtual void Tick(float DeltaSeconds) override;

    /**
     * Arm and launch this throwable: set its initial velocity from the aim and
     * throw speed (FThrowable::LaunchVelocity) and start the fuse countdown.
     * `FuseRemaining` is the time left after any cooking. If it's already <= 0
     * (cooked off in hand), it detonates on the next tick at the spawn point.
     */
    void Throw(const FVector& AimDir, double ThrowSpeed, double FuseRemaining);

    /**
     * Spawn + throw in one call. `CookSeconds` shortens the fuse (cooking); if the
     * cook exceeds the fuse the grenade goes off immediately. `Instigator` is the
     * damage instigator (so a player throw raises heat / scores kills via TakeDamage).
     * Returns the spawned actor, or null if the world/class is invalid.
     */
    static AGTCThrowable* SpawnAndThrow(
        UWorld* World, const FVector& Origin, const FVector& AimDir, AActor* InInstigator,
        double ThrowSpeed, double FuseSeconds, double CookSeconds, TSubclassOf<AGTCThrowable> Class,
        bool bIncendiary = false);

    /** Fired once at detonation — the seam for blast VFX/SFX. The C++ has already
     *  applied damage + knockback. `Center` is the blast origin, `RadiusCm` its reach. */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|Throwable")
    void OnDetonate(FVector Center, float RadiusCm);

protected:
    /** Blast reach in cm (5 m default). */
    UPROPERTY(EditAnywhere, Category = "GTC|Throwable")
    double BlastRadiusCm = 500.0;

    /** Damage at the centre, falling linearly to 0 at the radius. */
    UPROPERTY(EditAnywhere, Category = "GTC|Throwable")
    double MaxDamage = 120.0;

    /** Knockback impulse magnitude at the centre (cm/s velocity-change on characters). */
    UPROPERTY(EditAnywhere, Category = "GTC|Throwable")
    double MaxImpulse = 95000.0;

    /** Draw a debug blast sphere on detonation (editor/non-shipping). */
    UPROPERTY(EditAnywhere, Category = "GTC|Throwable")
    bool bDrawDebugBlast = true;

    /** Incendiary (molotov): leave a spreading fire at the detonation point. */
    UPROPERTY(EditAnywhere, Category = "GTC|Throwable")
    bool bIncendiary = false;

    /** Fire-patch class for the incendiary aftermath (defaults to AGTCFire). */
    UPROPERTY(EditAnywhere, Category = "GTC|Throwable")
    TSubclassOf<AGTCFire> FireClass;

private:
    /** Visible grenade body (also the bounce collision). */
    UPROPERTY(VisibleAnywhere, Category = "GTC|Throwable", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere, Category = "GTC|Throwable", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, Category = "GTC|Throwable", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UProjectileMovementComponent> Movement;

    /** Fuse seconds remaining once armed; detonates at 0. */
    double FuseRemaining = 0.0;
    bool bArmed = false;
    bool bDetonated = false;

    /** Resolve the blast: gather targets in radius, apply damage + Z-biased knockback. */
    void Detonate();
};
