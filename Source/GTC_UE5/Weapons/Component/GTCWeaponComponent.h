// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "Math/RandomStream.h"
#include "GTCWeaponComponent.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class USkeletalMeshComponent;
class UCameraComponent;
struct FWeaponFireController;
struct FWeaponStats;

/**
 * Broadcast each time a shot leaves the barrel, so Blueprints can drive the
 * cosmetic layer (muzzle flash, report audio, impact decal/particle, tracer).
 * MuzzleLocation is the visual shot origin; ImpactPoint is where the trace
 * landed (== trace end when bHit is false).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FGTCWeaponFiredSignature, FVector, MuzzleLocation, FVector, ImpactPoint, bool, bHit);

/** Broadcast when the equipped weapon changes (HUD weapon-name / ammo refresh). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGTCWeaponChangedSignature, const FString&, WeaponName);

/**
 * One row of weapon-wheel data: the display name, a one-line blurb for the wheel
 * hub ("Semi-auto · 12-round mag · 18 dmg"), live ammo, and whether the weapon is
 * automatic (so the UI can class-colour the slice). UI-agnostic — the player
 * controller maps this to the radial widget's FGTCRadialItem.
 */
struct FGTCWeaponWheelEntry
{
    FString Name;
    FString Blurb;
    int32 AmmoInMag = 0;
    int32 Reserve = 0;
    bool bAutomatic = false;
};

/**
 * UGTCWeaponComponent — the Wave-3 Unreal adapter that lets the player pawn HOLD
 * and SHOOT, GTA-style, on top of the pure-core weapon models.
 *
 * It owns an ordered arsenal of FWeaponFireController instances (each wrapping an
 * FWeapon ammo/cooldown/bloom state machine), attaches a placeholder weapon mesh
 * to the owning character's hand socket, and — when the trigger is held — ticks
 * the equipped weapon and converts each allowed shot into a camera-aimed line
 * trace. Spread, distance falloff and hit-zone multipliers come straight from
 * FWeaponBallistics; recoil kicks the controller pitch; damage is applied through
 * the engine TakeDamage path (UGameplayStatics::ApplyPointDamage).
 *
 * Distances in FWeaponStats are METRES (the Godot frame); this adapter converts
 * to Unreal centimetres for traces (x100) and back for the falloff math (/100).
 *
 * Headless-safe: the placeholder mesh is a soft reference (defaults to an engine
 * basic shape) and every world call is null-guarded, so the editor-closed build
 * and automation runs never hard-depend on game content. The trigger cadence and
 * ammo logic live in the unit-tested pure core; only the world trace / attach /
 * damage glue lives here (matching the codebase's pure-core + adapter split).
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UGTCWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTCWeaponComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(
        float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // --- Input-facing API (bound from the character) --------------------------

    /** Pull the trigger: fires immediately, and keeps firing if the weapon is automatic. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Weapon")
    void StartFire();

    /** Release the trigger. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Weapon")
    void StopFire();

    /** Reload the equipped weapon from its reserve ammo. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Weapon")
    void Reload();

    /** Equip the next weapon in the wheel (wraps). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Weapon")
    void EquipNext();

    /** Equip the previous weapon in the wheel (wraps). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Weapon")
    void EquipPrevious();

    /** Equip a weapon by wheel index. Returns false if out of range. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Weapon")
    bool EquipIndex(int32 Index);

    /** Give the standard pistol/SMG/rifle/shotgun arsenal and equip the pistol. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Weapon")
    void GiveDefaultArsenal();

    /** Replace the arsenal with a single named weapon (for an AI loadout). */
    void SetSingleWeapon(const FWeaponStats& Stats);

    // --- AI aim (camera-less owners: NPCs/police) -----------------------------

    /**
     * Aim along an explicit WORLD direction instead of an owner camera. Set by an
     * AI each tick to point its shots at a target; consumed by the firing path
     * ONLY when the owner has no follow camera (the player's camera always wins,
     * so this never perturbs player aim). The vector is normalised internally.
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|Weapon")
    void SetAimOverride(const FVector& WorldAimDir);

    /** Drop any AI aim override (fall back to the pawn's eyes view direction). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Weapon")
    void ClearAimOverride();

    // --- HUD queries ----------------------------------------------------------

    UFUNCTION(BlueprintPure, Category = "GTC|Weapon")
    FString CurrentWeaponName() const;

    UFUNCTION(BlueprintPure, Category = "GTC|Weapon")
    int32 CurrentAmmoInMag() const;

    UFUNCTION(BlueprintPure, Category = "GTC|Weapon")
    int32 CurrentReserveAmmo() const;

    UFUNCTION(BlueprintPure, Category = "GTC|Weapon")
    int32 WeaponCount() const;

    /** Snapshot of every owned weapon for the weapon wheel, in wheel order. */
    TArray<FGTCWeaponWheelEntry> WeaponWheelEntries() const;

    /** Index of the equipped weapon within WeaponWheelEntries(), or INDEX_NONE. */
    int32 EquippedWheelIndex() const { return EquippedIndex; }

    // --- Cosmetic / HUD events ------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "GTC|Weapon")
    FGTCWeaponFiredSignature OnWeaponFired;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Weapon")
    FGTCWeaponChangedSignature OnWeaponChanged;

    // --- Configuration --------------------------------------------------------

    /** Skeleton socket the weapon mesh attaches to (UE mannequin convention). */
    UPROPERTY(EditAnywhere, Category = "GTC|Weapon")
    FName AttachSocketName = TEXT("hand_r");

    /** Socket on the weapon mesh used as the visual muzzle origin, if present. */
    UPROPERTY(EditAnywhere, Category = "GTC|Weapon")
    FName MuzzleSocketName = TEXT("Muzzle");

    /** Trace channel for hit detection (defaults to Visibility). */
    UPROPERTY(EditAnywhere, Category = "GTC|Weapon")
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

    /** Draw a debug tracer + impact marker for each shot (editor/non-shipping). */
    UPROPERTY(EditAnywhere, Category = "GTC|Weapon")
    bool bDrawDebugShots = true;

    /** Show a faint muzzle->impact tracer streak on a fraction of shots. */
    UPROPERTY(EditAnywhere, Category = "GTC|Weapon|Tracer")
    bool bShowTracers = true;

    /** One tracer every Nth shot (1 = every shot). Keeps it reading as tracer rounds, not a beam. */
    UPROPERTY(EditAnywhere, Category = "GTC|Weapon|Tracer", meta = (ClampMin = "1"))
    int32 TracerEveryNthShot = 3;

    /** Tracer line thickness in centimetres — keep it thin so it stays subtle. */
    UPROPERTY(EditAnywhere, Category = "GTC|Weapon|Tracer", meta = (ClampMin = "0.1"))
    float TracerThicknessCm = 1.25f;

    /** How long a tracer streak lingers, in seconds — a brief blip. */
    UPROPERTY(EditAnywhere, Category = "GTC|Weapon|Tracer", meta = (ClampMin = "0.0"))
    float TracerLifeSeconds = 0.035f;

    /** Held weapon mesh; defaults to an engine basic shape so the gun is visible without content. */
    UPROPERTY(EditAnywhere, Category = "GTC|Weapon")
    TSoftObjectPtr<UStaticMesh> PlaceholderWeaponMesh;

protected:
    /** The visible weapon mesh attached to the character's hand. */
    UPROPERTY(VisibleAnywhere, Category = "GTC|Weapon")
    TObjectPtr<UStaticMeshComponent> WeaponMesh;

private:
    /** Ordered arsenal (wheel order); parallel to nothing — each holds its own id. */
    TArray<TSharedPtr<FWeaponFireController>> Arsenal;
    TArray<FString> WeaponNames;
    int32 EquippedIndex = INDEX_NONE;

    /** Deterministic-per-instance spread RNG. */
    FRandomStream SpreadRng;

    /** Rolling count of shots fired, for the tracer-every-Nth cadence. */
    int32 TracerShotCounter = 0;

    /** AI aim override: when set on a camera-less owner, shots fly along this world
     *  direction instead of the pawn's eyes view rotation. */
    bool bHasAimOverride = false;
    FVector AimOverrideDir = FVector::ForwardVector;

    FWeaponFireController* Equipped() const;

    /** Resolve the owner's follow camera (third-person aim source). */
    UCameraComponent* FindOwnerCamera() const;

    /** Attach (creating if needed) the weapon mesh to the owner's hand socket. */
    void EnsureWeaponMeshAttached();

    /** Refresh the held mesh + broadcast OnWeaponChanged for the current weapon. */
    void OnEquippedChanged();

    /** Run one allowed shot (trace, damage, recoil, cosmetics). No-op if the cadence blocks it. */
    void FireOneShot();

    /** Map a hit skeletal bone to a coarse body part for the hit-zone multiplier. */
    static FString BodyPartFromBone(FName BoneName);
};
