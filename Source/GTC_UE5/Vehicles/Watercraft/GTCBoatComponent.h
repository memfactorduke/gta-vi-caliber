// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../Pawn/GTCVehicleLocomotionComponent.h"
#include "BoatHandling.h"
#include "WatercraftFloat.h"
#include "GTCBoatComponent.generated.h"

/**
 * UGTCBoatComponent — the watercraft strategy for AGTCKinematicVehiclePawn. Composes the
 * two tested halves: FBoatHandling drives the planar surge/sway/heading (the planing hump,
 * the drift-then-grip), and FWatercraftFloat sits the hull ON the live ocean (height + pitch
 * + roll from the swell, capsize on a steep face). It samples the ONE wave-set via
 * UGTCOceanSubsystem (the cm<->m bridge). Kinematic — no Chaos, no buoyancy force solver.
 * The police Coast Guard boat reuses this exact component for its motion.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UGTCBoatComponent : public UGTCVehicleLocomotionComponent
{
    GENERATED_BODY()

public:
    UGTCBoatComponent();

    virtual void BeginPlay() override;
    virtual void Step(double Dt) override;
    virtual FVector WorldVelocity() const override { return Boat.Velocity(); } // planar, Z=0
    virtual double HeadingRad() const override { return Boat.Heading(); }
    virtual bool WantsBuoyancy() const override { return true; }
    virtual void IntegrateVertical(FVector& InOutLocCm, double Dt) override;
    virtual FRotator DesiredAttitude() const override;
    virtual float CameraSpeed() const override;

    UFUNCTION(BlueprintPure, Category = "GTC|Boat") bool IsPlaning() const { return Boat.IsPlaning(); }
    UFUNCTION(BlueprintPure, Category = "GTC|Boat") float ForwardSpeed() const { return static_cast<float>(Boat.ForwardSpeed()); }
    UFUNCTION(BlueprintPure, Category = "GTC|Boat") bool IsCapsized() const { return CachedPose.bCapsized; }

protected:
    // --- Handling tuning --------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Boat") float EngineAccel = 800.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Boat") float ForwardDragPlowing = 0.9f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Boat") float ForwardDragPlaning = 0.3f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Boat") float LateralDrag = 3.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Boat") float PlaningSpeed = 600.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Boat") float RudderAuthority = 1.2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Boat") float RudderFlowSpeed = 300.0f;

    // --- Float tuning -----------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Boat") float DraftCm = 40.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Boat") float CapsizeAngleRad = 1.2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Boat") float RoughnessRefCm = 200.0f;

    /** Hull contact points in the boat's local frame (cm; X forward, Y right). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Boat") TArray<FVector2D> HullSamplesCm;

    FBoatHandling Boat;
    FWatercraftFloat::FParams FloatParams() const;
    void ApplyHandlingTuning();

private:
    FWatercraftFloat::FPose CachedPose;
};
