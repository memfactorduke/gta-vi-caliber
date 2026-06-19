// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h" // IGTCInteractable contract this door fulfils
#include "Door.generated.h"

class UStaticMeshComponent;

/**
 * FDoorSwing — the pure, scene-free swing math for a hinged door. Extracted as a
 * plain non-UObject helper (the project's "testable logic lives off the actor"
 * convention, mirroring FInteraction) so it unit-tests headless
 * (Tests/DoorSwingTest.cpp). AGTCDoor is the thin UE adapter that drives a
 * UStaticMeshComponent's relative rotation from these results each tick.
 *
 * The door's state is a single open fraction in [0,1]: 0 = shut, 1 = fully open.
 * Advance() steps that fraction toward its target at a constant rate; YawDegrees()
 * maps it to an eased hinge angle for display. No UE types beyond float — double
 * precision is unnecessary here (angles/time, not world-space LWC distances).
 */
class GTC_UE5_API FDoorSwing
{
public:
    /**
     * Next open fraction, stepping `Frac` toward 1 when `bOpening` (else toward 0)
     * at a constant rate that covers the full 0..1 travel in `DurationSeconds`.
     * The result is clamped to [0,1]; a non-positive duration (or DeltaSeconds)
     * that would over/undershoot snaps straight to the target. Monotonic: opening
     * never decreases the fraction, closing never increases it.
     */
    static float Advance(float Frac, float DeltaSeconds, bool bOpening, float DurationSeconds);

    /**
     * Eased hinge angle (degrees) for an open fraction: 0 at Frac<=0, OpenAngleDeg
     * at Frac>=1, smoothstep-eased between so the leaf accelerates out of the jamb
     * and settles into the stop rather than moving linearly.
     */
    static float YawDegrees(float Frac, float OpenAngleDeg);
};

/**
 * AGTCDoor — a press-E openable hinged door. Implements IGTCInteractable, so the
 * player's UGTCInteractionComponent gathers it via its forward overlap, shows its
 * prompt, and on IA_Interact calls Interact() to toggle it open/shut. The leaf
 * swings about its hinge edge over OpenDurationSeconds using FDoorSwing; the actor
 * only ticks while it is actually moving.
 *
 * Coexists with the overlap-based BP_DoorFrame (the walk-in-interiors auto-doors) —
 * this is the explicit press-to-open variant. GTC-prefixed to stay clear of any
 * engine ADoor-style type, per the repo's interactable naming convention.
 */
UCLASS()
class GTC_UE5_API AGTCDoor : public AActor, public IGTCInteractable
{
    GENERATED_BODY()

public:
    AGTCDoor();

    // --- IGTCInteractable ---------------------------------------------------
    /** Toggle the door between open and shut for the interacting actor. */
    virtual void Interact_Implementation(AActor* Instigator) override;
    /** Interactable unless locked. */
    virtual bool CanInteract_Implementation(const AActor* Instigator) const override;
    /** "Press E to open" / "Press E to close" depending on current state. */
    virtual FText GetInteractPrompt_Implementation() const override;

    virtual void Tick(float DeltaSeconds) override;

    /** Hinge angle the leaf swings to when fully open (degrees). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Door")
    float OpenAngleDeg = 90.0f;

    /** Seconds for a full open or close sweep. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Door")
    float OpenDurationSeconds = 0.8f;

    /** When true the door starts (and stays) locked — CanInteract returns false. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Door")
    bool bStartsLocked = false;

    /**
     * Leaf offset from the hinge (cm). Zero suits a mesh whose pivot already sits
     * at the hinge edge (e.g. the prototyping SM_Door). For a centre-pivot mesh,
     * push it half its width along +Y so it swings about the jamb, not its middle.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Door")
    FVector PanelOffset = FVector::ZeroVector;

protected:
    virtual void BeginPlay() override;

private:
    /** Re-enable tick while the leaf is sweeping; it disables itself once settled. */
    void StartSweeping();

    /** Stable actor origin — keeps the actor/collision transform put while the
     *  Hinge child rotates. */
    UPROPERTY(VisibleAnywhere, Category = "GTC|Door")
    TObjectPtr<USceneComponent> Root;

    /** Hinge pivot child of Root — this is what yaws; the leaf hangs off it. */
    UPROPERTY(VisibleAnywhere, Category = "GTC|Door")
    TObjectPtr<USceneComponent> Hinge;

    /** The swinging door leaf (child of Hinge, placed at PanelOffset). */
    UPROPERTY(VisibleAnywhere, Category = "GTC|Door")
    TObjectPtr<UStaticMeshComponent> DoorPanel;

    /** Target state: true = opening/open, false = closing/shut. */
    bool bIsOpen = false;

    /** Locked gate (seeded from bStartsLocked on BeginPlay). */
    bool bLocked = false;

    /** Current open fraction in [0,1] driven by FDoorSwing. */
    float CurrentFrac = 0.0f;
};
