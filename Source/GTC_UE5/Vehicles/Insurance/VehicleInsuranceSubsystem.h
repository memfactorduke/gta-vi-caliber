// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "VehicleInsurance.h"
#include "VehicleInsuranceSubsystem.generated.h"

/** Blueprint-facing result of a claim attempt (mirror of FVehicleInsurance::FClaimResult). */
USTRUCT(BlueprintType)
struct FGtcInsuranceClaimResult
{
    GENERATED_BODY()

    /** True if the claim paid out. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicles|Insurance")
    bool bGranted = false;

    /** Amount to charge the player (0 when denied). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicles|Insurance")
    float Deductible = 0.0f;
};

/** A covered vehicle was reported destroyed and is now claimable -> (VehicleId). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGtcVehicleWrecked, const FString&, VehicleId);
/** A claim paid out -> (VehicleId, Deductible charged). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGtcInsuranceClaimGranted, const FString&, VehicleId, float, Deductible);

/**
 * UVehicleInsuranceSubsystem — fleet-wide vehicle insurance ledger (UE adapter over
 * FVehicleInsurance).
 *
 * A GameInstanceSubsystem because the policy ledger is player-global and should
 * outlive streamed sublevels (the same lifetime as the wanted/economy state).
 * Owns the pure-core model; charging the deductible to PlayerStats and respawning
 * the replacement at a garage is left to the caller / a BP listener on
 * OnClaimGranted.
 */
UCLASS()
class GTC_UE5_API UVehicleInsuranceSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicles|Insurance")
    float BaseDeductible = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicles|Insurance")
    float EscalationPerClaim = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicles|Insurance")
    float CooldownHours = 24.0f;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Vehicles|Insurance")
    FGtcVehicleWrecked OnVehicleWrecked;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Vehicles|Insurance")
    FGtcInsuranceClaimGranted OnClaimGranted;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /** Re-apply the tuning UPROPERTYs to the model (call after changing them at runtime). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Vehicles|Insurance")
    void ApplyTuning();

    UFUNCTION(BlueprintCallable, Category = "GTC|Vehicles|Insurance")
    void Insure(const FString& VehicleId);

    UFUNCTION(BlueprintCallable, Category = "GTC|Vehicles|Insurance")
    void Cancel(const FString& VehicleId);

    /** Report a vehicle destroyed; latches it claimable if insured and broadcasts OnVehicleWrecked. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Vehicles|Insurance")
    void ReportDestroyed(const FString& VehicleId);

    /** Attempt a claim at NowHour; on grant, broadcasts OnClaimGranted. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Vehicles|Insurance")
    FGtcInsuranceClaimResult Claim(const FString& VehicleId, float NowHour);

    UFUNCTION(BlueprintPure, Category = "GTC|Vehicles|Insurance")
    bool IsInsured(const FString& VehicleId) const { return Model.IsInsured(VehicleId); }

    UFUNCTION(BlueprintPure, Category = "GTC|Vehicles|Insurance")
    bool IsClaimable(const FString& VehicleId) const { return Model.IsClaimable(VehicleId); }

    UFUNCTION(BlueprintPure, Category = "GTC|Vehicles|Insurance")
    int32 ClaimsMade(const FString& VehicleId) const { return Model.ClaimsMade(VehicleId); }

    UFUNCTION(BlueprintPure, Category = "GTC|Vehicles|Insurance")
    float QuoteDeductible(const FString& VehicleId) const { return static_cast<float>(Model.QuoteDeductible(VehicleId)); }

    UFUNCTION(BlueprintPure, Category = "GTC|Vehicles|Insurance")
    int32 PolicyCount() const { return Model.PolicyCount(); }

private:
    FVehicleInsurance Model;
};
