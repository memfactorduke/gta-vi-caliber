// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTCPickup.generated.h"

class UStaticMeshComponent;
class USphereComponent;

/** What a pickup restores when the player walks over it. */
UENUM(BlueprintType)
enum class EGTCPickupKind : uint8
{
    Health,
    Armor,
};

/**
 * AGTCPickup — the classic walk-over world pickup that tops the player back up
 * mid-fight: a health cross or an armor vest. It's the counterweight to all the
 * combat pressure the police layer adds — somewhere to recover so a firefight is
 * survivable. A trigger sphere watches for the player, applies the restore through
 * the existing UPlayerHealthComponent (Heal / AddArmor — no new player code), then
 * either despawns or hides and respawns on a timer like a GTA pickup.
 *
 * Thin AActor; all the health math already lives in FPlayerHealthModel behind the
 * component, so this only detects, applies, and recycles.
 */
UCLASS()
class GTC_UE5_API AGTCPickup : public AActor
{
    GENERATED_BODY()

public:
    AGTCPickup();

    virtual void Tick(float DeltaSeconds) override;

    /** Health or armor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Pickup")
    EGTCPickupKind Kind = EGTCPickupKind::Health;

    /** How much it restores (points of health or armor). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Pickup")
    float Amount = 25.0f;

    /** If true the pickup hides and returns after RespawnSeconds; else it's one-shot. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Pickup")
    bool bRespawns = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Pickup")
    float RespawnSeconds = 30.0f;

    /** Fired when the player collects it — the seam for a pickup sound/VFX. */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|Pickup")
    void OnCollected(APawn* By);

protected:
    UFUNCTION()
    void OnTriggerOverlap(
        UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
    UPROPERTY(VisibleAnywhere, Category = "GTC|Pickup", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USphereComponent> Trigger;

    UPROPERTY(VisibleAnywhere, Category = "GTC|Pickup", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> Mesh;

    bool bCollected = false;
    float RespawnTimer = 0.0f;
    float SpinAccum = 0.0f;

    /** Apply the restore to the player if it needs it; returns true if collected. */
    bool TryCollect(AActor* OtherActor);

    void SetActive(bool bActive);
};
