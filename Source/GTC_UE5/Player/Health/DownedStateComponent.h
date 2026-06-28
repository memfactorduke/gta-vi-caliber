// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DownedState.h"
#include "DownedStateComponent.generated.h"

/** Blueprint mirror of FDownedState::EState. */
UENUM(BlueprintType)
enum class EGtcDownedState : uint8
{
    Up,
    Downed,
    Dead
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGtcOnDowned);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGtcOnRevived);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGtcOnDownedDied);

/**
 * UDownedStateComponent — the downed / bleed-out grace window (UE adapter over
 * FDownedState). Sits between health and death for the co-op revive beat.
 *
 * Wire it on the player pawn: on "health hit 0" call GoDown(); a partner reaching
 * you calls Revive(); an enemy execution calls FinishOff(). The component ticks the
 * bleed clock while Downed and fires OnDowned / OnRevived / OnDied for the HUD,
 * ragdoll, and the partner-bond nudge.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UDownedStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDownedStateComponent();

    /** Seconds from going down to dead if nobody revives you. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Player|Downed")
    float BleedOutSeconds = 20.0f;

    /** Downs survivable per life; the next drop after this is an instant kill. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Player|Downed")
    int32 MaxDowns = 2;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Player|Downed")
    FGtcOnDowned OnDowned;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Player|Downed")
    FGtcOnRevived OnRevived;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Player|Downed")
    FGtcOnDownedDied OnDied;

    /** Health ran out — drop the player (or instant-kill if the downs budget is spent). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Downed")
    void GoDown();

    /** A partner revived you: Downed -> Up. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Downed")
    void Revive();

    /** An enemy executed you while down: Downed -> Dead. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Downed")
    void FinishOff();

    /** Respawn / safehouse: back to Up with the downs budget restored. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Downed")
    void ResetDownedState();

    /** Re-apply the tuning UPROPERTYs to the model (also resets to Up). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Downed")
    void ApplyTuning();

    UFUNCTION(BlueprintPure, Category = "GTC|Player|Downed")
    EGtcDownedState GetState() const;

    UFUNCTION(BlueprintPure, Category = "GTC|Player|Downed")
    bool IsUp() const { return Model.IsUp(); }

    UFUNCTION(BlueprintPure, Category = "GTC|Player|Downed")
    bool IsDowned() const { return Model.IsDowned(); }

    UFUNCTION(BlueprintPure, Category = "GTC|Player|Downed")
    bool IsDead() const { return Model.IsDead(); }

    UFUNCTION(BlueprintPure, Category = "GTC|Player|Downed")
    int32 GetDownsTaken() const { return Model.DownsTaken(); }

    /** Fraction of the bleed-out elapsed, 0..1 (0 when not Downed). */
    UFUNCTION(BlueprintPure, Category = "GTC|Player|Downed")
    float GetBleedProgress() const { return static_cast<float>(Model.BleedProgress()); }

    /** Seconds left before bleed-out kills you (0 when not Downed). */
    UFUNCTION(BlueprintPure, Category = "GTC|Player|Downed")
    float GetTimeLeft() const { return static_cast<float>(Model.TimeLeft()); }

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    FDownedState Model;
};
