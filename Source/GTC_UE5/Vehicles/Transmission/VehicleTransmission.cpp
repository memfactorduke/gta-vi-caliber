// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleTransmission.h"

#include "Math/UnrealMathUtility.h"

void FVehicleTransmission::Update(const FInputs& Inputs, double WheelRpm, double Dt, const FParams& Params)
{
    (void)Dt; // rate-free model; Dt is taken for parity with the rest of the vehicle stack

    // --- Normalise the player input axes to their valid ranges --------------------
    const double Throttle = FMath::Clamp(Inputs.Throttle, -1.0, 1.0);
    const double Brake = FMath::Clamp(Inputs.Brake, 0.0, 1.0);
    const double Handbrake = FMath::Clamp(Inputs.Handbrake, 0.0, 1.0);
    SteerActuator = FMath::Clamp(Inputs.Steer, -1.0, 1.0);

    // --- Sanitise the per-vehicle tuning so the math below can't blow up ----------
    const int32 MaxForward = 3; // size of FParams::ForwardRatios
    const int32 NumForward = FMath::Clamp(Params.NumForwardGears, 1, MaxForward);
    const double Idle = FMath::Max(0.0, Params.IdleRpm);
    const double Redline = FMath::Max(Idle, Params.RedlineRpm);
    const double Deadzone = FMath::Max(0.0, Params.SelectDeadzone);

    // Two distinct notions of "go backward":
    //   ReverseThrottle is the actual reverse DRIVE demand — only the negative throttle
    //     axis, never the foot brake (pressing the brake must never accelerate a reverse).
    //   ReverseSelect is the looser DIRECTION-pick demand used only to roll into Reverse
    //     from a dead stop, where holding the foot brake is the conventional way to back up.
    const double ForwardDemand = FMath::Max(0.0, Throttle);
    const double ReverseThrottle = FMath::Max(0.0, -Throttle);
    const double ReverseSelect = FMath::Max(ReverseThrottle, Brake);

    const double AbsWheelRpm = FMath::Abs(WheelRpm);
    const bool bStandstill = AbsWheelRpm <= FMath::Max(0.0, Params.StandstillWheelRpm);

    // --- Gear selection -----------------------------------------------------------
    // Parked is a held state: only the handbrake at a standstill latches it, and any
    // throttle releases it (so the player can simply drive away).
    if (CurrentGear == EGear::Parked)
    {
        if (ForwardDemand > Deadzone || (Handbrake <= 0.0 && ReverseSelect > Deadzone))
        {
            CurrentGear = EGear::Neutral; // released; fall through to direction pick below
        }
    }

    if (CurrentGear != EGear::Parked)
    {
        if (bStandstill)
        {
            // From a stop we may freely flip direction based on what the player asks.
            if (Handbrake > 0.0 && ForwardDemand <= Deadzone && ReverseSelect <= Deadzone)
            {
                CurrentGear = EGear::Parked; // holding the handbrake with no go-input parks it
            }
            else if (ForwardDemand > Deadzone)
            {
                CurrentGear = EGear::Drive;
                ForwardIndex = 0; // pull away in first
            }
            else if (ReverseSelect > Deadzone)
            {
                CurrentGear = EGear::Reverse;
            }
            else
            {
                CurrentGear = EGear::Neutral; // coasting to rest with no input
            }
        }
        else
        {
            // While rolling, the box stays in its current driving direction; the wheel
            // RPM sign keeps the HUD honest if the chassis was already moving.
            if (CurrentGear == EGear::Neutral)
            {
                CurrentGear = (WheelRpm >= 0.0) ? EGear::Drive : EGear::Reverse;
                if (CurrentGear == EGear::Drive)
                {
                    ForwardIndex = FMath::Clamp(ForwardIndex, 0, NumForward - 1);
                }
            }
        }
    }

    // --- Forward auto-shift -------------------------------------------------------
    // Engine RPM is derived from the wheel RPM through the gear that is engaged. We
    // compute it for the candidate gear, then let it cross the shift thresholds.
    auto EngineRpmFor = [&](double Ratio) -> double
    {
        const double Raw = AbsWheelRpm * FMath::Max(0.0, Ratio);
        return FMath::Clamp(FMath::Max(Idle, Raw), Idle, Redline);
    };

    if (CurrentGear == EGear::Drive)
    {
        ForwardIndex = FMath::Clamp(ForwardIndex, 0, NumForward - 1);
        EngineRpm = EngineRpmFor(Params.ForwardRatios[ForwardIndex]);

        // Keep shifting UP while the engine is still over ShiftUpRpm in this gear and a
        // taller gear exists — so a single huge frame (e.g. a wheel snapped to a high RPM)
        // settles in the right gear instead of one step per frame. The bounded `Guard`
        // can never exceed NumForward steps because ForwardIndex strictly rises.
        for (int32 Guard = 0; ForwardIndex < NumForward - 1 && EngineRpm >= Params.ShiftUpRpm
                              && Guard < NumForward; ++Guard)
        {
            ++ForwardIndex;
            EngineRpm = EngineRpmFor(Params.ForwardRatios[ForwardIndex]);
        }

        // Otherwise keep shifting DOWN while bogging under ShiftDownRpm and a lower gear
        // exists (mutually exclusive with the up-shift above — the engine can't be both
        // over ShiftUp and under ShiftDown when ShiftDown < ShiftUp).
        for (int32 Guard = 0; ForwardIndex > 0 && EngineRpm <= Params.ShiftDownRpm
                              && Guard < NumForward; ++Guard)
        {
            --ForwardIndex;
            EngineRpm = EngineRpmFor(Params.ForwardRatios[ForwardIndex]);
        }
    }
    else if (CurrentGear == EGear::Reverse)
    {
        EngineRpm = EngineRpmFor(Params.ReverseRatio);
    }
    else
    {
        // Neutral / Parked: the engine idles, blipped by the player's throttle so the
        // HUD needle still moves even with no load on the wheels.
        EngineRpm = FMath::Clamp(Idle + ForwardDemand * (Redline - Idle), Idle, Redline);
    }

    // --- Actuator outputs ---------------------------------------------------------
    HandbrakeActuator = Handbrake;
    ThrottleActuator = 0.0;
    BrakeActuator = 0.0;

    switch (CurrentGear)
    {
    case EGear::Parked:
        // Locked: no drive, full brake hold, handbrake forced on regardless of input.
        BrakeActuator = 1.0;
        HandbrakeActuator = 1.0;
        break;

    case EGear::Drive:
        ThrottleActuator = ForwardDemand;
        // The foot brake, plus engine braking when coasting (off-throttle) at speed.
        if (ForwardDemand <= 0.0 && !bStandstill)
        {
            BrakeActuator = FMath::Max(Brake, FMath::Clamp(Params.EngineBrakeStrength, 0.0, 1.0));
        }
        else
        {
            BrakeActuator = Brake;
        }
        break;

    case EGear::Reverse:
        // Reverse drive comes ONLY from the negative throttle axis. The foot brake is a
        // brake here (never a throttle), so stabbing it slows a backward roll. A forward
        // throttle stab also brakes, the way it would arrest a reverse before re-selecting.
        ThrottleActuator = ReverseThrottle;
        BrakeActuator = FMath::Max(Brake, ForwardDemand);
        break;

    case EGear::Neutral:
    default:
        // No drive reaches the wheels; the foot brake still works.
        BrakeActuator = Brake;
        break;
    }

    ThrottleActuator = FMath::Clamp(ThrottleActuator, 0.0, 1.0);
    BrakeActuator = FMath::Clamp(BrakeActuator, 0.0, 1.0);
    HandbrakeActuator = FMath::Clamp(HandbrakeActuator, 0.0, 1.0);
}
