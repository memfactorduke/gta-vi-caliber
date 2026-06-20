// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../../NPC/Vitals/NpcVitals.h"
#include "../../Weapons/Melee/MeleeCombat.h"
#include "Math/RandomStream.h"
#include "GTCHostile.generated.h"

class UGTCWeaponComponent;
class AGTCPickup;
class AGTCThrowable;

/**
 * AGTCHostile — an armed NON-police enemy (a gang member / street thug) so that
 * "enemies that fight back" isn't only the law. It is a lean cousin of
 * AGTCPoliceOfficer: the same tested FCombatAi decision core and the reused
 * UGTCWeaponComponent, but with none of the police apparatus — no wanted-system
 * coupling, no dispatch, no cover/peek/pursuit depth. It simply hunts and shoots
 * its target (the player by default), takes the player's bullets, and dies.
 *
 * Spawned by AGTCGangSpawner (a territory the gang holds). Thin ACharacter shell;
 * the gunfight math lives in FCombatAi. Frame note: FCombatAi is the Godot planar
 * frame (XZ, metres) — remapped to Unreal (XY/Z-up, cm) at the boundary, same as
 * the officer.
 */
UCLASS()
class GTC_UE5_API AGTCHostile : public ACharacter
{
    GENERATED_BODY()

public:
    AGTCHostile();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    virtual float TakeDamage(
        float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    /** Stamp toughness + strafe side before BeginPlay. `bMeleeVariant` makes an
     *  unarmed thug that charges and strikes instead of shooting. */
    void InitializeHostile(int32 Seed, bool bMeleeVariant = false);

    UFUNCTION(BlueprintCallable, Category = "GTC|Hostile")
    bool IsDead() const { return bDead; }

    /** Fired once when killed — seam for the death FX. Body already ragdolling. */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|Hostile")
    void OnKilled(const FVector& FromDirection);

    /** Fired on a non-lethal hit — seam for a flinch/blood FX. */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|Hostile")
    void OnGunshotWound(float Severity, const FVector& FromDirection);

protected:
    UPROPERTY(EditAnywhere, Category = "GTC|Hostile")
    double MaxHealth = 80.0;

    UPROPERTY(EditAnywhere, Category = "GTC|Hostile")
    double RunSpeed = 440.0;

    /** Preferred engagement range (metres) — the band centre. */
    UPROPERTY(EditAnywhere, Category = "GTC|Hostile")
    double PreferredRangeM = 11.0;

    /** Aggression [0..1]; high so a thug presses the attack rather than hiding. */
    UPROPERTY(EditAnywhere, Category = "GTC|Hostile", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    double Aggression = 0.9;

    /** Seconds between shots. */
    UPROPERTY(EditAnywhere, Category = "GTC|Hostile")
    double FireCooldownSec = 0.55;

    /** Police within this range (metres) become targets too — gang-vs-cop fights.
     *  The player is always a candidate regardless of range. */
    UPROPERTY(EditAnywhere, Category = "GTC|Hostile")
    double TargetSearchRangeM = 40.0;

    UPROPERTY(EditAnywhere, Category = "GTC|Hostile")
    double CorpseLingerSeconds = 14.0;

    /** Melee reach (metres) at which an unarmed thug lands a strike. */
    UPROPERTY(EditAnywhere, Category = "GTC|Hostile")
    double MeleeRangeM = 2.0;

    /** Health pickup a downed thug may drop (loot). Defaults to AGTCPickup. */
    UPROPERTY(EditAnywhere, Category = "GTC|Hostile")
    TSubclassOf<AGTCPickup> LootPickupClass;

    /** Chance [0..1] a killed thug drops a health pickup. */
    UPROPERTY(EditAnywhere, Category = "GTC|Hostile", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    double LootDropChance = 0.4;

    /** Molotov a thrower lobs to ignite the player's cover. Defaults to AGTCThrowable. */
    UPROPERTY(EditAnywhere, Category = "GTC|Hostile")
    TSubclassOf<AGTCThrowable> MolotovClass;

    UPROPERTY(EditAnywhere, Category = "GTC|Hostile")
    double MolotovIntervalSec = 9.0;

    /** Chance [0..1] a ranged thug is a molotov-thrower (decided at init). */
    UPROPERTY(EditAnywhere, Category = "GTC|Hostile", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    double MolotovChance = 0.4;

    /** Chance [0..1] a ranged thug is a long-range marksman (slow, high-damage). */
    UPROPERTY(EditAnywhere, Category = "GTC|Hostile", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    double SniperChance = 0.2;

    /** A marksman's preferred engagement range (metres) and shot interval. */
    UPROPERTY(EditAnywhere, Category = "GTC|Hostile")
    double SniperRangeM = 45.0;

    UPROPERTY(EditAnywhere, Category = "GTC|Hostile")
    double SniperFireCooldownSec = 1.6;

private:
    UPROPERTY(VisibleAnywhere, Category = "GTC|Hostile", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UGTCWeaponComponent> Weapon;

    FNpcVitals Vitals;
    bool bDead = false;
    double FireTimer = 0.0;
    double StrafeSign = 1.0;
    FRandomStream Rng;

    /** Suppression (0..1): sustained fire pins the thug — seek cover sooner, fire
     *  slower (FSuppression), same as the police. */
    double Suppression = 0.0;

    /** Melee-thug mode: unarmed, charges to contact and strikes (FMeleeCombat). */
    bool bMelee = false;
    double MeleeStrikeCooldown = 0.0;
    FMeleeCombat::FMeleeFighter Fighter;

    /** Molotov-thrower (a fraction of ranged thugs, decided at init). */
    bool bThrowsMolotov = false;
    double MolotovCooldown = 0.0;

    /** Long-range marksman variant (mutually exclusive with thrower; decided at init). */
    bool bSniper = false;

    /** Cover the thug holds while TakeCover is active (FCombatCover). */
    FVector CoverPos = FVector::ZeroVector;
    bool bHasCover = false;
    double CoverSearchTimer = 0.0;

    /** Cached current target + the throttle on re-scanning for the nearest threat. */
    TWeakObjectPtr<APawn> CurrentTarget;
    double RetargetTimer = 0.0;

    /** Nearest threat: the player, or a closer police officer within search range. */
    APawn* PickTarget() const;
    bool HasLineOfSight(const APawn* Target) const;
    void FaceTarget(const FVector& TargetPos, float DeltaSeconds);

    /** Nearest spot whose line to the target is geometry-blocked (real cover). */
    bool FindCover(const APawn* Target, FVector& OutCover) const;
    void EnterDeath(const FVector& BulletTravel);
};
