// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HelicopterFlight.h"         // FHelicopterFlight
#include "HelicopterFlightResolver.h" // FHelicopterFlightResolver
#include "GTCHelicopterComponent.generated.h"

class AActor;

/**
 * UGTCHelicopterComponent — the movement adapter that makes the police chopper (and any flyable
 * helicopter) actually fly. FHelicopterFlight is the pure rotor model (four controls -> a world
 * velocity + heading) and explicitly defers "reading the sticks, integrating Velocity into a world
 * position, ground/obstacle collision" to its adapter; this is that adapter.
 *
 * Each tick, while flying, it Update()s the flight model from the stored control axes, integrates
 * the resulting velocity into a new world position with a ground floor (the pure
 * FHelicopterFlightResolver), and moves the owning actor there (a swept move for obstacle
 * collision) facing the flight heading — then publishes altitude / climb rate / ground speed for
 * the HUD. It's a KINEMATIC flyer: no Chaos, no new subsystem; the actor is a plain pawn the
 * component drives. Feed the four axes from Enhanced Input via SetControls, StartFlight when the
 * pilot takes the seat, and (optionally) SetGroundHeight from a downward trace under the chopper.
 *
 * Note: the flight model's heading starts at 0 (facing +X) on StartFlight, so the actor snaps to
 * face +X as it lifts off — spawn/possess the chopper facing +X, or add a heading offset in BP.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UGTCHelicopterComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTCHelicopterComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // --- Per-frame input the pilot BP feeds from Enhanced Input --------------------

    /** Feed this frame's controls: collective 0..1 (lift), cyclic pitch/roll -1..1 (tilt), pedal
     *  -1..1 (yaw). Nose-down (negative pitch) accelerates forward; bank (positive roll) slides right. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Helicopter")
    void SetControls(float Collective, float CyclicPitch, float CyclicRoll, float Pedal);

    /** Ground height (world Z) directly under the chopper — feed a downward trace result for terrain,
     *  else it uses the flat WorldFloorZ. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Helicopter")
    void SetGroundHeight(float InGroundZ)
    {
        GroundZ = InGroundZ;
    }

    // --- Control -------------------------------------------------------------------

    /** Start flying (installs the tuning + resets the rotor model to rest). Call on take-the-seat. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Helicopter")
    void StartFlight();

    /** Stop flying (the component stops moving the actor). Call on leave-the-seat. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Helicopter")
    void StopFlight();

    UFUNCTION(BlueprintPure, Category = "GTC|Helicopter")
    bool IsFlying() const { return bFlying; }

    /** The hands-off hover collective (the setting where lift cancels gravity), 0..1. */
    UFUNCTION(BlueprintPure, Category = "GTC|Helicopter")
    float GetHoverCollective() const { return (float)Flight.HoverCollective(); }

    // --- Airframe tuning (maps to FHelicopterFlight::FParams) ----------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Helicopter", meta = (ClampMin = "0.0"))
    float Gravity = 980.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Helicopter", meta = (ClampMin = "0.0"))
    float MaxLiftAccel = 2000.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Helicopter", meta = (ClampMin = "0.0"))
    float TiltAccel = 1200.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Helicopter", meta = (ClampMin = "0.0"))
    float YawRatePerSec = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Helicopter", meta = (ClampMin = "0.0"))
    float LinearDrag = 0.5f;

    /** Flat world floor (world Z) used when SetGroundHeight isn't fed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Helicopter")
    float WorldFloorZ = 0.0f;

    /** The aircraft actor to fly. Null => this component's owner. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Helicopter")
    TObjectPtr<AActor> Aircraft = nullptr;

    // --- Published telemetry (for the HUD) -----------------------------------------

    /** Height above the ground under the chopper (cm). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Helicopter")
    float AltitudeAgl = 0.0f;

    /** Vertical speed (cm/s, +up). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Helicopter")
    float ClimbRate = 0.0f;

    /** Horizontal speed (cm/s). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Helicopter")
    float GroundSpeed = 0.0f;

    /** Heading (degrees, 0 = +X). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Helicopter")
    float HeadingDegrees = 0.0f;

    /** True while the chopper is on the ground. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Helicopter")
    bool bGrounded = false;

private:
    /** Build the rotor tuning from the editable properties. */
    FHelicopterFlight::FParams FlightParams() const;

    /** The aircraft actor (Aircraft if set, else the owner). */
    AActor* ResolveAircraft() const;

    /** The pure rotor model (controls -> velocity + heading). */
    FHelicopterFlight Flight;

    // Control axes fed by the BP each frame.
    float RawCollective = 0.0f;
    float RawPitch = 0.0f;
    float RawRoll = 0.0f;
    float RawPedal = 0.0f;

    /** Ground Z under the chopper (SetGroundHeight, else WorldFloorZ at StartFlight). */
    double GroundZ = 0.0;

    bool bFlying = false;
};
