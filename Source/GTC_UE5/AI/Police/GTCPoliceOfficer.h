// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../../NPC/Vitals/NpcVitals.h"
#include "../PoliceDispatch/PoliceEscalation.h"
#include "../Combat/GTCStunnable.h"
#include "GTCPoliceOfficer.generated.h"

class UGTCWeaponComponent;
class AGTCPickup;
class AGTCThrowable;
class AGTCHostile;

/**
 * APoliceOfficer — the Wave-3 actor that EMBODIES the police gunfight the whole
 * wanted->dispatch->combat pure-core stack already computes but never enacted.
 * It is the combat sibling of AGTCCitizen: a thin ACharacter shell that, each
 * frame, reads the live wanted level, runs the tested decision helpers, and walks
 * the result out into the world (move / face / fire / take damage / die).
 *
 * What it OWNS (the mutable per-officer state the pure core expects the node to
 * hold): a UGTCWeaponComponent arsenal, a fire cooldown timer, a stable strafe
 * side, an FNpcVitals life pool. What it READS (never mutates): the player pawn
 * as its target, and UWantedSubsystem::Stars() for escalation. What it DELEGATES
 * (the actual decisions, all headless-tested elsewhere):
 *   - FPoliceCombat::Plan(...)   -> advance / engage / reposition / cover / retreat + whether to fire
 *   - FCombatAi::DesiredMove/MoveSpeed -> the per-action movement intent
 *   - FPoliceCombat::FireCooldown(Stars) -> heat-scaled trigger cadence
 *
 * FRAME REMAP: the pure-core helpers are written in the Godot planar frame (XZ,
 * Y ignored, metres); Unreal is Z-up centimetres. This adapter converts at the
 * boundary (Unreal XY <-> core XZ, cm <-> m) so the tested math is fed exactly
 * what it expects. Damage arrives through the standard engine TakeDamage path, so
 * the player's existing UGTCWeaponComponent hitscan drops officers with no extra
 * wiring; on death/wound the officer reports the crime to the wanted system,
 * closing the escalation loop (shooting cops raises heat).
 *
 * Spawning/retiring officers around the player is AGTCPoliceDirector's job; this
 * class is the embodied endpoint, not the spawner.
 */
UCLASS()
class GTC_UE5_API AGTCPoliceOfficer : public ACharacter, public IGTCStunnable
{
    GENERATED_BODY()

public:
    AGTCPoliceOfficer();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    /**
     * Stamp this officer's loadout/toughness from its dispatched unit type and a
     * deterministic seed (strafe side, spread RNG). The director calls this on the
     * deferred-spawned actor before FinishSpawning; BeginPlay applies a default if
     * it never ran, so a hand-placed officer still works. Idempotent.
     */
    void InitializeUnit(EPoliceUnitType InUnitType, int32 Seed);

    /** IGTCStunnable: a flashbang freezes this officer's combat for `Seconds`. */
    virtual void Stun(float Seconds) override;

    /** Unit type this officer embodies (beat cop / cruiser crew / SWAT / military). */
    EPoliceUnitType GetUnitType() const { return UnitType; }

    /** True once killed — the body is ragdolling and counting down to despawn. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Police")
    bool IsDead() const { return bDead; }

    /** True while flashbanged (holding fire + frozen) — excluded from bust/seen checks. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Police")
    bool IsStunned() const { return StunTimer > 0.0; }

    /** Current health as a 0..1 fraction (1 = unhurt), for a HUD/debug overlay. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Police")
    float GetHealthFraction() const;

    /**
     * Engine damage entry point. The player's weapon lands a hitscan and calls
     * UGameplayStatics::ApplyPointDamage, which routes here: draw down vitals, then
     * either flinch or die. Honours the standard TakeDamage contract.
     */
    virtual float TakeDamage(
        float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    /**
     * Fired when a non-lethal round lands — the seam for a pain/recoil montage.
     * `Severity` is 0..1; `FromDirection` is the planar world direction the shot
     * came from. The C++ has already staggered the body; the montage is optional.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|Police|Combat")
    void OnGunshotWound(float Severity, FVector FromDirection);

    /**
     * Fired once when this officer is killed — the seam for a death montage/FX
     * before the body ragdolls. `FromDirection` is the planar world direction the
     * killing shot came from. The C++ has already entered the death state.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|Police|Combat")
    void OnKilled(FVector FromDirection);

protected:
    // --- Tuning -----------------------------------------------------------------

    /** Base run speed (cm/s); the engagement action scales this down for strafing. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police")
    double RunSpeed = 540.0; // must exceed the player's ~450 cm/s flat sprint even after
                             // the low-aggression chase multiplier, or cops never close.

    /** Seconds a killed body lingers, ragdolled, before it despawns. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police")
    double CorpseLingerSeconds = 14.0;

    /** Armor pickup a downed officer may drop (loot). Defaults to AGTCPickup. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police")
    TSubclassOf<AGTCPickup> LootPickupClass;

    /** Chance [0..1] a killed officer drops loot (SWAT-grade units always do). */
    UPROPERTY(EditAnywhere, Category = "GTC|Police", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    double LootDropChance = 0.5;

    /** Frag a SWAT officer lobs to flush the player from cover. Defaults to AGTCThrowable. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police")
    TSubclassOf<AGTCThrowable> GrenadeClass;

    /** Seconds between a SWAT officer's grenade throws. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police")
    double GrenadeIntervalSec = 8.0;

    /** Chance [0..1] a SWAT unit carries a riot shield (decided at init). */
    UPROPERTY(EditAnywhere, Category = "GTC|Police", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    double ShieldChance = 0.34;

    /** Fraction of a FRONTAL hit a riot shield absorbs — flank to bypass it. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    double ShieldFrontReduction = 0.85;

    /** Wound severity (0..1) at/above which a non-lethal round visibly staggers the officer. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    double StaggerWoundSeverity = 0.35;

    /** Yaw turn rate (deg/s) the officer rotates to keep the player covered. */
    UPROPERTY(EditAnywhere, Category = "GTC|Police")
    double AimTurnRateDeg = 540.0;

private:
    /** The officer's gun — reused player weapon component, fired via aim override. */
    UPROPERTY(VisibleAnywhere, Category = "GTC|Police", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UGTCWeaponComponent> Weapon;

    EPoliceUnitType UnitType = EPoliceUnitType::BeatCop;

    /** Life pool; seeded by unit type in InitializeUnit. */
    FNpcVitals Vitals;
    bool bDead = false;
    bool bInitialized = false;

    /** Riot-shield SWAT: soaks most frontal damage so the player must flank. */
    bool bShielded = false;

    /** Per-officer trigger timer; a shot is allowed only once it elapses. */
    double FireTimer = 0.0;
    /** Stable circling side (+1/-1) for Reposition strafing, chosen from the seed. */
    double StrafeSign = 1.0;
    FRandomStream Rng;

    /** Cached wanted stars, refreshed each tick from the live subsystem. */
    int32 CachedStars = 0;

    /** True while the player is actively committing crimes (recent ReportCrime) — the
     *  apprehend gate's "armed/dangerous suspect" signal. */
    bool bCachedPlayerDangerous = false;

    /** Flashbang stun: while > 0 the officer holds position and holds fire. */
    double StunTimer = 0.0;

    /** Chosen cover spot while TakeCover is active, and whether one is held. */
    FVector CoverPos = FVector::ZeroVector;
    bool bHasCover = false;
    /** Throttle on re-searching for cover (search is a handful of traces). */
    double CoverSearchTimer = 0.0;
    /** Peek/tuck cycle while holding cover: lean out to shoot, then duck back. */
    bool bPeeking = false;
    double PeekTimer = 0.0;

    /** Suppression (0..1): builds as rounds land, decays when fire lets up; high
     *  suppression makes the officer seek cover sooner and fire slower (FSuppression). */
    double Suppression = 0.0;

    /** Seconds the player has been un-wanted; after a grace period the officer goes
     *  home (also retires chopper-dropped officers the director doesn't pool). */
    double StandDownTimer = 0.0;

    /** Cooldown on this officer's grenade throws (SWAT units only). */
    double GrenadeCooldown = 0.0;

    /** Pursuit memory: the player's last-seen spot + how long out of sight, so a
     *  broken sightline sends the officer to where it last saw you, not to your
     *  true position (FPursuitMemory). */
    FVector LastKnownPos = FVector::ZeroVector;
    double TimeUnseen = 0.0;
    bool bHasLastKnown = false;

    // --- Pipeline ---------------------------------------------------------------

    /** The player pawn this officer hunts, or null. */
    APawn* ResolveTarget() const;

    /** Clear line of fire from the officer's eyes to the target chest? */
    bool HasLineOfSight(const APawn* Target) const;

    /** Find a nearby spot whose line to `ThreatPos` is blocked by geometry (real
     *  cover), scored via FCombatCover. `Target` is only the trace's ignore-actor; the
     *  cover is scored against ThreatPos (the same point combat aims at — the live
     *  player when seen, the last-known spot when not). False if none found. */
    bool FindCover(const APawn* Target, const FVector& ThreatPos, FVector& OutCover) const;

    /** Turn smoothly to keep the target in front (so aim + firing arc line up). */
    void FaceTarget(const FVector& TargetPos, float DeltaSeconds);

    /** Run the tested combat decision and drive movement + firing for this tick.
     *  `TargetPos` is the combat target (live when seen, last-known when not, or a
     *  gang member when we've lost the player and one is on top of us); `bLos` is the
     *  sightline to it; `bFightingHostile` suppresses the (player-only) apprehend. */
    void DriveCombat(
        float DeltaSeconds, APawn* Target, const FVector& TargetPos, bool bLos, bool bFightingHostile);

    /** Nearest living gang member within `RangeCm`, or null — a self-defense target
     *  used only when the player is out of sight. */
    AGTCHostile* FindNearbyHostile(double RangeCm) const;

    /** Route a resolved hit: draw down vitals, then flinch or die. `BulletTravel`
     *  is the planar world direction the round was travelling; bByPlayer raises heat. */
    void ApplyGunshot(double Damage, const FVector& BulletTravel, bool bByPlayer);

    /** Enter the death state: stop firing/AI, ragdoll, despawn countdown, report the
     *  kill to the wanted system when the player did it. */
    void EnterDeath(const FVector& BulletTravel, bool bByPlayer);

    /** Raise heat on the wanted system for a crime against this officer. */
    void ReportCrimeToWanted(bool bKilled) const;
};
