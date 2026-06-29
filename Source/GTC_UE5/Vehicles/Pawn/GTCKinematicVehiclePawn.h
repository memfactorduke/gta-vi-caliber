// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "../Contact/VehicleGroundContact.h"
#include "../Condition/VehicleHealth.h"
#include "GTCVehicleLocomotionComponent.h"
#include "GTCKinematicVehiclePawn.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UVehicleSeatComponent;
class UInputAction;
struct FInputActionValue;

/**
 * AGTCKinematicVehiclePawn — the one pawn behind every Wings & Waves vehicle (helicopter,
 * plane, boat, jet-ski). It owns the shared per-frame loop and the engine plumbing; the
 * *kind* of vehicle is a swappable UGTCVehicleLocomotionComponent added on the Blueprint
 * (BP_Helicopter / BP_Airplane / BP_Boat / BP_JetSki are this pawn + a different strategy
 * component + mesh + tuning). KINEMATIC by design — it integrates the locomotion core's
 * Velocity() into SetActorLocation each tick, exactly like AGTCPoliceHelicopter, so it
 * needs no ChaosVehicles dependency.
 *
 * Tick (only while driven): read input -> Locomotion.Step -> integrate Velocity -> contact
 * (aircraft ground clamp via FVehicleGroundContact, or watercraft buoyancy via the boat
 * component) -> orient mesh -> drive the chase cam -> tick FVehicleHealth.
 *
 * Enter/exit is the reused UVehicleSeatComponent (possess/attach/IMC/camera), driven by the
 * player's existing walk-up interaction — so boarding needs no new input here.
 *
 * NOTE (gate-deferred): this UObject layer compiles under UnrealBuildTool / runs in the
 * editor; it cannot be built headless in this environment (missing CesiumForUnreal). The
 * pure cores it drives are shim-verified + GTC.* tested.
 */
UCLASS()
class GTC_UE5_API AGTCKinematicVehiclePawn : public APawn
{
    GENERATED_BODY()

public:
    AGTCKinematicVehiclePawn();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    /** Hull health as a 0..1 fraction (HUD / damage FX). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Vehicle")
    float GetHealthFraction() const { return static_cast<float>(Health.HealthFraction()); }

    /** Fired once when the vehicle is destroyed — the seam for the explosion/smoke FX. */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|Vehicle")
    void OnExploded();

    /** Fired on a hard collision (hardness 0..1) — the seam for the impact thud/sparks. */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|Vehicle")
    void OnHardImpact(float Hardness01);

    virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

protected:
    // --- Components -------------------------------------------------------------
    UPROPERTY(VisibleAnywhere, Category = "GTC|Vehicle") TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere, Category = "GTC|Vehicle") TObjectPtr<USpringArmComponent> ChaseBoom;
    UPROPERTY(VisibleAnywhere, Category = "GTC|Vehicle") TObjectPtr<UCameraComponent> ChaseCam;
    UPROPERTY(VisibleAnywhere, Category = "GTC|Vehicle") TObjectPtr<UVehicleSeatComponent> Seats;

    // --- Input actions (mapped per-vehicle by its IMC; bind only what it uses) ---
    UPROPERTY(EditAnywhere, Category = "GTC|Input") TObjectPtr<UInputAction> PitchAction;
    UPROPERTY(EditAnywhere, Category = "GTC|Input") TObjectPtr<UInputAction> RollAction;
    UPROPERTY(EditAnywhere, Category = "GTC|Input") TObjectPtr<UInputAction> YawAction;
    UPROPERTY(EditAnywhere, Category = "GTC|Input") TObjectPtr<UInputAction> ThrottleAction;
    UPROPERTY(EditAnywhere, Category = "GTC|Input") TObjectPtr<UInputAction> CollectiveAction;
    UPROPERTY(EditAnywhere, Category = "GTC|Input") TObjectPtr<UInputAction> LookBehindAction;
    UPROPERTY(EditAnywhere, Category = "GTC|Input") TObjectPtr<UInputAction> ExitAction;

    // --- Health / contact tuning ------------------------------------------------
    UPROPERTY(EditAnywhere, Category = "GTC|Vehicle") float MaxHealth = 1000.0f;
    /** Speed (cm/s) below which a collision is harmless; above it damage grows quadratically. */
    UPROPERTY(EditAnywhere, Category = "GTC|Vehicle") float SafeImpactSpeedCm = 600.0f;
    UPROPERTY(EditAnywhere, Category = "GTC|Vehicle") float ImpactDamageScale = 0.0008f;

    /** Chase-camera shaping (reuses FVehicleChaseCamera). */
    UPROPERTY(EditAnywhere, Category = "GTC|Camera") float BaseFov = 90.0f;
    UPROPERTY(EditAnywhere, Category = "GTC|Camera") float MaxFov = 110.0f;
    UPROPERTY(EditAnywhere, Category = "GTC|Camera") float BaseBoom = 700.0f;
    UPROPERTY(EditAnywhere, Category = "GTC|Camera") float MaxBoom = 1400.0f;
    UPROPERTY(EditAnywhere, Category = "GTC|Camera") float SpeedAtMaxCam = 4000.0f;
    UPROPERTY(EditAnywhere, Category = "GTC|Camera") float CamPitchGain = 0.5f;
    UPROPERTY(EditAnywhere, Category = "GTC|Camera") float CamMaxPitchRad = 0.6f;

private:
    /** The vehicle-kind strategy, found by base class on the BP in BeginPlay. */
    UPROPERTY(Transient) TObjectPtr<UGTCVehicleLocomotionComponent> Locomotion;

    FVehicleControlState ControlState;
    FVehicleHealth Health{1000.0};
    FVehicleGroundContact::FParams GroundParams;
    FVehicleGroundContact::EContact PrevContact = FVehicleGroundContact::EContact::Airborne;
    bool bDead = false;

    /** Aircraft: down-trace, clamp to the ground, classify landing/crash. Mutates Loc.Z. */
    void ResolveAircraftContact(FVector& Loc, double VerticalSpeedCm, double Dt);

    /** Aim/length/FOV the chase boom from the current speed + climb. */
    void DriveChaseCam(float SpeedCmS, float ClimbCmS, float DeltaSeconds);

    void OnHardContact(const FHitResult& Hit, const FVector& Velocity);

    // EnhancedInput handlers.
    void InputPitch(const FInputActionValue& V);
    void InputRoll(const FInputActionValue& V);
    void InputYaw(const FInputActionValue& V);
    void InputThrottle(const FInputActionValue& V);
    void InputCollective(const FInputActionValue& V);
    void InputLookBehind(const FInputActionValue& V);
    void InputLookBehindReleased(const FInputActionValue& V);
    void InputExit(const FInputActionValue& V);
};
