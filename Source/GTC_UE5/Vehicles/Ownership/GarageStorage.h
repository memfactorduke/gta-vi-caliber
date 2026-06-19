// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure garage / vehicle-storage model — the GTC "park it in your garage" system:
 * owned vehicles (by id) are stored across one or more garages, retrieved when the
 * player wants to drive them, and impounded by the cops when abandoned.
 *
 * STATEFUL instance, no nodes, no wallet coupling: recovering an impounded vehicle
 * resolves against a balance the caller passes in and returns the result so the caller
 * applies the spend — same headless, unit-testable pattern as VehicleModShop.
 *
 * A vehicle is in exactly one of three states: out (the default — drivable / in the
 * world), stored (parked in a specific garage), or impounded (seized by the cops).
 * Each garage holds up to Capacity() vehicles. Garages are created lazily on first
 * store, so any garage id is valid and starts empty with the default capacity.
 *
 * No RNG; fully deterministic.
 *
 * Parity note: an insertion-ordered map/Array iteration is insertion-ordered and Contents()
 * relies on it. Garage contents and impound order are stored as ordered TArrays; a
 * companion TSet provides O(1) impound lookup.
 */
class GTC_UE5_API GarageStorage
{
public:
    /** Outcome of a RecoverFromImpound attempt. */
    struct FRecoverResult
    {
        bool bSuccess = false;
        int32 Cost = 0;
        int32 NewBalance = 0;
        FString Reason;
    };

    /** Construct with a per-garage capacity (floored at 1). */
    explicit GarageStorage(int32 CapacityPerGarage = 4);

    // --- Storage ----------------------------------------------------------

    /**
     * Park a vehicle in a garage. Fails (state unchanged) when the id is empty, the
     * garage is full, the vehicle is already stored somewhere, or it is impounded.
     */
    bool Store(const FString& GarageId, const FString& VehicleId);

    /** Pull a vehicle out of a garage (it becomes out). Fails if not stored in that garage. */
    bool Retrieve(const FString& GarageId, const FString& VehicleId);

    // --- Queries ----------------------------------------------------------

    bool IsStored(const FString& VehicleId) const;

    /** The garage a vehicle is parked in, or "" if out / impounded / unknown. */
    FString GarageOf(const FString& VehicleId) const;

    /** Stored vehicle ids in a garage (a COPY, in insertion order; empty if unknown). */
    TArray<FString> Contents(const FString& GarageId) const;

    int32 CountIn(const FString& GarageId) const;

    /** Remaining slots in a garage (full capacity for an unknown / empty garage). */
    int32 FreeSpace(const FString& GarageId) const;

    int32 Capacity() const;

    /** Total vehicles parked across every garage. */
    int32 TotalStored() const;

    // --- Impound ----------------------------------------------------------

    /** Flag a vehicle as impounded; pulls it out of any garage first. No-op on empty id. */
    void Impound(const FString& VehicleId);

    bool IsImpounded(const FString& VehicleId) const;

    /**
     * Pay the impound fee to release a vehicle against a wallet balance. On success the
     * vehicle becomes out (not re-stored). Fails (state unchanged) when not impounded
     * or funds fall short. Fee is floored at 0.
     */
    FRecoverResult RecoverFromImpound(const FString& VehicleId, int32 Balance, int32 Fee);

    // --- Persistence ------------------------------------------------------

    /** Snapshot capacity, garage contents (ordered), and impound list (ordered). */
    void Serialize(int32& OutCapacity, TArray<TPair<FString, TArray<FString>>>& OutGarages, TArray<FString>& OutImpounded) const;

    /**
     * Rebuild from a Serialize() snapshot. Over-capacity garages are truncated, empty
     * garages are dropped, empty ids are skipped. A capacity < 1 is ignored.
     */
    void Restore(int32 InCapacity, const TArray<TPair<FString, TArray<FString>>>& InGarages, const TArray<FString>& InImpounded);

    /** Empty every garage and clear all impounds. */
    void Reset();

private:
    struct FGarage
    {
        FString Id;
        TArray<FString> Contents;
    };

    int32 CapacityPerGarage = 4;

    /** Insertion-ordered garages. */
    TArray<FGarage> Garages;
    /** Garage id -> index into Garages. */
    TMap<FString, int32> GarageIndex;

    /** Impounded vehicle ids in insertion order. */
    TArray<FString> ImpoundedOrder;
    /** Impounded vehicle ids for O(1) lookup. */
    TSet<FString> ImpoundedSet;

    const FGarage* FindGarage(const FString& GarageId) const;
    FGarage* FindGarage(const FString& GarageId);
    FGarage& FindOrAddGarage(const FString& GarageId);

    void RemoveImpound(const FString& VehicleId);
};
