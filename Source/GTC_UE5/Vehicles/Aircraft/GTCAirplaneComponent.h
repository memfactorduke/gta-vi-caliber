// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../Pawn/GTCVehicleLocomotionComponent.h"
#include "FixedWingFlight.h"
#include "GTCAirplaneComponent.generated.h"

/**
 * UGTCAirplaneComponent — the fixed-wing strategy for AGTCKinematicVehiclePawn. Wraps the
 * tested FFixedWingFlight, whose defining mechanic is the STALL: lift comes from airspeed,
 * so "keep your speed up" is the whole skill. Throttle is a held lever (FVehicleAxisState);
 * elevator/aileron are momentary. Exposes Airspeed()/IsStalled() for the HUD stall warning.
 * Kinematic — the pawn integrates WorldVelocity() into position, no Chaos.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UGTCAirplaneComponent : public UGTCVehicleLocomotionComponent
{
    GENERATED_BODY()

public:
    UGTCAirplaneComponent();

    virtual void BeginPlay() override;
    virtual void Step(double Dt) override;
    virtual FVector WorldVelocity() const override { return Flight.Velocity(); }
    virtual double HeadingRad() const override { return Flight.Heading(); }
    virtual FRotator DesiredAttitude() const override;
    virtual float CameraSpeed() const override { return static_cast<float>(Flight.Airspeed()); }

    /** Forward airspeed, cm/s (HUD airspeed indicator). */
    UFUNCTION(BlueprintPure, Category = "GTC|Plane") float Airspeed() const { return static_cast<float>(Flight.Airspeed()); }
    /** Vertical speed, cm/s (HUD vertical-speed gauge). */
    UFUNCTION(BlueprintPure, Category = "GTC|Plane") float ClimbRate() const { return static_cast<float>(Flight.ClimbRate()); }
    /** True when the wings have stopped flying — drive the stall-warning light/horn. */
    UFUNCTION(BlueprintPure, Category = "GTC|Plane") bool IsStalled() const { return Flight.IsStalled(); }
    /** Current throttle setting, 0..1. */
    UFUNCTION(BlueprintPure, Category = "GTC|Plane") float Throttle() const { return static_cast<float>(ThrottleSetting); }

    // --- Tuning -----------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Plane") float MaxThrustAccel = 500.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Plane") float Drag = 0.2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Plane") float StallSpeed = 800.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Plane") float StallSinkRate = 600.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Plane") float CruiseSpeed = 1500.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Plane") float ClimbAuthority = 700.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Plane") float ClimbAirspeedCost = 300.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Plane") float TurnAuthority = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Plane") float ThrottleRatePerSec = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Plane") float MaxVisualBankDeg = 30.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Plane") float MaxVisualPitchRad = 0.6f;

private:
    FFixedWingFlight Flight;
    double ThrottleSetting = 0.0;

    void ApplyTuning();
};
