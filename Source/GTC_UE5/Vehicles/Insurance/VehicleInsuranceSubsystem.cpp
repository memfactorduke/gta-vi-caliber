// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleInsuranceSubsystem.h"

void UVehicleInsuranceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ApplyTuning();
}

void UVehicleInsuranceSubsystem::ApplyTuning()
{
    FVehicleInsurance::FParams Params;
    Params.BaseDeductible = static_cast<double>(BaseDeductible);
    Params.EscalationPerClaim = static_cast<double>(EscalationPerClaim);
    Params.CooldownHours = static_cast<double>(CooldownHours);
    Model.Configure(Params);
}

void UVehicleInsuranceSubsystem::Insure(const FString& VehicleId)
{
    Model.Insure(VehicleId);
}

void UVehicleInsuranceSubsystem::Cancel(const FString& VehicleId)
{
    Model.Cancel(VehicleId);
}

void UVehicleInsuranceSubsystem::ReportDestroyed(const FString& VehicleId)
{
    const bool bWasClaimable = Model.IsClaimable(VehicleId);
    Model.ReportDestroyed(VehicleId);
    // Only announce a fresh wreck (latched flag rising edge).
    if (!bWasClaimable && Model.IsClaimable(VehicleId))
    {
        OnVehicleWrecked.Broadcast(VehicleId);
    }
}

FGtcInsuranceClaimResult UVehicleInsuranceSubsystem::Claim(const FString& VehicleId, float NowHour)
{
    const FVehicleInsurance::FClaimResult Result = Model.Claim(VehicleId, static_cast<double>(NowHour));

    FGtcInsuranceClaimResult Out;
    Out.bGranted = Result.bGranted;
    Out.Deductible = static_cast<float>(Result.Deductible);

    if (Result.bGranted)
    {
        OnClaimGranted.Broadcast(VehicleId, Out.Deductible);
    }
    return Out;
}
