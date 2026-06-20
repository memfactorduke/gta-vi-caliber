// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The bridge between what the PLAYER asks for and what the PHYSICS gets — the
 * deterministic half of the Phase-1 Driving slice that sits between FVehicleEntry
 * (you are now in the seat) and FVehicleHandling (now make it feel arcadey). Once
 * the player is seated, every frame the ChaosVehicle pawn adapter reads the
 * Enhanced Input axes — throttle in [-1, 1], brake in [0, 1], handbrake in
 * [0, 1], steer in [-1, 1] — and the engine's measured wheel RPM, hands them to
 * Update(...), and reads back two things: the GEAR/RPM to show on the HUD, and a
 * pair of normalised actuator scalars (throttle 0..1, brake 0..1) to feed the
 * physics solver. This core owns the gearbox logic; it never touches a wheel.
 *
 * The model is a simple 3-speed "auto-manual": one Drive band with three forward
 * ratios that auto-shift on RPM, one Reverse ratio, plus Neutral and Parked. The
 * gear is chosen the way an arcade driver expects — hold throttle from a standstill
 * to roll into Drive, hold brake/back from a standstill to roll into Reverse — so
 * the player never thinks about a clutch but the HUD still shows a real gear. The
 * engine RPM is DERIVED from the wheel RPM through the active gear ratio (so it
 * tracks the chassis the solver actually produced) and the auto-shift watches that
 * RPM cross ShiftUpRpm / ShiftDownRpm to step the forward gear up or down. Engine
 * braking (lifting the throttle at speed) is folded into the brake actuator so a
 * coasting car slows like a real one without the player touching the brake.
 *
 * It pairs with the Car adapter exactly as FVehicleHandling and FVehicleEntry do:
 * the adapter supplies the live inputs + wheel RPM each physics frame, calls
 * Update, applies ThrottleOut()/BrakeOut()/HandbrakeOut() to the Chaos vehicle
 * movement component, applies SteerOut() to the steering, and reads Gear()/Rpm()
 * for the HUD. Mapping those scalars onto a specific UChaosWheeledVehicleMovement
 * component is the DEFERRED adapter's job.
 *
 * GTC-original pure-core (no Godot oracle): scene-free, deterministic, double
 * precision, no UObject. Unit-tested headless
 * (Vehicles/Transmission/Tests/VehicleTransmissionTest.cpp, prefix
 * GTC.Vehicles.Transmission).
 *
 * PURE-CORE boundary: reading the Enhanced Input axes and pushing the resulting
 * scalars into a ChaosVehicle movement component is the DEFERRED adapter and is
 * NOT covered by the tests. Units: RPM is revolutions/minute; wheel RPM is the
 * signed angular speed of the driven wheels (positive = rolling forward). All
 * control signals are dimensionless.
 */
struct GTC_UE5_API FVehicleTransmission
{
    /** The selected gear, for the HUD and the actuator logic. */
    enum class EGear : uint8
    {
        Parked,  // handbrake-locked, no drive, no roll
        Reverse, // single reverse ratio
        Neutral, // engine spins but no drive reaches the wheels
        Drive,   // forward band: one of NumForwardGears auto-shifted ratios
    };

    /** Per-vehicle tuning the adapter passes in (so a truck and a coupe can differ). */
    struct FParams
    {
        /** Idle engine speed (RPM) — the floor the engine never drops below. */
        double IdleRpm = 800.0;
        /** Redline (RPM) — the engine RPM ceiling the gearbox shifts to avoid. */
        double RedlineRpm = 7000.0;
        /** Auto-shift UP to the next forward gear when engine RPM rises to/over this. */
        double ShiftUpRpm = 6200.0;
        /** Auto-shift DOWN to the previous forward gear when engine RPM falls to/under this. */
        double ShiftDownRpm = 2200.0;

        /** Number of forward ratios in the Drive band (this is the 3-speed box). */
        int32 NumForwardGears = 3;
        /** Forward gear ratios, fastest-revving first; size must be >= NumForwardGears.
         *  Engine RPM = |WheelRpm| * Ratio, so gear 1 (high ratio) revs hardest. */
        double ForwardRatios[3] = {12.0, 7.0, 4.5};
        /** Reverse ratio (same RPM mapping as a forward gear). */
        double ReverseRatio = 12.0;

        /** Wheel RPM whose magnitude is at/under this counts as a standstill — the only
         *  state from which the box may flip between Drive and Reverse. */
        double StandstillWheelRpm = 5.0;
        /** Throttle/brake magnitude needed to actually select a direction from a standstill
         *  (so a feather touch / drift on the stick doesn't jam it into gear). */
        double SelectDeadzone = 0.1;

        /** Fraction of full brake applied by engine braking when fully off the throttle in
         *  a forward gear at speed (lifting to coast bleeds speed without the brake). */
        double EngineBrakeStrength = 0.15;
    };

    /** The raw player input axes for one frame (Enhanced Input ranges). */
    struct FInputs
    {
        double Throttle = 0.0;  // [-1, 1]; forward demand (the sign also picks Drive vs Reverse at rest)
        double Brake = 0.0;     // [0, 1]; foot brake
        double Handbrake = 0.0; // [0, 1]; parking / drift brake
        double Steer = 0.0;     // [-1, 1]; negative = left, positive = right
    };

    /**
     * Advance the gearbox one physics frame. Clamps the inputs to their valid
     * ranges, derives engine RPM from |WheelRpm| through the active gear, selects
     * Drive/Reverse/Neutral/Parked, runs the forward auto-shift, and bakes the
     * actuator scalars. `Dt` is accepted for parity with the rest of the vehicle
     * stack and clamped to >= 0; the model is rate-free so the result depends only
     * on the current inputs + RPM, never on the frame length. Call once per frame
     * after the player is Seated; reading the outputs after Update is the contract.
     */
    void Update(const FInputs& Inputs, double WheelRpm, double Dt, const FParams& Params);

    /** Selected gear this frame (for the HUD). */
    EGear Gear() const { return CurrentGear; }

    /** Active forward ratio index within the Drive band, 1..NumForwardGears (1 when
     *  not in Drive) — the "1/2/3" a manual-style HUD prints. */
    int32 ForwardGearNumber() const { return ForwardIndex + 1; }

    /** Derived engine speed this frame (RPM), clamped to [IdleRpm, RedlineRpm]. */
    double Rpm() const { return EngineRpm; }

    /** Throttle scalar to feed the physics drive torque, 0..1. Zero unless Drive or
     *  Reverse is engaged AND the player is demanding go in that direction. */
    double ThrottleOut() const { return ThrottleActuator; }

    /** Brake scalar to feed the physics brakes, 0..1 — the foot brake plus engine
     *  braking, and full lock while Parked. */
    double BrakeOut() const { return BrakeActuator; }

    /** Handbrake scalar passthrough, 0..1 (also forced to 1 while Parked). */
    double HandbrakeOut() const { return HandbrakeActuator; }

    /** Steering scalar to feed the physics steering, clamped to [-1, 1]. */
    double SteerOut() const { return SteerActuator; }

    /** True when the box is delivering forward or reverse drive torque this frame. */
    bool IsInDrivingGear() const { return CurrentGear == EGear::Drive || CurrentGear == EGear::Reverse; }

private:
    EGear CurrentGear = EGear::Neutral;
    /** 0-based active forward ratio within the Drive band (0 == gear 1). */
    int32 ForwardIndex = 0;
    double EngineRpm = 0.0;

    double ThrottleActuator = 0.0;
    double BrakeActuator = 0.0;
    double HandbrakeActuator = 0.0;
    double SteerActuator = 0.0;
};
