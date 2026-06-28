// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Lockpick.h"
#include "LockpickComponent.generated.h"

/** Blueprint mirror of FLockpick::EOutcome. */
UENUM(BlueprintType)
enum class EGtcLockpickOutcome : uint8
{
    Picking,
    Unlocked,
    Broken
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGtcOnLockUnlocked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGtcOnPickBroken);

/**
 * ULockpickComponent — the lock-tumbler minigame (UE adapter over FLockpick).
 *
 * Attach to a door / safe / car lock (or drive from the player). StartPick() begins
 * an attempt with the lock's hidden sweet spot; the input layer calls
 * UpdatePick(PickAngle, Tension, Dt) each frame; AlignmentAt() feeds a "warmer" cue.
 * Fires OnLockUnlocked / OnPickBroken.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API ULockpickComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULockpickComponent();

    /** The hidden angle that turns the lock, 0..1. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Lockpick")
    float SweetSpot = 0.5f;

    /** Half-width of the sweet spot. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Lockpick")
    float Tolerance = 0.08f;

    /** Turn progress per second at perfect alignment and full tension. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Lockpick")
    float TurnRatePerSec = 0.8f;

    /** Pick stress per second at zero alignment and full tension. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Lockpick")
    float StressRatePerSec = 1.5f;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Lockpick")
    FGtcOnLockUnlocked OnLockUnlocked;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Lockpick")
    FGtcOnPickBroken OnPickBroken;

    /** Begin a fresh attempt using the current tuning. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Lockpick")
    void StartPick();

    /** Advance one tick with PickAngle (0..1) and Tension (0..1) over DeltaSeconds. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Lockpick")
    void UpdatePick(float PickAngle, float Tension, float DeltaSeconds);

    /** How aligned Angle is with the hidden sweet spot, 0..1 (for the "warmer" cue). */
    UFUNCTION(BlueprintPure, Category = "GTC|Lockpick")
    float AlignmentAt(float Angle) const { return static_cast<float>(Model.AlignmentAt(Angle)); }

    UFUNCTION(BlueprintPure, Category = "GTC|Lockpick")
    EGtcLockpickOutcome GetOutcome() const;

    UFUNCTION(BlueprintPure, Category = "GTC|Lockpick")
    bool IsPicking() const { return Model.IsPicking(); }

    UFUNCTION(BlueprintPure, Category = "GTC|Lockpick")
    float GetProgress() const { return static_cast<float>(Model.Progress()); }

    UFUNCTION(BlueprintPure, Category = "GTC|Lockpick")
    float GetStress() const { return static_cast<float>(Model.Stress()); }

private:
    FLockpick Model;
};
