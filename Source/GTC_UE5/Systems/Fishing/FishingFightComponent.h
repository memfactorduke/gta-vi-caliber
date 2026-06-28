// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FishingFight.h"
#include "FishingFightComponent.generated.h"

/** Blueprint mirror of FFishingFight::EOutcome. */
UENUM(BlueprintType)
enum class EGtcFishingOutcome : uint8
{
    Fighting,
    Landed,
    Lost
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGtcOnFishLanded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGtcOnFishLost);

/**
 * UFishingFightComponent — the reel-vs-line fight (UE adapter over FFishingFight).
 *
 * Lives on the player. StartFight() begins a fresh contest; the rod/input layer
 * calls UpdateFight(ReelInput, Dt) each frame with the player's reel effort; the
 * component fires OnFishLanded / OnFishLost when the fight resolves. Cast/bite
 * detection and awarding the catch is the caller's job.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UFishingFightComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFishingFightComponent();

    /** Tension at which the line snaps. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Fishing")
    float LineStrength = 1.0f;

    /** Distance reeled in per second at full reel. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Fishing")
    float ReelRate = 0.25f;

    /** Distance the fish takes per second at full pull. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Fishing")
    float PullBack = 0.15f;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Fishing")
    FGtcOnFishLanded OnFishLanded;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Fishing")
    FGtcOnFishLost OnFishLost;

    /** Begin a fresh fight using the current tuning. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Fishing")
    void StartFight();

    /** Advance the fight one tick with ReelInput (0..1) over DeltaSeconds. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Fishing")
    void UpdateFight(float ReelInput, float DeltaSeconds);

    UFUNCTION(BlueprintPure, Category = "GTC|Fishing")
    EGtcFishingOutcome GetOutcome() const;

    UFUNCTION(BlueprintPure, Category = "GTC|Fishing")
    bool IsFighting() const { return Model.IsFighting(); }

    /** Remaining distance to land the fish, 0..1 (0 = at the boat). */
    UFUNCTION(BlueprintPure, Category = "GTC|Fishing")
    float GetDistance() const { return static_cast<float>(Model.Distance()); }

    /** The fish's current pull strength, 0..1. */
    UFUNCTION(BlueprintPure, Category = "GTC|Fishing")
    float GetPullStrength() const { return static_cast<float>(Model.PullStrength()); }

    /** The line tension from the last update, 0.. (>= LineStrength means snapped). */
    UFUNCTION(BlueprintPure, Category = "GTC|Fishing")
    float GetTension() const { return static_cast<float>(Model.Tension()); }

private:
    FFishingFight Model;
};
