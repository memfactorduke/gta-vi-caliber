// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VehicleDriveResolver.h"                 // FVehicleDriveResolver (+ FVehicleDriveInput)
#include "../Transmission/VehicleTransmission.h"  // FVehicleTransmission
#include "GTCVehicleDriveComponent.generated.h"

class AActor;

/** Blueprint mirror of FVehicleTransmission::EGear (1:1 order so a static_cast bridges the seam). */
UENUM(BlueprintType)
enum class EGTCGear : uint8
{
    Parked,
    Reverse,
    Neutral,
    Drive,
};

/**
 * UGTCVehicleDriveComponent — the driver-command pipeline for the player car: the "Car adapter" both
 * FVehicleDriveInput and FVehicleTransmission describe (they read the Enhanced Input axes + wheel RPM
 * each frame and hand back steering / throttle / brake actuators plus a HUD gear + RPM). It is the
 * input/powertrain half of the car; the wet-road grip half is the FVehicleHandling core.
 *
 * The pure FVehicleDriveResolver shapes the raw axes (steering deadzone + speed-sensitive authority +
 * smoothing, pedals + auto-reverse); this shell owns the stateful FVehicleTransmission (a 3-speed
 * auto-manual: auto-shift on RPM, engine braking, Drive/Reverse/Neutral/Parked selection). Each tick
 * it reads the car's speed (and derives wheel RPM from it, unless the BP feeds the real Chaos value),
 * shapes the stored input, advances the gearbox, and PUBLISHES the actuator scalars + gear/RPM. It
 * only computes while the car is being driven (a possessed pawn); an unoccupied car publishes zero.
 *
 * Chassis-agnostic (bare-AActor reads, NO ChaosVehicles dep), plain UActorComponent (NO new
 * subsystem). Feed it input from the car BP's Enhanced Input each frame via SetDriveInput, then apply
 * the published SteerOut/ThrottleOut/BrakeOut/HandbrakeOut to the Chaos movement component and show
 * Gear/EngineRpm on the HUD — the same "expose the seam, let the BP apply it" split as the grip core.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UGTCVehicleDriveComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTCVehicleDriveComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // --- Per-frame input the car BP feeds from Enhanced Input ----------------------

    /** Feed this frame's raw axes: steer [-1,1], throttle [0,1], brake [0,1], handbrake [0,1]. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Vehicle|Drive")
    void SetDriveInput(float Steer, float Throttle, float Brake, float Handbrake);

    /** Optionally feed the real driven-wheel RPM from the Chaos movement component. It is consumed for
     *  ONE frame — call it every frame you want it used, else the tick falls back to the speed +
     *  WheelRadiusCm estimate. Signed: positive = rolling forward. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Vehicle|Drive")
    void SetWheelRpm(float InWheelRpm)
    {
        WheelRpm = InWheelRpm;
        bHasExternalWheelRpm = true;
    }

    // --- Config -------------------------------------------------------------------

    /** The car whose velocity drives speed + the wheel-RPM estimate. Null => this component's owner. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Drive")
    TObjectPtr<AActor> Vehicle = nullptr;

    /** Driven-wheel radius (cm) for the speed -> wheel-RPM estimate (ignored if SetWheelRpm is fed). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Drive", meta = (ClampMin = "1.0"))
    float WheelRadiusCm = 35.0f;

    // Gearbox tuning (maps to FVehicleTransmission::FParams).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Drive|Gearbox", meta = (ClampMin = "0.0"))
    float IdleRpm = 800.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Drive|Gearbox", meta = (ClampMin = "0.0"))
    float RedlineRpm = 7000.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Drive|Gearbox", meta = (ClampMin = "0.0"))
    float ShiftUpRpm = 6200.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Drive|Gearbox", meta = (ClampMin = "0.0"))
    float ShiftDownRpm = 2200.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Drive|Gearbox", meta = (ClampMin = "0.1"))
    float ForwardRatio1 = 12.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Drive|Gearbox", meta = (ClampMin = "0.1"))
    float ForwardRatio2 = 7.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Drive|Gearbox", meta = (ClampMin = "0.1"))
    float ForwardRatio3 = 4.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Drive|Gearbox", meta = (ClampMin = "0.1"))
    float ReverseRatio = 12.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Drive|Gearbox", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EngineBrakeStrength = 0.15f;

    // --- Published outputs (apply to the Chaos movement component; show on the HUD) ---

    /** Steering scalar to feed the physics steering, [-1,1] (shaped + smoothed). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicle|Drive")
    float SteerOut = 0.0f;

    /** Throttle scalar for the drive torque, 0..1. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicle|Drive")
    float ThrottleOut = 0.0f;

    /** Brake scalar (foot brake + engine braking; full lock while Parked), 0..1. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicle|Drive")
    float BrakeOut = 0.0f;

    /** Handbrake scalar, 0..1. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicle|Drive")
    float HandbrakeOut = 0.0f;

    /** Selected gear (for the HUD). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicle|Drive")
    EGTCGear Gear = EGTCGear::Neutral;

    /** Forward ratio 1..N when in Drive (1 otherwise) — the "1/2/3" a manual HUD prints. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicle|Drive")
    int32 ForwardGearNumber = 1;

    /** Engine speed this frame (RPM), for the tacho. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicle|Drive")
    float EngineRpm = 0.0f;

    /** True while the box is delivering forward or reverse drive torque. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicle|Drive")
    bool bInDrivingGear = false;

private:
    /** The car whose motion drives the pipeline (Vehicle if set, else the owner). */
    const AActor* ResolveVehicle() const;

    /** True when the resolved car is a possessed pawn (being driven). */
    bool IsDriven() const;

    /** Publish an idle/neutral state and clear latched input (used while unoccupied). */
    void PublishIdle();

    /** Build gearbox tuning from the editable properties. */
    FVehicleTransmission::FParams TransmissionParams() const;

    /** The stateful gearbox (plain runtime state; not reflected). */
    FVehicleTransmission Transmission;

    /** Last frame's smoothed steer, carried for the steering interp. */
    double PrevSteer = 0.0;

    // Raw axes fed by the BP each frame via SetDriveInput.
    float RawSteer = 0.0f;
    float RawThrottle = 0.0f;
    float RawBrake = 0.0f;
    float RawHandbrake = 0.0f;

    // Optional real wheel RPM from Chaos (else estimated from speed).
    float WheelRpm = 0.0f;
    bool bHasExternalWheelRpm = false;
};
