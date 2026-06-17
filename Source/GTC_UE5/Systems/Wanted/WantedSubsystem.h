// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WantedSystem.h"
#include "CrimeWitness.h"
#include "WantedEvasion.h"
#include "WantedSubsystem.generated.h"

/**
 * UWantedSubsystem — UE 5.7 port of the reference self-wiring `wanted_tracker.gd` (class
 * WantedTracker, a Node). Owns the three pure models (FWantedSystem, FCrimeWitness
 * report timers, FWantedEvasion) and coordinates them: crimes are perception-gated
 * through CrimeWitness math, heat lands in WantedSystem, evasion tracks the go-cold
 * countdown. Exposes Stars()/IsWanted() for police AI and HUDs.
 *
 * Why a GameInstanceSubsystem: the the reference WantedTracker is a persistent per-world Node
 * joined to group "wanted"; the player's heat is a player-global, save-persisted value
 * that should outlive individual streamed sublevels. A GameInstanceSubsystem has that
 * lifetime. (If a future design needs per-world reset semantics this can move to a
 * WorldSubsystem — flagged as the one lifecycle choice here.)
 *
 * OWNERSHIP MODEL: the subsystem OWNS its FWantedSystem + FWantedEvasion + the pending
 * witness-report timers, and exposes them only via getters/driver methods. It owns its
 * OWN state and does NOT depend on any other Wave-2 sibling type (PlayerStats, Arrest,
 * Health) — this branch is fully decoupled.
 *
 * DEFERRED (Wave-3 adapters — documented, NOT implemented or depended on here):
 *   - WeaponController "crime_committed" signal binding (Godot _bind / _on_crime).
 *   - Scene-graph observer gathering (Godot _gather_observers walks groups "pedestrians"
 *     / "police", reads each Rig's forward -Z). Here the caller supplies an explicit
 *     TArray<FCrimeObserver>; the Node3D/AI-Perception/LOS source is Wave-3.
 *   - Police spawning, disguise wiring, the is_dead()/witness-down liveness probe.
 * The pure motion/perception math is preserved through explicit driver methods so the
 * ported behavior stays headless-testable; the scene adapter feeds it in Wave-3.
 *
 * stars_changed: the the reference signal fires when the quantised star count changes. Mirrored
 * here as FOnStarsChanged (broadcast from RefreshStars when the cached star count moves).
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStarsChanged, int32, Stars);

UCLASS()
class GTC_UE5_API UWantedSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // --- Tuning (Godot @export defaults) -----------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wanted")
    double WoundHeat = 0.7;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wanted")
    double KillHeat = 2.5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wanted")
    double DecayRate = 0.35;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wanted")
    double HeatCap = 20.0;

    /** Civilian perception: how far / how wide (half-angle, degrees) a ped sees a crime. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wanted")
    double PedSightRange = 24.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wanted")
    double PedFovDegrees = 70.0;

    /** Police perception: trained spotters get a longer, wider cone. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wanted")
    double PoliceSightRange = 40.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wanted")
    double PoliceFovDegrees = 100.0;

    /** Seconds a civilian witness takes to call a crime in. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wanted")
    double ReportDelaySec = 2.5;

    /** Seconds after a crime during which the player still counts as "actively committing". */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wanted")
    double CrimeActiveWindow = 0.6;

    /** Fires when the quantised star count changes (the reference stars_changed). */
    UPROPERTY(BlueprintAssignable, Category = "Wanted")
    FOnStarsChanged OnStarsChanged;

    // UGameInstanceSubsystem lifecycle.
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // --- Queries (delegate to the owned models) ----------------------------

    int32 Stars() const;
    bool IsWanted() const;

    /** Const access to the owned heat model (ownership stays with the subsystem). */
    const FWantedSystem& GetWantedSystem() const { return _Wanted; }
    /** Const access to the owned evasion state machine. */
    const FWantedEvasion& GetEvasion() const { return _Evasion; }
    /** Number of in-flight witness reports still dialling. */
    int32 PendingReportCount() const { return _PendingReports.Num(); }

    // --- Crime drivers (ported from the WantedTracker Node) ----------------

    /**
     * Direct, unconditional heat — the crime is taken as already reported (scripted
     * events, probes). Mirrors the reference report_crime. Re-arms the active-committing window.
     */
    void ReportCrime(bool bKilled);

    /**
     * A crime at a world position that someone must have SEEN for heat to land. Runs the
     * CrimeWitness LOS pass over the supplied observers (the Wave-3 scene adapter gathers
     * them). A cop witness radios it in instantly; civilian witnesses queue a delayed
     * report. Mirrors the reference report_witnessed_crime. Re-arms the active-committing window.
     */
    void ReportWitnessedCrime(bool bKilled, const FVector& CrimePos, const TArray<FCrimeObserver>& Observers);

    /**
     * Advance one frame: cool heat (held while still actively committing), tick in-flight
     * reports (completed ones convert to heat), and refresh stars. Mirrors Godot _process.
     * Witness-liveness silencing (Godot _all_witnesses_down) is a Wave-3 scene probe and
     * is NOT driven here; reports simply tick to completion.
     */
    void TickFrame(double Delta);

    // --- Evasion driver ----------------------------------------------------

    /** Feed line-of-sight + delta into the owned evasion state machine. */
    void UpdateEvasion(bool bSeenByPolice, double Delta) { _Evasion.Update(bSeenByPolice, Delta); }

    // --- State management --------------------------------------------------

    /** Wipe all heat and drop in-flight reports (death/arrest). Mirrors the reference clear(). */
    void Clear();

    /** Snapshot heat for the save system (the reference serialize). */
    double SerializeHeat() const { return _Wanted.Heat; }

    /** Restore heat from a snapshot, floored at 0 (the reference restore). */
    void RestoreHeat(double Heat);

private:
    /** Re-evaluate the quantised star count and broadcast OnStarsChanged on change. */
    void RefreshStars();

    /** One queued civilian witness report: a dialling timer plus the heat it will apply. */
    struct FPendingReport
    {
        FCrimeWitness Report;
        double Heat = 0.0;
        FPendingReport(double Delay, double InHeat) : Report(Delay), Heat(InHeat) {}
    };

    FWantedSystem _Wanted{0.35, 20.0};
    FWantedEvasion _Evasion;
    TArray<FPendingReport> _PendingReports;
    double _CrimeTimer = 0.0;
    int32 _Stars = -1;
};
