// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../Pawn/GTCVehicleLocomotionComponent.h"
#include "HelicopterFlight.h"
#include "GTCHelicopterComponent.generated.h"

/**
 * UGTCHelicopterComponent — the helicopter strategy for AGTCKinematicVehiclePawn. Wraps
 * the tested FHelicopterFlight (rotor dynamics) and FVehicleAxisState (the HELD collective
 * lever) and exposes them through the locomotion interface. No Chaos: the pawn integrates
 * WorldVelocity() into position itself, exactly like AGTCPoliceHelicopter.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UGTCHelicopterComponent : public UGTCVehicleLocomotionComponent
{
    GENERATED_BODY()

public:
    UGTCHelicopterComponent();

    virtual void BeginPlay() override;
    virtual void Step(double Dt) override;
    virtual FVector WorldVelocity() const override { return Flight.Velocity(); }
    virtual double HeadingRad() const override { return Flight.Heading(); }
    virtual FRotator DesiredAttitude() const override;

    /** The hands-off hover collective (lift cancels gravity), 0..1. */
    UFUNCTION(BlueprintPure, Category = "GTC|Heli")
    float HoverCollective() const { return static_cast<float>(Flight.HoverCollective()); }

    /** Current collective setting, 0..1 (for the HUD). */
    UFUNCTION(BlueprintPure, Category = "GTC|Heli")
    float Collective() const { return static_cast<float>(CollectiveSetting); }

    // --- Tuning (per-airframe, designer-editable) -------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Heli") float MaxLiftAccel = 2000.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Heli") float TiltAccel = 1200.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Heli") float YawRatePerSec = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Heli") float LinearDrag = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Heli") float GravityCmS2 = 980.0f;

    /** How fast the collective lever moves at full input (per second). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Heli") float CollectiveRatePerSec = 0.8f;

    /** Visual body tilt (degrees) at full cyclic deflection. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Heli") float MaxVisualTiltDeg = 18.0f;

private:
    FHelicopterFlight Flight;
    /** The held collective lever, 0..1; starts at hover so a spawned heli holds altitude. */
    double CollectiveSetting = 0.0;

    void ApplyTuning();
};
