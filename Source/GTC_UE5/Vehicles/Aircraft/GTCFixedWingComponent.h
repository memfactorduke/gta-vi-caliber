// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FixedWingFlight.h"          // FFixedWingFlight
#include "FixedWingFlightResolver.h"  // FFixedWingFlightResolver
#include "GTCFixedWingComponent.generated.h"

class AActor;

/**
 * UGTCFixedWingComponent — the movement adapter that makes a plane fly. FFixedWingFlight is the pure
 * arcade model (throttle/elevator/aileron -> a world velocity + heading, whose defining mechanic is
 * the STALL: lift comes from airspeed, so fly too slow and the wings quit) and defers "reading the
 * sticks, integrating Velocity into a world position, gear/terrain collision" to its adapter; this
 * is that adapter.
 *
 * Each flying tick it Update()s the model from the stored controls, integrates the velocity into a
 * new world position with a ground floor (the pure FFixedWingFlightResolver), moves the owning
 * (kinematic) actor there facing the flight heading, and publishes airspeed / stall / altitude for
 * the cockpit HUD. NO Chaos, no new subsystem. Feed the three axes from Enhanced Input via
 * SetControls, StartFlight when the pilot takes the seat, and (optionally) SetGroundHeight from a
 * downward trace.
 *
 * Note: heading starts at 0 (facing +X) on StartFlight, so the actor's yaw snaps to +X on take-off
 * — spawn/possess the plane facing +X, or add a heading offset in BP. Unlike a helicopter this has
 * no strafe axis (aileron turns the heading), and increasing heading is a right turn in UE, so the
 * controls are UE-correct with no sign flip.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UGTCFixedWingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTCFixedWingComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // --- Per-frame input the pilot BP feeds from Enhanced Input --------------------

    /** Feed this frame's controls: throttle 0..1 (airspeed), elevator -1..1 (positive = nose up =
     *  climb, costing airspeed), aileron -1..1 (bank to turn; positive banks/turns right). */
    UFUNCTION(BlueprintCallable, Category = "GTC|FixedWing")
    void SetControls(float Throttle, float Elevator, float Aileron);

    /** Ground height (world Z) directly under the plane — feed a downward trace for terrain, else it
     *  uses the flat WorldFloorZ. */
    UFUNCTION(BlueprintCallable, Category = "GTC|FixedWing")
    void SetGroundHeight(float InGroundZ)
    {
        GroundZ = InGroundZ;
    }

    // --- Control -------------------------------------------------------------------

    /** Start flying (installs the tuning + resets the model to rest, stalled at heading 0). */
    UFUNCTION(BlueprintCallable, Category = "GTC|FixedWing")
    void StartFlight();

    /** Stop flying (the component stops moving the actor). */
    UFUNCTION(BlueprintCallable, Category = "GTC|FixedWing")
    void StopFlight();

    UFUNCTION(BlueprintPure, Category = "GTC|FixedWing")
    bool IsFlying() const { return bFlying; }

    // --- Airframe tuning (maps to FFixedWingFlight::FParams) -----------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|FixedWing", meta = (ClampMin = "0.0"))
    float MaxThrustAccel = 500.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|FixedWing", meta = (ClampMin = "0.0"))
    float Drag = 0.2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|FixedWing", meta = (ClampMin = "0.0"))
    float StallSpeed = 800.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|FixedWing", meta = (ClampMin = "0.0"))
    float StallSinkRate = 600.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|FixedWing", meta = (ClampMin = "0.0"))
    float CruiseSpeed = 1500.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|FixedWing", meta = (ClampMin = "0.0"))
    float ClimbAuthority = 700.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|FixedWing", meta = (ClampMin = "0.0"))
    float ClimbAirspeedCost = 300.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|FixedWing", meta = (ClampMin = "0.0"))
    float TurnAuthority = 0.8f;

    /** Flat world floor (world Z) used when SetGroundHeight isn't fed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|FixedWing")
    float WorldFloorZ = 0.0f;

    /** The plane actor to fly. Null => this component's owner. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|FixedWing")
    TObjectPtr<AActor> Aircraft = nullptr;

    // --- Published telemetry (for the cockpit HUD) ---------------------------------

    /** Forward airspeed (cm/s). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|FixedWing")
    float Airspeed = 0.0f;

    /** Height above the ground under the plane (cm). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|FixedWing")
    float AltitudeAgl = 0.0f;

    /** Vertical speed (cm/s, +up). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|FixedWing")
    float ClimbRate = 0.0f;

    /** Horizontal speed (cm/s). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|FixedWing")
    float GroundSpeed = 0.0f;

    /** Heading (degrees, 0 = +X). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|FixedWing")
    float HeadingDegrees = 0.0f;

    /** Airspeed cushion over stall, 0..1 (0 = at/below stall) — drive a stall-warning gauge. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|FixedWing")
    float StallMargin = 0.0f;

    /** True when the wings are stalled (airspeed below StallSpeed). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|FixedWing")
    bool bStalled = false;

    /** True while the plane is on the ground. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|FixedWing")
    bool bGrounded = false;

private:
    /** Build the airframe tuning from the editable properties. */
    FFixedWingFlight::FParams FlightParams() const;

    /** The plane actor (Aircraft if set, else the owner). */
    AActor* ResolveAircraft() const;

    /** The pure flight model (controls -> velocity + heading). */
    FFixedWingFlight Flight;

    // Control axes fed by the BP each frame.
    float RawThrottle = 0.0f;
    float RawElevator = 0.0f;
    float RawAileron = 0.0f;

    /** Ground Z under the plane (SetGroundHeight, else WorldFloorZ at StartFlight). */
    double GroundZ = 0.0;

    /** Stall speed snapshotted at StartFlight — matches what the flight model runs on, so the HUD
     *  stall telemetry can't desync from the physics if the property is edited mid-flight. */
    double StallSpeedActive = 0.0;

    bool bFlying = false;
};
