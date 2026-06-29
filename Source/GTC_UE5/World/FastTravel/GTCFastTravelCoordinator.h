// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FastTravelNetwork.h"
#include "GTCFastTravelHub.h" // AGTCFastTravelHub + the reflected EGTCHubKind
#include "GTCFastTravelCoordinator.generated.h"

/**
 * AGTCFastTravelCoordinator — the one actor that turns the placed AGTCFastTravelHub markers
 * into a working fast-travel network. Drop ONE in the persistent level (the
 * AGTCWeatherController / AGTCPoliceDirector "drop-one" pattern; no new subsystem). On
 * BeginPlay it gathers every `fast_travel_hub`-tagged actor, and a hop request runs the
 * pure FFastTravelNetwork decisions: pick the hub, check you're allowed to leave (no fast
 * travel while wanted), quote the fare/time, then fade -> relocate the player -> fade.
 *
 * Gate-deferred UObject (UBT/editor only here). FFastTravelNetwork is shim-verified + tested.
 */
UCLASS()
class GTC_UE5_API AGTCFastTravelCoordinator : public AActor
{
    GENERATED_BODY()

public:
    AGTCFastTravelCoordinator();

    virtual void BeginPlay() override;

    /** Fast-travel the player to the hub nearest the cursor/selection of a given kind.
     *  Returns false if departure is blocked (wanted / threat) or no such hub exists. */
    UFUNCTION(BlueprintCallable, Category = "GTC|FastTravel")
    bool RequestHopToNearestOfKind(EGTCHubKind Kind);

    /** Fast-travel the player to a specific hub by its index in the gathered list. */
    UFUNCTION(BlueprintCallable, Category = "GTC|FastTravel")
    bool RequestHopToIndex(int32 HubIndex);

    /** Whether the player may currently fast-travel at all (no stars). */
    UFUNCTION(BlueprintCallable, Category = "GTC|FastTravel")
    bool CanDepartNow() const;

    /** Fare the player would pay to hop to HubIndex from their current position. */
    UFUNCTION(BlueprintCallable, Category = "GTC|FastTravel")
    int32 QuoteFareToIndex(int32 HubIndex) const;

    /** Refresh the hub list (call if hubs are spawned/unlocked at runtime). */
    UFUNCTION(BlueprintCallable, Category = "GTC|FastTravel")
    void RebuildNetwork();

protected:
    /** Per-km fare on top of the base. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|FastTravel") double FarePerKm = 5.0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|FastTravel") int32 BaseFare = 50;
    /** A threat closer than this (m) blocks departure. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|FastTravel") double ThreatSafeRangeM = 30.0;
    /** Lift the player this far above the hub on arrival (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|FastTravel") double ArriveZOffsetCm = 120.0;
    /** Camera fade time each way (seconds). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|FastTravel") float FadeSeconds = 0.4f;

private:
    int32 ReadStars() const;
    APawn* PlayerPawn() const;
    FVector PlayerLocation() const;
    void RelocatePlayerTo(const FVector& Dest);

    /** Live FFastTravelNetwork::FHub graph, rebuilt from the tagged hub actors. */
    TArray<FFastTravelNetwork::FHub> Network;
    UPROPERTY() TArray<TObjectPtr<AGTCFastTravelHub>> Hubs;
};
