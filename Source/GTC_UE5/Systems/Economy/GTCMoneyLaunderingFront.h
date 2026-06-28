// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoneyLaundering.h"
#include "GTCMoneyLaunderingFront.generated.h"

/** Clean cash washed out this tick -> (CleanAmount). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGtcOnCleanCashProduced, float, CleanAmount);

/**
 * AGTCMoneyLaunderingFront — a laundromat front (UE actor over FMoneyLaundering).
 *
 * Place one as a business location. Deposit() drops hot cash into the wash; it
 * trickles out clean over in-game hours minus a cut. Banking the clean payout into
 * PlayerStats is the caller's job (listen to OnCleanCashProduced). Real-time ticking
 * uses GameSecondsPerHour to map wall-clock seconds to in-game hours.
 */
UCLASS()
class GTC_UE5_API AGTCMoneyLaunderingFront : public AActor
{
    GENERATED_BODY()

public:
    AGTCMoneyLaunderingFront();

    /** Fraction of laundered cash kept as the fee. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Economy")
    float Cut = 0.15f;

    /** Maximum dirty cash laundered per in-game hour. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Economy")
    float ThroughputPerHour = 1000.0f;

    /** Maximum dirty cash the front can hold at once. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Economy")
    float Capacity = 50000.0f;

    /** Real seconds that map to one in-game hour for the wash tick (0 = don't auto-tick). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Economy")
    float GameSecondsPerHour = 60.0f;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Economy")
    FGtcOnCleanCashProduced OnCleanCashProduced;

    UFUNCTION(BlueprintCallable, Category = "GTC|Economy")
    void ApplyTuning();

    /** Drop dirty cash into the wash; returns the amount actually taken. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Economy")
    float Deposit(float DirtyAmount);

    /** Run the wash for InGameHours and return the clean cash produced. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Economy")
    float AdvanceHours(float InGameHours);

    UFUNCTION(BlueprintPure, Category = "GTC|Economy")
    float GetDirtyBalance() const { return static_cast<float>(Model.DirtyBalance()); }

    UFUNCTION(BlueprintPure, Category = "GTC|Economy")
    float GetSpaceRemaining() const { return static_cast<float>(Model.SpaceRemaining()); }

    UFUNCTION(BlueprintPure, Category = "GTC|Economy")
    bool IsIdle() const { return Model.IsIdle(); }

    UFUNCTION(BlueprintPure, Category = "GTC|Economy")
    float GetTotalFees() const { return static_cast<float>(Model.TotalFees()); }

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    FMoneyLaundering Model;
};
