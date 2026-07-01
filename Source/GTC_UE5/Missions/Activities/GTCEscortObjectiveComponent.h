// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EscortObjective.h" // FEscortObjective — the by-value core member
#include "GTCEscortObjectiveComponent.generated.h"

class AActor;

/** Blueprint mirror of FEscortObjective::EState (1:1 order so a static_cast bridges the seam). */
UENUM(BlueprintType)
enum class EGTCEscortState : uint8
{
    Escorting,
    Delivered, // reached the destination with integrity left
    Lost,      // the escortee was destroyed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGTCEscortDelivered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGTCEscortLost);

/**
 * UGTCEscortObjectiveComponent — the live "protect the convoy" mission: keep the escortee alive to
 * the drop-off. It runs the orphan pure-core FEscortObjective (integrity bleeds in proportion to the
 * THREAT on the escortee, recovers once the attackers are suppressed, delivers on arrival, lost at
 * zero integrity) from actual world state — the "DEFERRED adapter" EscortObjective.h describes.
 *
 * Follows the project's pure-core + thin-shell split: FEscortResolver does the route-progress +
 * threat-from-hostiles geometry, this shell owns the FEscortObjective, feeds it each tick, publishes
 * integrity / progress / threat for the HUD, and fires OnEscortDelivered / OnEscortLost. Plain
 * UActorComponent — NO new subsystem, NO ChaosVehicles dep. Point Escortee at the convoy, set the
 * Destination, keep the Hostiles list current (the BP adds/removes attackers as they spawn/die), and
 * call StartEscort. It only computes while a live escort is running; a destroyed escortee is lost.
 * Rounds out the mission primitives alongside the tail and delivery activities.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UGTCEscortObjectiveComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTCEscortObjectiveComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // --- Who / where ---------------------------------------------------------------

    /** The convoy being protected (required). Its destruction (going null) loses the escort. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Escort")
    TObjectPtr<AActor> Escortee = nullptr;

    /** World location the escortee is headed to. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Escort")
    FVector Destination = FVector::ZeroVector;

    /** Live list of attacker actors; the BP keeps it current as hostiles spawn / die. Their
     *  proximity to the escortee is cooked into the threat level. Null entries are skipped. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Escort")
    TArray<TObjectPtr<AActor>> Hostiles;

    // --- Tuning --------------------------------------------------------------------

    /** Integrity (0..1) lost per second at full threat. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Escort", meta = (ClampMin = "0.0"))
    float DrainPerSec = 0.25f;

    /** Integrity recovered per second while the threat is suppressed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Escort", meta = (ClampMin = "0.0"))
    float RegenPerSec = 0.1f;

    /** Threat (0..1) at/below which the escortee is "safe" and starts recovering. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Escort", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RegenThreatThreshold = 0.05f;

    /** Distance (cm) from the destination that counts as arrived. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Escort", meta = (ClampMin = "0.0"))
    float ArrivalRadius = 500.0f;

    /** Hostiles beyond this distance (cm) from the escortee pose no threat. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Escort", meta = (ClampMin = "1.0"))
    float ThreatRadius = 3000.0f;

    /** Proximity-weighted point-blank attacker count that saturates the threat to 1. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Escort", meta = (ClampMin = "0.1"))
    float ThreatSaturation = 3.0f;

    // --- Control -------------------------------------------------------------------

    /** Begin the escort (measures the route from the escortee's current position to Destination). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Escort")
    void StartEscort();

    /** Abandon the escort without success/failure (no events fire). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Escort")
    void CancelEscort();

    /** True while an escort is running and unresolved. */
    UFUNCTION(BlueprintPure, Category = "GTC|Escort")
    bool IsEscortActive() const { return bActive; }

    // --- Published state (updated each active tick) --------------------------------

    /** Escortee integrity, 0..1 (0 = destroyed). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Escort")
    float Integrity = 1.0f;

    /** Route covered so far, 0..1 (monotonic). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Escort")
    float Progress = 0.0f;

    /** Current heat on the escortee, 0..1. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Escort")
    float ThreatLevel = 0.0f;

    /** Current escort state. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Escort")
    EGTCEscortState EscortState = EGTCEscortState::Escorting;

    // --- Events -------------------------------------------------------------------

    /** Fired once when the escortee reaches the destination alive. */
    UPROPERTY(BlueprintAssignable, Category = "GTC|Escort")
    FGTCEscortDelivered OnEscortDelivered;

    /** Fired once when the escortee is destroyed. */
    UPROPERTY(BlueprintAssignable, Category = "GTC|Escort")
    FGTCEscortLost OnEscortLost;

private:
    /** Copy the core's state (+ the tick's threat) onto the published UPROPERTYs. */
    void Publish(double Threat);

    /** Resolve to Lost, fire the event, and stop (used on integrity-0 or a destroyed escortee). */
    void ResolveLost();

    /** The pure objective + its run state. */
    FEscortObjective Escort;

    /** Route length (measured when the first live escortee is seen) + high-water progress. */
    double RouteLength = 0.0;
    double HighWaterProgress = 0.0;

    // Resolver geometry snapshotted at StartEscort, so a mid-escort edit to the UPROPERTYs can't
    // retune progress/threat under way (and ArrivalRadius stays consistent with the frozen route).
    double ArrivalRadiusSnapshot = 0.0;
    double ThreatRadiusSnapshot = 0.0;
    double ThreatSaturationSnapshot = 0.0;

    bool bActive = false;
    bool bHadEscortee = false;
};
