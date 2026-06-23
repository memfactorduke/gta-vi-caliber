// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The brain behind "press a key, get in the car, drive off, get back out" — the
 * deterministic half of the Phase-1 Driving slice (ROADMAP: Driving — enter/exit,
 * NET-NEW). It answers the two questions the get-in/get-out flow asks and nothing
 * about the engine: WHICH seat does the player take, and WHERE is the timed
 * get-in / get-out transition right now.
 *
 * It pairs with a Car adapter the same way FVehicleHandling does: the adapter (a
 * ChaosVehicle pawn / the player controller) gathers the candidate seats and the
 * player's position, calls NearestAvailableSeat to pick one, drives the transition
 * with Update each frame, and — on the single frame a transition COMPLETES — does
 * the engine-only work this core deliberately never touches: Possess/UnPossess the
 * pawn, attach the player mesh to the seat socket, play the get-in montage, swap
 * the camera. Possessing exactly once is why Update returns a one-shot "just
 * finished" edge instead of a level signal.
 *
 * State machine (one occupant — the player):
 *
 *     OnFoot --BeginEnter--> Entering --(EnterSeconds)--> Seated
 *       ^                                                   |
 *       +------------------(ExitSeconds)-- Exiting <--BeginExit
 *
 * BeginEnter / BeginExit are guarded: you can only start entering from OnFoot and
 * only start exiting from Seated, so a mashed key during the get-in animation is a
 * no-op (returns false) instead of teleporting the player. The active seat is held
 * all the way through Exiting so the adapter knows which door to put the player
 * beside; it clears to None only once both feet are back on the street.
 *
 * GTC-original pure-core (no Godot oracle): scene-free, deterministic, double
 * precision, no UObject. Unit-tested headless
 * (Vehicles/Entry/Tests/VehicleEntryTest.cpp, prefix GTC.Vehicles.Entry).
 *
 * Frame / units: seat positions live on the repo's XZ ground plane (Y up), matching
 * FInteraction; NearestAvailableSeat uses a full 3D distance (FVector::Dist) exactly
 * as FInteraction::Nearest, so reach reads the same for a door as for any other
 * interactable. Distances are metres, durations seconds. Mapping seat anchors to a
 * skeletal-mesh socket in UE's Z-up space is the adapter's job, not this core's.
 */
struct GTC_UE5_API FVehicleEntry
{
    /** Returned by NearestAvailableSeat / Seat() when there is no seat. */
    static constexpr int32 None = -1;

    /** Where the player gets in / out, and whether someone is already there. */
    struct FSeat
    {
        /** Seat position on the XZ ground plane (where the occupant rides). */
        FVector Anchor = FVector::ZeroVector;
        /** Offset from Anchor to the standing spot beside the door, used to place
         *  the player on exit (e.g. a step out to the seat's side). */
        FVector ExitOffset = FVector::ZeroVector;
        /** True when another occupant already holds this seat (skip it). */
        bool bOccupied = false;
    };

    /** Per-vehicle tuning the adapter passes in (so a bus and a coupe can differ). */
    struct FParams
    {
        /** How close (metres) the player must be to a seat to enter it. */
        double Reach = 2.5;
        /** Get-in transition duration (seconds). */
        double EnterSeconds = 0.9;
        /** Get-out transition duration (seconds). */
        double ExitSeconds = 0.8;
    };

    /** Where the occupant is in the get-in / get-out flow. */
    enum class EState : uint8
    {
        OnFoot,   // standing outside, no seat held
        Entering, // playing the get-in transition toward a seat
        Seated,   // in the seat (the adapter has possessed the pawn)
        Exiting,  // playing the get-out transition back to the street
    };

    /**
     * Index into `Seats` of the nearest UNOCCUPIED seat within `Reach` metres of
     * `PlayerPos`, or None. The reach boundary is inclusive; ties resolve to the
     * lower index (so the driver seat at index 0 wins when you walk up to the front
     * corner); a non-positive reach selects nothing.
     */
    static int32 NearestAvailableSeat(const TArray<FSeat>& Seats, const FVector& PlayerPos, double Reach);

    /** The street position to drop the player at when they leave `Seat`. */
    static FVector ExitStandPosition(const FSeat& Seat) { return Seat.Anchor + Seat.ExitOffset; }

    /**
     * Start getting into `SeatIndex`. Only valid from OnFoot with a real seat index
     * (>= 0); returns false (and changes nothing) otherwise, so a double-press can't
     * restart the animation. The caller picks `SeatIndex` with NearestAvailableSeat,
     * which already rejected occupied / out-of-reach seats.
     */
    bool BeginEnter(int32 SeatIndex);

    /**
     * Start getting out. Only valid from Seated; returns false otherwise. Keeps the
     * active seat through the transition so the adapter can place the player at that
     * seat's ExitStandPosition.
     */
    bool BeginExit();

    /**
     * Advance the active timed transition by `Dt` seconds. Returns true on the ONE
     * frame a transition completes (Entering->Seated or Exiting->OnFoot) — the edge
     * the adapter uses to possess / unpossess the pawn exactly once. A no-op
     * returning false while OnFoot or Seated. `Dt` is clamped to >= 0.
     */
    bool Update(double Dt, const FParams& Params);

    /** Current flow state. */
    EState State() const { return CurrentState; }

    /** The seat being entered / held / exited, or None when fully OnFoot. */
    int32 Seat() const { return ActiveSeat; }

    /** Convenience: the player is in the seat and the car is drivable. */
    bool IsSeated() const { return CurrentState == EState::Seated; }

    /** Convenience: a get-in or get-out animation is playing right now. */
    bool InTransition() const { return CurrentState == EState::Entering || CurrentState == EState::Exiting; }

    /**
     * 0..1 progress through the current transition (0 when not transitioning, and
     * clamped to 1 if a long frame overshoots). The adapter blends the get-in
     * montage / camera lerp on this.
     */
    double TransitionAlpha(const FParams& Params) const;

private:
    EState CurrentState = EState::OnFoot;
    int32 ActiveSeat = None;
    /** Seconds elapsed in the current Entering / Exiting transition. */
    double Elapsed = 0.0;
};
