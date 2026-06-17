// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * FCrimeWitness — pure crime witness & reporting model (UE 5.7 port of Godot
 * `crime_witness.gd`, class CrimeWitness, RefCounted). The perception layer under the
 * wanted system: a crime only raises heat if someone actually *sees* it, and a witness
 * needs time to call it in. Parity oracle game/tests/unit/test_crime_witness.gd (25 funcs).
 *
 * Two parts (matching Godot):
 *   - Static line-of-sight + heat math (no state): CanWitness / CountWitnesses /
 *     CollectWitnesses / HeatForCrime. Cops pass a wider, longer cone than peds.
 *   - A small stateful in-progress report (an instance): a witness has started dialling;
 *     Tick() runs the timer down, Silence() cancels it.
 *
 * All XZ-plane math is ported faithfully. Godot is Y-up and uses the X/Z plane for the
 * ground; per the Wave-2 "no Z-up remap" rule we keep the SAME axes the oracle uses —
 * the FVector components map 1:1 to Godot's Vector3 (X->X, Y->Y, Z->Z) and "ground"
 * means dropping Y, exactly as Godot _ground() drops y. double precision for float parity.
 *
 * Observer payloads: Godot passes Dictionaries {pos, facing, is_police, node}. Here an
 * observer is the FCrimeObserver struct; CountWitnesses/CollectWitnesses iterate an
 * ordered TArray so witness order is preserved where the oracle observes it. The opaque
 * "node" scene payload (Godot's Node3D ride-along for pending-report bookkeeping) is a
 * Wave-3 concern and is represented as an opaque void* carried through untouched — the
 * subsystem behavior tests never dereference it.
 */

/**
 * One observer for the LOS pass. Mirrors the Godot observer Dictionary. A "malformed"
 * Godot entry (non-dict, or missing pos/facing keys) decoded to a zero facing that can
 * never witness anything; the equivalent here is a default FCrimeObserver (zero facing).
 */
struct GTC_UE5_API FCrimeObserver
{
    FVector Pos = FVector::ZeroVector;
    FVector Facing = FVector::ZeroVector;
    bool bIsPolice = false;
    /** Opaque scene-node ride-along (Wave-3). Carried through CollectWitnesses untouched. */
    void* Node = nullptr;

    FCrimeObserver() = default;
    FCrimeObserver(const FVector& InPos, const FVector& InFacing, bool bInIsPolice = false, void* InNode = nullptr)
        : Pos(InPos), Facing(InFacing), bIsPolice(bInIsPolice), Node(InNode) {}
};

/** Result of CollectWitnesses: the seeing observers (in order) plus whether a cop saw it. */
struct GTC_UE5_API FCollectedWitnesses
{
    TArray<FCrimeObserver> Witnesses;
    bool bPoliceSaw = false;
};

class GTC_UE5_API FCrimeWitness
{
public:
    // --- Static line-of-sight + heat math ----------------------------------

    /**
     * True if `Observer` can see `CrimePos`: within `SightRange` AND inside the forward
     * FOV cone of half-angle `FovRadians`. A zero/degenerate facing can't witness
     * anything; a crime sitting on top of the observer counts as seen.
     */
    static bool CanWitness(
        const FVector& ObserverPos,
        const FVector& ObserverFacing,
        const FVector& CrimePos,
        double SightRange,
        double FovRadians);

    /** How many of `Observers` can witness the crime (peds share one cone). */
    static int32 CountWitnesses(
        const FVector& CrimePos,
        const TArray<FCrimeObserver>& Observers,
        double SightRange,
        double FovRadians);

    /**
     * Partition `Observers` into the ones who actually saw the crime. Police get their
     * own (wider, longer) cone than civilians. The seeing observers are returned in
     * iteration order with their payload carried through untouched.
     */
    static FCollectedWitnesses CollectWitnesses(
        const FVector& CrimePos,
        const TArray<FCrimeObserver>& Observers,
        double PedRange,
        double PedFov,
        double PoliceRange,
        double PoliceFov);

    /**
     * Heat a crime generates given how many people saw it. Zero witnesses -> 0 heat.
     * The first witness carries most of the weight; extra witnesses add diminishing
     * returns via a saturating curve that approaches but never exceeds base_heat.
     */
    static double HeatForCrime(double BaseHeat, int32 WitnessCount);

    // --- Stateful in-progress report ---------------------------------------

    /**
     * A witness has started calling it in; `ReportDelay` seconds until the report lands.
     * A non-positive delay means an instant report (lands on the first tick). Default 3.0.
     */
    explicit FCrimeWitness(double ReportDelay = 3.0)
        : _ReportDelay(FMath::Max(ReportDelay, 0.0))
    {
    }

    /**
     * Advance the call by `Delta` seconds. No effect once silenced or already reported;
     * negative deltas are ignored so time never runs backwards.
     */
    void Tick(double Delta);

    /** True once the witness has finished calling it in (and wasn't silenced first). */
    bool IsReported() const;

    /** How far along the call is, 0..1. Reads 1.0 the instant it completes. */
    double Progress() const;

    /** Cancel the report before it lands. Permanently unreported until Reset(). */
    void Silence() { _bSilenced = true; }

    /** Re-arm a fresh report: clears progress and the silenced flag. */
    void Reset()
    {
        _Elapsed = 0.0;
        _bSilenced = false;
    }

private:
    /** Project a vector onto the XZ ground plane (drop Y), mirroring Godot _ground(). */
    static FVector Ground(const FVector& V) { return FVector(V.X, 0.0, V.Z); }

    double _ReportDelay = 3.0;
    double _Elapsed = 0.0;
    bool _bSilenced = false;
};
