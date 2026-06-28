// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BirdFlock.h"
#include "GTCBirdFlock.generated.h"

/** Blueprint mirror of FBirdFlock::EPhase. */
UENUM(BlueprintType)
enum class EGtcBirdPhase : uint8
{
    Perched,
    Startled,
    Wheeling,
    Settling
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGtcOnFlockPhaseChanged, EGtcBirdPhase, Phase);

/**
 * AGTCBirdFlock — a flock of birds as one collective agent (UE actor over FBirdFlock).
 *
 * Place one at a perch (wires, a pier). Startle() on a nearby gunshot/explosion bursts
 * the flock aloft; it wheels and resettles. The visual cloud (instanced/Niagara birds)
 * is a BP that lerps from GetAltitude() and listens to OnFlockPhaseChanged.
 */
UCLASS()
class GTC_UE5_API AGTCBirdFlock : public AActor
{
    GENERATED_BODY()

public:
    AGTCBirdFlock();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Wildlife")
    float BurstSeconds = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Wildlife")
    float MinWheelSeconds = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Wildlife")
    float CalmBeforeSettle = 2.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Wildlife")
    float SettleSeconds = 3.0f;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Wildlife")
    FGtcOnFlockPhaseChanged OnFlockPhaseChanged;

    /** Spook the flock this frame (gunshot/explosion/player proximity). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Wildlife")
    void Startle() { bStartlePending = true; }

    UFUNCTION(BlueprintPure, Category = "GTC|Wildlife")
    EGtcBirdPhase GetPhase() const;

    UFUNCTION(BlueprintPure, Category = "GTC|Wildlife")
    bool IsAloft() const { return Model.IsAloft(); }

    /** Flock height 0..1 (0 perched, 1 wheeling). */
    UFUNCTION(BlueprintPure, Category = "GTC|Wildlife")
    float GetAltitude() const;

    virtual void Tick(float DeltaSeconds) override;

private:
    FBirdFlock Model;
    bool bStartlePending = false;
    EGtcBirdPhase LastPhase = EGtcBirdPhase::Perched;

    FBirdFlock::FParams MakeParams() const;
};
