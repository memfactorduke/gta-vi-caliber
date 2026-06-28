// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WildlifeBehavior.h"
#include "WildlifeBehaviorComponent.generated.h"

/** Blueprint mirror of FWildlifeBehavior::ETemperament. */
UENUM(BlueprintType)
enum class EGtcTemperament : uint8
{
    Placid,
    Skittish,
    Aggressive
};

/** Blueprint mirror of FWildlifeBehavior::EState. */
UENUM(BlueprintType)
enum class EGtcWildlifeState : uint8
{
    Idle,
    Alert,
    Flee,
    Charge
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGtcOnWildlifeStateChanged, EGtcWildlifeState, State);

/**
 * UWildlifeBehaviorComponent — a Florida-fauna reflex (UE adapter over
 * FWildlifeBehavior). Attach to an animal pawn.
 *
 * Each tick the adapter measures the distance to the nearest threat (usually the
 * player) and whether the animal was just provoked, and calls UpdateBehavior(); the
 * temperament-branched FSM decides idle / alert / flee / charge. The flee/charge MOVE
 * and the hiss/bolt/lunge anim is a BP that reads GetState() / listens to
 * OnWildlifeStateChanged.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UWildlifeBehaviorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWildlifeBehaviorComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Wildlife")
    EGtcTemperament Temperament = EGtcTemperament::Skittish;

    /** Distance (cm) at/under which the animal notices a threat. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Wildlife")
    float NoticeRange = 1500.0f;

    /** Personal space (cm): at/under this an Alert animal flees or charges. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Wildlife")
    float ReactRange = 600.0f;

    /** Seconds the threat must stay beyond NoticeRange before settling to Idle. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Wildlife")
    float CalmSeconds = 4.0f;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Wildlife")
    FGtcOnWildlifeStateChanged OnWildlifeStateChanged;

    /** Advance the brain one tick. DistanceToThreat in cm; bProvoked forces escalation. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Wildlife")
    void UpdateBehavior(float DistanceToThreat, bool bProvoked, float DeltaSeconds);

    UFUNCTION(BlueprintPure, Category = "GTC|Wildlife")
    EGtcWildlifeState GetState() const;

    UFUNCTION(BlueprintPure, Category = "GTC|Wildlife")
    bool IsHostile() const { return Model.IsHostile(); }

    UFUNCTION(BlueprintPure, Category = "GTC|Wildlife")
    bool IsFleeing() const { return Model.IsFleeing(); }

    /** How far through the calm-down the animal is, 0..1. */
    UFUNCTION(BlueprintPure, Category = "GTC|Wildlife")
    float GetCalmProgress() const { return static_cast<float>(Model.CalmProgress(MakeParams())); }

private:
    FWildlifeBehavior Model;
    EGtcWildlifeState LastState = EGtcWildlifeState::Idle;

    FWildlifeBehavior::FParams MakeParams() const;
};
