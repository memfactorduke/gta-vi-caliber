// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficSignal.h"
#include "GTCTrafficSignal.generated.h"

/** Blueprint mirror of FTrafficSignal::ESignal. */
UENUM(BlueprintType)
enum class EGtcSignalState : uint8
{
    Red,
    Yellow,
    Green
};

/** Blueprint mirror of FTrafficSignal::FPhase — one stage of the cycle. */
USTRUCT(BlueprintType)
struct FGtcSignalPhase
{
    GENERATED_BODY()

    /** Approach-group ids that move (Green then Yellow) this phase. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    TArray<int32> GoGroups;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    float GreenSeconds = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    float YellowSeconds = 2.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGtcOnSignalPhaseChanged, int32, ActivePhase);

/**
 * AGTCTrafficSignal — a placed junction traffic-light controller (UE actor over
 * FTrafficSignal).
 *
 * Drop one per signalized intersection in the map, author its phase ring + offset,
 * and it advances its own clock. Driving the light-mesh emissive and braking the
 * traffic agents on a Red/Yellow group is a BP/adapter that reads StateFor(Group).
 */
UCLASS()
class GTC_UE5_API AGTCTrafficSignal : public AActor
{
    GENERATED_BODY()

public:
    AGTCTrafficSignal();

    /** The phase ring (Green->Yellow per phase, all others Red). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    TArray<FGtcSignalPhase> Phases;

    /** All-red clearance inserted after each phase (seconds). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    float AllRedSeconds = 1.0f;

    /** Slides this junction along its cycle so neighbours desync (seconds). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    float OffsetSeconds = 0.0f;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Traffic")
    FGtcOnSignalPhaseChanged OnSignalPhaseChanged;

    /** Re-install the phase ring from the current UPROPERTYs (resets the clock). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Traffic")
    void ConfigureSignal();

    /** The light shown to approach-group Group right now. */
    UFUNCTION(BlueprintPure, Category = "GTC|Traffic")
    EGtcSignalState StateFor(int32 Group) const;

    UFUNCTION(BlueprintPure, Category = "GTC|Traffic")
    int32 GetActivePhase() const { return Model.ActivePhase(); }

    UFUNCTION(BlueprintPure, Category = "GTC|Traffic")
    bool IsAllRed() const { return Model.IsAllRed(); }

    UFUNCTION(BlueprintPure, Category = "GTC|Traffic")
    float GetTimeUntilChange() const { return static_cast<float>(Model.TimeUntilChange()); }

    UFUNCTION(BlueprintPure, Category = "GTC|Traffic")
    float GetCycleSeconds() const { return static_cast<float>(Model.CycleSeconds()); }

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    FTrafficSignal Model;
    int32 LastActivePhase = INDEX_NONE;
};
