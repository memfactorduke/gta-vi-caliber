// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleInsurance.h"

const FVehicleInsurance::FPolicy* FVehicleInsurance::Find(const FString& VehicleId) const
{
    for (const FPolicy& Policy : Policies)
    {
        if (Policy.VehicleId == VehicleId)
        {
            return &Policy;
        }
    }
    return nullptr;
}

FVehicleInsurance::FPolicy* FVehicleInsurance::Find(const FString& VehicleId)
{
    return const_cast<FPolicy*>(static_cast<const FVehicleInsurance*>(this)->Find(VehicleId));
}

FVehicleInsurance::FPolicy& FVehicleInsurance::FindOrAdd(const FString& VehicleId)
{
    if (FPolicy* Existing = Find(VehicleId))
    {
        return *Existing;
    }
    FPolicy Policy;
    Policy.VehicleId = VehicleId;
    const int32 Index = Policies.Add(MoveTemp(Policy));
    return Policies[Index];
}

void FVehicleInsurance::Insure(const FString& VehicleId)
{
    FindOrAdd(VehicleId).bInsured = true;
}

void FVehicleInsurance::Cancel(const FString& VehicleId)
{
    if (FPolicy* Policy = Find(VehicleId))
    {
        Policy->bInsured = false;
        // History (Claims/LastClaimHour) is retained; a pending wreck can't be claimed
        // while cancelled, but we leave the flag so re-insuring restores it.
    }
}

void FVehicleInsurance::ReportDestroyed(const FString& VehicleId)
{
    if (FPolicy* Policy = Find(VehicleId))
    {
        if (Policy->bInsured)
        {
            Policy->bWreckPending = true; // latched; cleared only by a successful claim
        }
    }
}

bool FVehicleInsurance::IsInsured(const FString& VehicleId) const
{
    const FPolicy* Policy = Find(VehicleId);
    return Policy != nullptr && Policy->bInsured;
}

bool FVehicleInsurance::IsClaimable(const FString& VehicleId) const
{
    const FPolicy* Policy = Find(VehicleId);
    return Policy != nullptr && Policy->bInsured && Policy->bWreckPending;
}

int32 FVehicleInsurance::ClaimsMade(const FString& VehicleId) const
{
    const FPolicy* Policy = Find(VehicleId);
    return Policy ? Policy->Claims : 0;
}

double FVehicleInsurance::QuoteDeductible(const FString& VehicleId) const
{
    const FPolicy* Policy = Find(VehicleId);
    if (Policy == nullptr)
    {
        return 0.0;
    }
    const double Base = FMath::Max(0.0, Params.BaseDeductible);
    const double Escalation = FMath::Max(0.0, Params.EscalationPerClaim);
    return Base * (1.0 + Escalation * Policy->Claims);
}

FVehicleInsurance::FClaimResult FVehicleInsurance::Claim(const FString& VehicleId, double NowHour)
{
    FClaimResult Result;
    FPolicy* Policy = Find(VehicleId);
    if (Policy == nullptr || !Policy->bInsured || !Policy->bWreckPending)
    {
        return Result; // unknown, uninsured, or nothing to claim
    }

    // The first claim has no wait; later ones must clear the cooldown.
    if (Policy->bEverClaimed)
    {
        const double Cooldown = FMath::Max(0.0, Params.CooldownHours);
        if (NowHour - Policy->LastClaimHour < Cooldown)
        {
            return Result; // still cooling down
        }
    }

    Result.Deductible = QuoteDeductible(VehicleId); // priced off the claims so far
    Result.bGranted = true;

    Policy->bWreckPending = false;
    ++Policy->Claims;
    Policy->bEverClaimed = true;
    Policy->LastClaimHour = NowHour;
    return Result;
}
