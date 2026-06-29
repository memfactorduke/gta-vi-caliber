// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GTCVehicleLocomotionComponent.generated.h"

/**
 * The per-frame control inputs a vehicle reads, normalized so one pawn can drive a
 * helicopter, a plane or a boat — each locomotion strategy interprets only the fields its
 * vehicle uses. Filled by the pawn's EnhancedInput handlers each frame.
 */
USTRUCT(BlueprintType)
struct FVehicleControlState
{
    GENERATED_BODY()

    /** Forward drive: boat throttle (-1..1, reverse) / plane throttle change. */
    UPROPERTY(BlueprintReadWrite, Category = "GTC|Vehicle") float Throttle = 0.0f;
    /** Cyclic pitch (heli) / elevator (plane), -1..1; negative = nose down. */
    UPROPERTY(BlueprintReadWrite, Category = "GTC|Vehicle") float PitchAxis = 0.0f;
    /** Cyclic roll (heli) / aileron (plane), -1..1; positive = bank right. */
    UPROPERTY(BlueprintReadWrite, Category = "GTC|Vehicle") float RollAxis = 0.0f;
    /** Pedal (heli) / rudder (plane) / steer (boat), -1..1. */
    UPROPERTY(BlueprintReadWrite, Category = "GTC|Vehicle") float YawAxis = 0.0f;
    /** Collective up(+)/down(-) for a helicopter, -1..1 (a held setting). */
    UPROPERTY(BlueprintReadWrite, Category = "GTC|Vehicle") float CollectiveRaise = 0.0f;
    /** Hold to flip the chase camera 180 (look behind). */
    UPROPERTY(BlueprintReadWrite, Category = "GTC|Vehicle") bool bLookBehind = false;
};

/**
 * UGTCVehicleLocomotionComponent — the abstract "kind of vehicle" strategy that
 * AGTCKinematicVehiclePawn drives. It is a THIN wrapper over a tested pure flight/handling
 * core: the pawn owns the shared tick (read input -> Step -> integrate Velocity -> contact
 * -> orient -> camera), and each subclass (helicopter / airplane / boat) plugs its core in
 * here. This is the component-strategy split that lets the police naval unit reuse the
 * player's boat locomotion wholesale.
 *
 * All velocities are Z-up cm/s (no Godot remap — the player cores are already Z-up).
 * Drop a concrete subclass on the pawn Blueprint; the pawn finds it by base class.
 */
UCLASS(Abstract, ClassGroup = (GTC))
class GTC_UE5_API UGTCVehicleLocomotionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    /** Stash this frame's inputs; the pawn fills it before Step. */
    void SetControls(const FVehicleControlState& In) { Controls = In; }

    /** Advance the wrapped pure core by Dt seconds. */
    virtual void Step(double Dt) {}

    /** World-space velocity (Z-up, cm/s) the pawn integrates into position. */
    virtual FVector WorldVelocity() const { return FVector::ZeroVector; }

    /** Facing heading (radians, 0 = +X). */
    virtual double HeadingRad() const { return 0.0; }

    /** Desired mesh attitude (yaw from heading + any bank/pitch lean). */
    virtual FRotator DesiredAttitude() const;

    /** True for watercraft: the pawn defers vertical to IntegrateVertical (buoyancy). */
    virtual bool WantsBuoyancy() const { return false; }

    /** Watercraft only: set InOutLocCm.Z (+ supply attitude via DesiredAttitude) from the
     *  ocean under the hull. Aircraft leave this and get a ground-clamp instead. */
    virtual void IntegrateVertical(FVector& InOutLocCm, double Dt) {}

    /** Horizontal speed (cm/s) the chase camera ramps FOV/distance on. */
    virtual float CameraSpeed() const { return static_cast<float>(WorldVelocity().Size2D()); }

protected:
    FVehicleControlState Controls;
};
