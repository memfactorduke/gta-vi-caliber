// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HostageStandoff.h"
#include "HostageStandoffComponent.generated.h"

/** Blueprint mirror of FHostageStandoff::EState. */
UENUM(BlueprintType)
enum class EGtcHostageState : uint8
{
    None,
    Holding,
    BrokeFree,
    Released
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGtcOnHostageGrabbed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGtcOnHostageBrokeFree);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGtcOnHostageReleased);

/**
 * UHostageStandoffComponent — grabbing a human shield (UE adapter over
 * FHostageStandoff). Lives on the player.
 *
 * Grab() snatches a bystander; UpdateHold(bMoving, Dt) runs the struggle each frame;
 * Intimidate() shoves the struggle back down with diminishing returns; Release() lets
 * them go. Police "hold fire" is the adapter reading IsShieldActive(). Fires
 * OnHostageGrabbed / OnHostageBrokeFree / OnHostageReleased.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UHostageStandoffComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UHostageStandoffComponent();

    /** Struggle gained per second while held and standing still. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Hostage")
    float StrugglePerSec = 0.06f;

    /** Multiplier on struggle while dragging the hostage around. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Hostage")
    float MoveStruggleMult = 2.0f;

    /** Struggle the first intimidation pushes down. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Hostage")
    float IntimidateRelief = 0.4f;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Hostage")
    FGtcOnHostageGrabbed OnHostageGrabbed;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Hostage")
    FGtcOnHostageBrokeFree OnHostageBrokeFree;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Hostage")
    FGtcOnHostageReleased OnHostageReleased;

    /** Apply tuning to the model (call before Grab if tuning changed at runtime). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Hostage")
    void ApplyTuning();

    /** Grab a fresh hostage: shield up. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Hostage")
    void Grab();

    /** Advance the struggle one tick; bMoving = the player is dragging the hostage. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Hostage")
    void UpdateHold(bool bMoving, float DeltaSeconds);

    /** Shout the hostage down; returns the struggle relief applied. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Hostage")
    float Intimidate();

    /** Let the hostage go. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Hostage")
    void Release();

    UFUNCTION(BlueprintPure, Category = "GTC|Hostage")
    EGtcHostageState GetState() const;

    UFUNCTION(BlueprintPure, Category = "GTC|Hostage")
    bool IsHolding() const { return Model.IsHolding(); }

    UFUNCTION(BlueprintPure, Category = "GTC|Hostage")
    bool IsShieldActive() const { return Model.IsShieldActive(); }

    /** How close the hostage is to breaking loose, 0..1. */
    UFUNCTION(BlueprintPure, Category = "GTC|Hostage")
    float GetStruggleFraction() const { return static_cast<float>(Model.StruggleFraction()); }

    UFUNCTION(BlueprintPure, Category = "GTC|Hostage")
    int32 GetIntimidations() const { return Model.Intimidations(); }

private:
    FHostageStandoff Model;
};
