// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DeliveryRun.h"         // FDeliveryRun — same folder
#include "DeliveryRunResolver.h" // FDeliveryRunResolver
#include "GTCDeliveryRunComponent.generated.h"

class AActor;

/** Blueprint mirror of FDeliveryRun::EState (kept in 1:1 order so a static_cast bridges the seam). */
UENUM(BlueprintType)
enum class EGTCDeliveryState : uint8
{
    InProgress,
    Delivered,
    TooSlow,
    Wrecked,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGTCDeliveryCompleted, float, PayoutFactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGTCDeliveryFailed, EGTCDeliveryState, Reason);

/**
 * UGTCDeliveryRunComponent — the live courier-mission adapter. It runs the orphan pure-core
 * FDeliveryRun (timed run: countdown + cargo condition + route progress -> scored payout) from
 * actual world state, the thing DeliveryRun.h calls its "DEFERRED adapter". Put it on the
 * courier (the player pawn or the delivery vehicle) — or on a giver and point Courier at the
 * vehicle — call StartRun(), and each tick it measures the courier's progress toward the drop
 * (via the pure FDeliveryRunResolver's monotonic high-water route math) and folds any crash
 * impulse into cargo damage, then publishes progress / time / cargo / payout for the HUD and
 * fires OnDeliveryCompleted / OnDeliveryFailed.
 *
 * It matches the codebase's adapter pattern (pure resolver + thin UObject shell, like
 * UGTCVehicleHandlingComponent): all the measurement math is in FDeliveryRunResolver, this
 * shell only gathers positions and publishes results. It is a plain UActorComponent — NO new
 * subsystem, no ChaosVehicles dependency — and reports OUT through Blueprint-assignable events
 * so the mission wiring stays decoupled: bind OnDeliveryCompleted -> UMissionController::Complete
 * and OnDeliveryFailed -> UMissionController::Fail (or straight to the HUD / reward payout) in the
 * car / mission BP. Composes with the driving slice: a delivery in the rain is harder to keep
 * clean (see UGTCVehicleHandlingComponent's wet-road grip).
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UGTCDeliveryRunComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTCDeliveryRunComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // --- Config --------------------------------------------------------------------

    /** World location of the drop-off. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Delivery")
    FVector DropoffLocation = FVector::ZeroVector;

    /** Seconds allowed to reach the drop. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Delivery", meta = (ClampMin = "1.0"))
    float TimeLimitSeconds = 120.0f;

    /** Cargo condition (0..1) lost per unit of impact impulse fed to NotifyCargoImpact. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Delivery", meta = (ClampMin = "0.0"))
    float DamagePerImpulse = 0.0002f;

    /** Distance (cm) from the drop that counts as arrived — the courier delivers once this close,
     *  rather than needing to sit exactly on the marker. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Delivery", meta = (ClampMin = "0.0"))
    float ArrivalRadius = 500.0f;

    /** The actor whose position is the courier. Null => this component's owner. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Delivery")
    TObjectPtr<AActor> Courier = nullptr;

    // --- Control -------------------------------------------------------------------

    /** Begin a run to DropoffLocation, measuring the route from the courier's current position. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Delivery")
    void StartRun();

    /** Begin a run to an explicit drop with an explicit time limit. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Delivery")
    void StartRunTo(const FVector& Dropoff, float TimeLimit);

    /** Feed a collision impulse this frame (bind to the courier's OnHit). Erodes the cargo. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Delivery")
    void NotifyCargoImpact(float Impulse) { PendingImpulse += FMath::Max(0.0f, Impulse); }

    /** Abort the current run without success or failure (no events fire). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Delivery")
    void CancelRun() { bActive = false; }

    /** True while a run is active and unresolved. */
    UFUNCTION(BlueprintPure, Category = "GTC|Delivery")
    bool IsRunActive() const { return bActive; }

    // --- Published state (updated each active tick) --------------------------------

    /** Route covered so far, 0..1 (monotonic — never falls). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Delivery")
    float Progress = 0.0f;

    /** Seconds left on the clock. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Delivery")
    float TimeLeftSeconds = 0.0f;

    /** Cargo condition, 0..1 (0 = wrecked). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Delivery")
    float CargoCondition = 1.0f;

    /** Reward MULTIPLIER a delivery would pay right now, 0..1 (0 unless delivered). This is a
     *  quality factor, not a cash amount — feed it into the SideJob / MissionReward cash economy
     *  (e.g. basePayout * PayoutFactor), don't treat it as a standalone reward. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Delivery")
    float PayoutFactor = 0.0f;

    /** Straight-line distance from the courier to the drop (for a HUD "distance to drop"). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Delivery")
    float DistanceToDropoff = 0.0f;

    /** Current run state (mirror of the core). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Delivery")
    EGTCDeliveryState DeliveryState = EGTCDeliveryState::InProgress;

    // --- Events (bind to UMissionController::Complete/Fail or the HUD/rewards) ------

    /** Fired once when the cargo is delivered; carries the 0..1 payout factor. */
    UPROPERTY(BlueprintAssignable, Category = "GTC|Delivery")
    FGTCDeliveryCompleted OnDeliveryCompleted;

    /** Fired once on a failed run; carries the reason (TooSlow or Wrecked). */
    UPROPERTY(BlueprintAssignable, Category = "GTC|Delivery")
    FGTCDeliveryFailed OnDeliveryFailed;

private:
    /** The tracked courier's world location (Courier if set, else the owner). */
    FVector CourierLocation() const;

    /** Copy the core's state onto the published UPROPERTYs (DistToDrop already computed by the caller). */
    void Publish(double DistToDrop);

    /** The pure delivery core + its stored run state. */
    FDeliveryRun Run;
    double RouteLength = 0.0;
    double HighWaterProgress = 0.0;
    float PendingImpulse = 0.0f;
    bool bActive = false;
};
