// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Vehicle stunt detection — the wheelies, stoppies, and big jumps that earn style points on
 * a bike or in a car. FStuntScore already BANKS tricks (AddTrick(kind, magnitude) -> combo
 * multiplier -> cash/respect), but something has to NOTICE the trick happened; nothing did.
 * This is that detector: fed the chassis pitch and whether the vehicle is airborne each
 * frame, it tracks the trick in progress and emits a completed trick the instant it ends.
 *
 * Pairs with FVehicleHandling::FDriftScorer (the drift side of the stunt economy) and feeds
 * the same FStuntScore. The adapter maps the returned EVehicleStunt to the score's kind
 * string (Wheelie->"wheelie", Stoppie->"stoppie", Jump->"jump").
 *
 * PURE-CORE: scene-free, deterministic, double precision, no engine coupling, no RNG. The
 * Car/Bike pawn reads its own pitch (radians; +nose-up), an airborne flag (no wheels in
 * contact), and height above the ground, calls Update(dt, ...) each physics frame, and on a
 * completed trick calls FStuntScore::AddTrick(KindString, Magnitude). Wheelie/stoppie
 * magnitude is seconds held; jump magnitude is peak height. Times are seconds. Unit-tested
 * headless (Vehicles/Stunt/Tests/VehicleStuntTest.cpp, prefix GTC.Vehicles.Stunt).
 */
enum class EVehicleStunt : uint8
{
    None,
    Wheelie, // nose up, front wheel(s) off the ground
    Stoppie, // nose down, rear wheel(s) off the ground (endo)
    Jump,    // fully airborne
};

/** What Update reports: a finished trick, or nothing. */
struct GTC_UE5_API FStuntDetection
{
    bool bCompleted = false;        // a trick finished THIS frame
    EVehicleStunt Kind = EVehicleStunt::None; // which trick (valid iff bCompleted)
    double Magnitude = 0.0;         // wheelie/stoppie: seconds held; jump: peak height
};

struct GTC_UE5_API FVehicleStuntDetector
{
    /** Nose-up pitch (radians) at/above which the vehicle counts as wheelie-ing (~15deg). */
    static constexpr double WheelieMinPitchRad = 0.26;
    /** Nose-down pitch (radians) at/below which it counts as a stoppie (~15deg). */
    static constexpr double StoppieMinPitchRad = 0.26;
    /** A wheelie/stoppie shorter than this (s) is a blip, not a stunt — discarded. */
    static constexpr double MinTrickSeconds = 0.4;
    /** A jump peaking below this height is a bump, not air — discarded. (Adapter units.) */
    static constexpr double MinJumpHeight = 0.5;

    EVehicleStunt Current = EVehicleStunt::None; // trick in progress
    double Duration = 0.0;                       // seconds the current trick has held
    double PeakHeight = 0.0;                      // peak height during the current jump

    /**
     * Advance one frame with the chassis pitch (radians, +nose-up), whether the vehicle is
     * airborne, and its height above the ground. Returns a completed trick on the frame the
     * current trick ends (if it held at least MinTrickSeconds); otherwise an empty result.
     */
    FStuntDetection Update(double Dt, double PitchRad, bool bAirborne, double HeightAboveGround);

    bool IsActive() const { return Current != EVehicleStunt::None; }
};
