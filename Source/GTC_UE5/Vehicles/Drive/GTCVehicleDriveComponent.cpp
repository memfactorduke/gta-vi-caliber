// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCVehicleDriveComponent.h"

#include "VehicleDriveInput.h" // FVehicleDriveInput::ReverseEngageSpeed (threshold alignment)

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Math/UnrealMathUtility.h"

// EGTCGear mirrors FVehicleTransmission::EGear; the tick bridges them with a static_cast, so pin the
// values — reordering either enum breaks the build instead of silently mis-reporting the gear.
static_assert((uint8)FVehicleTransmission::EGear::Parked == (uint8)EGTCGear::Parked, "gear mirror drift");
static_assert((uint8)FVehicleTransmission::EGear::Reverse == (uint8)EGTCGear::Reverse, "gear mirror drift");
static_assert((uint8)FVehicleTransmission::EGear::Neutral == (uint8)EGTCGear::Neutral, "gear mirror drift");
static_assert((uint8)FVehicleTransmission::EGear::Drive == (uint8)EGTCGear::Drive, "gear mirror drift");

UGTCVehicleDriveComponent::UGTCVehicleDriveComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UGTCVehicleDriveComponent::SetDriveInput(float Steer, float Throttle, float Brake, float Handbrake)
{
    RawSteer = Steer;
    RawThrottle = Throttle;
    RawBrake = Brake;
    RawHandbrake = Handbrake;
}

const AActor* UGTCVehicleDriveComponent::ResolveVehicle() const
{
    if (const AActor* V = Vehicle)
    {
        return V;
    }
    return GetOwner();
}

bool UGTCVehicleDriveComponent::IsDriven() const
{
    const APawn* Pawn = Cast<APawn>(ResolveVehicle());
    return Pawn != nullptr && Pawn->GetController() != nullptr;
}

FVehicleTransmission::FParams UGTCVehicleDriveComponent::TransmissionParams() const
{
    FVehicleTransmission::FParams P;
    P.IdleRpm = IdleRpm;
    P.RedlineRpm = RedlineRpm;
    P.ShiftUpRpm = ShiftUpRpm;
    P.ShiftDownRpm = ShiftDownRpm;
    P.NumForwardGears = 3;
    P.ForwardRatios[0] = ForwardRatio1;
    P.ForwardRatios[1] = ForwardRatio2;
    P.ForwardRatios[2] = ForwardRatio3;
    P.ReverseRatio = ReverseRatio;
    P.EngineBrakeStrength = EngineBrakeStrength;
    // Align the gearbox's Drive<->Reverse standstill with the speed the input layer flips to
    // auto-reverse at, so a full brake-to-stop doesn't fall into a low-speed window where the
    // service brake is already zeroed but the box hasn't reversed yet (it would coast on engine
    // braking alone). Both are now the same road speed, expressed in the gearbox's wheel-RPM units.
    P.StandstillWheelRpm =
        FVehicleDriveResolver::WheelRpmFromForwardSpeed(FVehicleDriveInput::ReverseEngageSpeed, WheelRadiusCm);
    return P;
}

void UGTCVehicleDriveComponent::PublishIdle()
{
    // An unoccupied car makes no demands: zero the actuators, drop to Neutral at idle, and clear any
    // latched raw input so re-entering the car starts clean.
    RawSteer = RawThrottle = RawBrake = RawHandbrake = 0.0f;
    PrevSteer = 0.0;
    SteerOut = ThrottleOut = BrakeOut = HandbrakeOut = 0.0f;
    Gear = EGTCGear::Neutral;
    ForwardGearNumber = 1;
    EngineRpm = IdleRpm;
    bInDrivingGear = false;
}

void UGTCVehicleDriveComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (DeltaTime <= 0.0f)
    {
        return; // the steering smoother needs a real frame time
    }

    if (!IsDriven())
    {
        PublishIdle(); // parked / unoccupied — don't latch the last driver's input
        return;
    }

    const AActor* Car = ResolveVehicle();
    const FVector Velocity = Car->GetVelocity();
    const double Speed = Velocity.Size();
    const double ForwardSpeed = FVector::DotProduct(Velocity, Car->GetActorForwardVector());

    // Driven-wheel RPM: the real Chaos value if the BP fed one this frame, else estimated from the
    // car's forward speed through the wheel circumference.
    double Wheel;
    if (bHasExternalWheelRpm)
    {
        Wheel = WheelRpm;
        bHasExternalWheelRpm = false; // consume it; the BP re-feeds each frame if it wants
    }
    else
    {
        Wheel = FVehicleDriveResolver::WheelRpmFromForwardSpeed(ForwardSpeed, WheelRadiusCm);
    }

    // Shape the raw axes (steering feel + pedal auto-reverse), then run the gearbox.
    FVehicleDriveResolver::FInput In;
    In.RawSteer = RawSteer;
    In.RawThrottle = RawThrottle;
    In.RawBrake = RawBrake;
    In.SpeedCmS = Speed;
    In.ForwardSpeedCmS = ForwardSpeed;
    In.PrevSteer = PrevSteer;
    In.Dt = DeltaTime;

    const FVehicleDriveResolver::FOutput Shaped = FVehicleDriveResolver::Shape(In);
    PrevSteer = Shaped.Steer;

    FVehicleTransmission::FInputs TxIn;
    TxIn.Throttle = Shaped.GearboxThrottle;
    TxIn.Brake = Shaped.Brake;
    TxIn.Handbrake = RawHandbrake;
    TxIn.Steer = Shaped.Steer;
    Transmission.Update(TxIn, Wheel, DeltaTime, TransmissionParams());

    // Publish the actuators (for the Chaos movement component) + gear/RPM (for the HUD).
    SteerOut = (float)Transmission.SteerOut();
    ThrottleOut = (float)Transmission.ThrottleOut();
    BrakeOut = (float)Transmission.BrakeOut();
    HandbrakeOut = (float)Transmission.HandbrakeOut();
    Gear = static_cast<EGTCGear>((uint8)Transmission.Gear());
    ForwardGearNumber = Transmission.ForwardGearNumber();
    EngineRpm = (float)Transmission.Rpm();
    bInDrivingGear = Transmission.IsInDrivingGear();
}
