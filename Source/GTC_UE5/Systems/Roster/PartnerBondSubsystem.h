// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PartnerBond.h"
#include "PartnerBondSubsystem.generated.h"

/** Blueprint mirror of FPartnerBond::ETier. */
UENUM(BlueprintType)
enum class EGtcBondTier : uint8
{
    Estranged,
    Wary,
    Solid,
    RideOrDie
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGtcOnBondChanged, float, Bond);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGtcOnBondTierChanged, EGtcBondTier, Tier);

/**
 * UPartnerBondSubsystem — the bond between the two leads (UE adapter over FPartnerBond).
 *
 * A GameInstanceSubsystem because the relationship is player-global and persists
 * across streamed sublevels. Warm() on a shared win / revive, Cool() on a betrayal,
 * Advance(Hours) for neglect drift. Co-op perks gate on GetTier(); listen to
 * OnBondTierChanged.
 */
UCLASS()
class GTC_UE5_API UPartnerBondSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Roster|Bond")
    float StartBond = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Roster|Bond")
    float Baseline = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Roster|Bond")
    float WarmGain = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Roster|Bond")
    float CoolLoss = 0.20f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Roster|Bond")
    float DecayPerHour = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Roster|Bond")
    float WaryAt = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Roster|Bond")
    float SolidAt = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Roster|Bond")
    float RideOrDieAt = 0.80f;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Roster|Bond")
    FGtcOnBondChanged OnBondChanged;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Roster|Bond")
    FGtcOnBondTierChanged OnBondTierChanged;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /** Re-apply the tuning UPROPERTYs (resets the bond to StartBond). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Roster|Bond")
    void ApplyTuning();

    /** A warming act of weight Strength; returns the bond change. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Roster|Bond")
    float Warm(float Strength);

    /** A cooling act of weight Strength; returns the (negative) bond change. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Roster|Bond")
    float Cool(float Strength);

    /** Advance Hours of neglect (the bond drifts toward Baseline). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Roster|Bond")
    void Advance(float Hours);

    UFUNCTION(BlueprintPure, Category = "GTC|Roster|Bond")
    float GetBond() const { return static_cast<float>(Model.Bond()); }

    UFUNCTION(BlueprintPure, Category = "GTC|Roster|Bond")
    EGtcBondTier GetTier() const;

private:
    FPartnerBond Model;
    EGtcBondTier LastTier = EGtcBondTier::Estranged;

    /** Broadcast bond + tier change after a mutation. */
    void BroadcastChange();
};
