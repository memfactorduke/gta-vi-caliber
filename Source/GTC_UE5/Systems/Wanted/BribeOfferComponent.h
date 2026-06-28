// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BribeOffer.h"
#include "BribeOfferComponent.generated.h"

/** Blueprint-facing result of a bribe attempt (mirror of FBribeOffer::FResult). */
USTRUCT(BlueprintType)
struct FGtcBribeResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "GTC|Bribe")
    bool bAccepted = false;

    /** The quoted cost (filled whether or not accepted). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Bribe")
    float Cost = 0.0f;

    /** Resulting wanted level. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Bribe")
    int32 StarsAfter = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGtcOnBribeAccepted, float, Cost, int32, StarsAfter);

/**
 * UBribeOfferComponent — greasing a cop to cool the heat (UE adapter over FBribeOffer).
 *
 * Lives on the player. Ticks the cooldown (so going straight forgets the greed),
 * quotes a bribe at the current heat, and resolves an attempt. Charging the player
 * and lowering the WantedSubsystem heat is the caller's job (listen to
 * OnBribeAccepted). Produces for the wanted system; never reaches into it.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UBribeOfferComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBribeOfferComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Bribe")
    int32 MaxBribableStars = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Bribe")
    float CostPerStar = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Bribe")
    float GreedPerBribe = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Bribe")
    float CooldownHours = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Bribe")
    int32 StarsReducedPerBribe = 1;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Bribe")
    FGtcOnBribeAccepted OnBribeAccepted;

    UFUNCTION(BlueprintCallable, Category = "GTC|Bribe")
    void ApplyTuning();

    /** What a bribe at Stars of heat costs right now (0 if not bribable). */
    UFUNCTION(BlueprintPure, Category = "GTC|Bribe")
    float QuoteCost(int32 Stars) const { return static_cast<float>(Model.QuoteCost(Stars)); }

    /** True if a bribe is on the table at Stars and affordable with Cash. */
    UFUNCTION(BlueprintPure, Category = "GTC|Bribe")
    bool CanBribe(int32 Stars, float Cash) const { return Model.CanBribe(Stars, static_cast<double>(Cash)); }

    /** Attempt a bribe; on accept, fires OnBribeAccepted. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Bribe")
    FGtcBribeResult Bribe(int32 Stars, float Cash);

    UFUNCTION(BlueprintPure, Category = "GTC|Bribe")
    int32 GetRecentBribes() const { return Model.RecentBribes(); }

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    FBribeOffer Model;
};
