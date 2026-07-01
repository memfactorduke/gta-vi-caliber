// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VehicleGripResolver.h" // FVehicleGripResolver (+ FVehicleHandling, FRoadGrip)
#include "GTCVehicleHandlingComponent.generated.h"

/**
 * UGTCVehicleHandlingComponent — the arcade-handling brain that lives on the player car
 * (BP_VehicleBase), the sibling of UVehicleSeatComponent (get in/out). It wires the three
 * orphan handling cores into the world so the car finally REACTS to the weather: a wet road
 * bleeds grip, the tail steps out, and past the aquaplane speed a flooded road floats the
 * car. It is the player-car counterpart to the ambient traffic already driving cautious in
 * the rain (FTrafficWeather / UGTCTrafficSubsystem).
 *
 * Chassis-agnostic by design — like UVehicleSeatComponent it treats the car as a bare
 * AActor/APawn (reads GetVelocity()/GetActorForwardVector()), so it needs NO ChaosVehicles
 * module dependency. Each tick, while the car is being driven, it gathers the chassis state,
 * calls the pure FVehicleGripResolver, then PUBLISHES the result for the car BP, HUD and FX.
 * It deliberately does NOT poke the physics itself: the BP reads GripFactor (multiply your
 * tyre friction by it) and/or GrippedVelocity (adopt it for a full arcade feel) — the same
 * "expose the seam, let the BP apply it" split the project uses for URoadGripLibrary. All the
 * handling math lives in the pure resolver; this shell is just the engine plumbing.
 *
 * Wetness is an INPUT the car BP feeds (SetWetness / the Wetness property), sourced from the
 * weather seam (AGTCWeatherController::GetWetness() / URoadGripLibrary / the wetness MPC).
 * Keeping it an input rather than reaching into the weather actor keeps the handling brain
 * decoupled from the weather subsystem.
 *
 * It also runs the GTA-style drift scorer (FVehicleHandling::FDriftScorer): hold a clean
 * slide for points, bank on a clean exit, wipe on a spin-out. NOTE: the scorer is surfaced as
 * raw payout for now; wiring BankDrift() into the FStuntScore economy (see Vehicles/Stunt) so
 * drift points become cash/respect is a deferred follow-up, left to the BP / a later PR.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UGTCVehicleHandlingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTCVehicleHandlingComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // --- Per-frame input the car BP feeds -------------------------------------------

    /** Handbrake held this frame (bind to the car BP's handbrake action). Steps the tail out. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Vehicle|Handling")
    void SetHandbrake(bool bHeld) { bHandbrakeHeld = bHeld; }

    /** Feed current surface wetness 0..1 (from the weather seam). 0 => drive in the dry. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Vehicle|Handling")
    void SetWetness(float InWetness) { Wetness = FMath::Clamp(InWetness, 0.0f, 1.0f); }

    /** Surface wetness 0..1 the grip is computed from — set by the BP each frame (or SetWetness). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Handling", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Wetness = 0.0f;

    /** Tyre/wear grip 0..1 (1 = fresh) — feed FVehicleCondition::GripFactor here when wired; default 1. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Handling", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float TyreGripFactor = 1.0f;

    // --- Tuning (designer-editable on the component; maps 1:1 to FRoadGrip::FParams) --

    /** Grip lost at full wetness, low speed (so wet low-speed road grip = 1 - WetGripLoss). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Handling", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WetGripLoss = 0.35f;

    /** Speed (km/h) at which aquaplaning begins on a fully wet road. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Handling", meta = (ClampMin = "0.0"))
    float AquaplaneSpeedKmh = 90.0f;

    /** Speed (km/h) at which aquaplaning is fully developed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Handling", meta = (ClampMin = "0.0"))
    float AquaplaneSpeedFullKmh = 160.0f;

    /** Grip floor when fully aquaplaning (keep below 1 - WetGripLoss). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Handling", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float AquaplaneFloor = 0.25f;

    // --- Published outputs the car BP / HUD / FX read (updated each driven tick) ------

    /** Road x tyre grip this frame, 0..1. Multiply the chassis' lateral tyre friction by this. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicle|Handling")
    float GripFactor = 1.0f;

    /** The road-only grip (FRoadGrip), 0..1 — before the tyres' own wear is folded in. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicle|Handling")
    float RoadGrip = 1.0f;

    /** Angle (degrees) between where the car points and where it's moving — the slide angle. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicle|Handling")
    float SlipAngleDegrees = 0.0f;

    /** How hard the car is drifting FORWARD, 0..1 — drives drift FX, camera lean and the scorer. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicle|Handling")
    float DriftFactor = 0.0f;

    /** Fraction of sideways velocity the tyres kept this frame, 0..1 (high = sliding). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicle|Handling")
    float LateralRetention = 0.0f;

    /** How developed aquaplaning is right now, 0..1. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicle|Handling")
    float AquaplaneAmount = 0.0f;

    /** True while the car is aquaplaning (water present and over the aquaplane speed). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicle|Handling")
    bool bAquaplaning = false;

    /** The post-grip velocity (chassis units) the arcade layer wants this frame. Set the
     *  chassis' linear velocity to this for a full arcade feel, or ignore it and just scale
     *  friction by GripFactor. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicle|Handling")
    FVector GrippedVelocity = FVector::ZeroVector;

    // --- Drift scorer (GTA-style "hold a slide for points") --------------------------

    /** Points the current slide would pay if banked right now. */
    UFUNCTION(BlueprintPure, Category = "GTC|Vehicle|Handling|Drift")
    float GetPendingDriftPayout() const { return (float)Scorer.PendingPayout(); }

    /** Current combo multiplier (1..MaxMultiplier), grows the longer the slide is held. */
    UFUNCTION(BlueprintPure, Category = "GTC|Vehicle|Handling|Drift")
    float GetDriftMultiplier() const { return (float)Scorer.Multiplier; }

    /** True while a scoring slide is banked and unresolved. */
    UFUNCTION(BlueprintPure, Category = "GTC|Vehicle|Handling|Drift")
    bool IsDriftActive() const { return Scorer.IsActive(); }

    /** Clean exit: banks the run and returns the payout (Score x Multiplier). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Vehicle|Handling|Drift")
    float BankDrift() { return (float)Scorer.Bank(); }

    /** Spin-out / crash: wipes the run and returns the score lost. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Vehicle|Handling|Drift")
    float WipeDrift() { return (float)Scorer.Wipe(); }

private:
    /** Build FRoadGrip tuning from the editable properties. */
    FRoadGrip::FParams RoadParams() const;

    /** True when the car is currently possessed/driven — the tick early-outs otherwise so a
     *  parked/unoccupied car pays only this check per frame (mirrors UVehicleSeatComponent). */
    bool IsBeingDriven() const;

    /** Handbrake latch set by the BP each frame (SetHandbrake). */
    bool bHandbrakeHeld = false;

    /** The stunt scorer (plain runtime state; not reflected). */
    FVehicleHandling::FDriftScorer Scorer;
};
