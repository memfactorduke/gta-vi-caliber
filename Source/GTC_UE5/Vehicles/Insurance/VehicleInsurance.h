// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Vehicle insurance and recovery — the safety net under the cars you care about.
 * The vehicle economy can store cars (GarageStorage), fence them (ChopShop) and
 * track their condition, but when your prized ride gets blown up in a chase it's
 * just gone. FVehicleInsurance is the recover-it-for-a-fee loop: insure a vehicle,
 * and when it's destroyed you can claim a replacement for a deductible the
 * adapter charges to the player.
 *
 * Two rules keep it from being a free respawn button. The "destroyed" state is a
 * LATCHED claimable flag, set by a destroy report and cleared only by a successful
 * claim — so a wreck event firing twice (physics, a save reload) can't be
 * double-claimed. And the deductible ESCALATES with prior claims while a COOLDOWN
 * gates back-to-back claims, so writing off cars repeatedly gets steadily more
 * expensive and can't be spammed. Each policy therefore carries its own history
 * (claim count + last-claim time), not just an insured bit.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject.
 * The same events in the same order always yield the same policies — unit-tested
 * headless (Tests/VehicleInsuranceTest.cpp, prefix GTC.Vehicles.Insurance).
 *
 * PURE-CORE boundary: detecting a vehicle's destruction (Vehicles/Condition),
 * charging the deductible to PlayerStats, and respawning the replacement at a
 * garage is the DEFERRED adapter and is NOT covered by the tests. Units: the
 * deductible is money; the cooldown and the claim time are in hours.
 */
struct FVehicleInsurance
{
    /** Tuning for what a claim costs and how often you can make one. */
    struct FParams
    {
        /** Deductible for a first claim. */
        double BaseDeductible = 500.0;
        /** Each PRIOR claim raises the deductible by this fraction of the base. */
        double EscalationPerClaim = 0.5;
        /** Hours that must pass between successful claims on the same vehicle. */
        double CooldownHours = 24.0;
    };

    /** The outcome of a claim attempt. */
    struct FClaimResult
    {
        bool bGranted = false;   // true if the claim paid out
        double Deductible = 0.0; // amount to charge the player (0 when denied)
    };

    /** Install the tuning. Existing policies are kept. */
    void Configure(const FParams& InParams) { Params = InParams; }

    /** Open (or re-open) a policy on `VehicleId`. Idempotent; clears any cancelled state. */
    void Insure(const FString& VehicleId);

    /** Cancel the policy on `VehicleId` (keeps its claim history; just stops covering it). */
    void Cancel(const FString& VehicleId);

    /**
     * Report `VehicleId` destroyed. If it's insured, latches it claimable; otherwise a
     * no-op. Reporting an already-claimable wreck again changes nothing.
     */
    void ReportDestroyed(const FString& VehicleId);

    /** True if a (non-cancelled) policy covers the vehicle. */
    bool IsInsured(const FString& VehicleId) const;

    /** True if the vehicle is insured AND currently has an unclaimed wreck. */
    bool IsClaimable(const FString& VehicleId) const;

    /** Successful claims made on the vehicle so far. */
    int32 ClaimsMade(const FString& VehicleId) const;

    /**
     * What the NEXT claim on `VehicleId` would cost given its claim history:
     * BaseDeductible * (1 + EscalationPerClaim * priorClaims). 0 for an unknown vehicle.
     */
    double QuoteDeductible(const FString& VehicleId) const;

    /**
     * Attempt to claim a replacement at time `NowHour`. Granted only if the vehicle is
     * insured, has an unclaimed wreck, and the cooldown since its last claim has
     * elapsed (the first claim has no wait). On grant: returns the deductible, clears
     * the wreck, bumps the claim count, and stamps the claim time. Denied claims change
     * nothing.
     */
    FClaimResult Claim(const FString& VehicleId, double NowHour);

    /** Number of policies on record (including cancelled ones that retain history). */
    int32 PolicyCount() const { return Policies.Num(); }

private:
    struct FPolicy
    {
        FString VehicleId;
        bool bInsured = false;
        bool bWreckPending = false;
        int32 Claims = 0;
        double LastClaimHour = 0.0;
        bool bEverClaimed = false;
    };

    const FPolicy* Find(const FString& VehicleId) const;
    FPolicy* Find(const FString& VehicleId);
    FPolicy& FindOrAdd(const FString& VehicleId);

    FParams Params;
    TArray<FPolicy> Policies;
};
