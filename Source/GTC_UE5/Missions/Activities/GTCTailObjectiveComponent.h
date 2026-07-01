// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TailObjective.h" // FTailObjective — same folder
#include "TailResolver.h"  // FTailResolver
#include "GTCTailObjectiveComponent.generated.h"

class AActor;

/** Blueprint mirror of FTailObjective::EState (1:1 order so a static_cast bridges the seam). */
UENUM(BlueprintType)
enum class EGTCTailState : uint8
{
    Tailing,       // still on them
    Succeeded,     // tracked long enough
    FailedLost,    // let them get too far away
    FailedSpotted, // got made
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGTCTailSucceeded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGTCTailFailed, EGTCTailState, Reason);

/**
 * UGTCTailObjectiveComponent — the live "tail the mark" surveillance mission: follow without being
 * made. It runs the orphan pure-core FTailObjective (two-sided failure — drift too far and a grace
 * clock loses them; sit too close or in their view and a suspicion meter blows the tail; ride the
 * band long enough and it succeeds) from actual world state, the "DEFERRED adapter" TailObjective.h
 * describes.
 *
 * Follows the project's pure-core + thin-shell split (like URadioTunerComponent over FRadioTuner):
 * FTailResolver does the distance + view-cone geometry, this shell owns the FTailObjective, feeds it
 * the follower's distance and whether the follower is in the mark's forward view each tick, publishes
 * suspicion / progress / distance for the HUD, and fires OnTailSucceeded / OnTailFailed. Plain
 * UActorComponent — NO new subsystem, NO ChaosVehicles dep — reporting OUT via Blueprint events (bind
 * to the mission wiring / "you're being followed" HUD). Put it on the player (or set Follower), point
 * Target at the mark, call StartTail. Self-contained: it needs only the two actors' transforms; true
 * line-of-sight (walls) is an optional refinement the BP can layer on.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UGTCTailObjectiveComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTCTailObjectiveComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // --- Who's tailing whom -------------------------------------------------------

    /** The mark being followed (required). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Tail")
    TObjectPtr<AActor> Target = nullptr;

    /** The follower. Null => this component's owner (the player). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Tail")
    TObjectPtr<AActor> Follower = nullptr;

    // --- Tuning (maps to FTailObjective::FParams + the view cone) ------------------

    /** Closer than this (cm) and the mark gets suspicious even out of its view. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Tail", meta = (ClampMin = "0.0"))
    float MinDistance = 400.0f;

    /** Farther than this (cm) and the "lost" grace clock starts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Tail", meta = (ClampMin = "0.0"))
    float MaxDistance = 3000.0f;

    /** Seconds you can stay beyond MaxDistance before the mark is lost. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Tail", meta = (ClampMin = "0.0"))
    float LostGraceSeconds = 4.0f;

    /** Suspicion (0..1) gained per second while too close or in the mark's view. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Tail", meta = (ClampMin = "0.0"))
    float SuspicionRisePerSec = 0.4f;

    /** Suspicion shed per second while safely in the band. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Tail", meta = (ClampMin = "0.0"))
    float SuspicionFallPerSec = 0.3f;

    /** Seconds tracked within the band needed to complete the tail. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Tail", meta = (ClampMin = "0.0"))
    float RequiredTrackSeconds = 20.0f;

    /** Half-angle (degrees) of the mark's forward view cone. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Tail", meta = (ClampMin = "0.0", ClampMax = "180.0"))
    float ViewConeHalfAngleDeg = 45.0f;

    /** Distance (cm) beyond which the mark can't make the follower out. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Tail", meta = (ClampMin = "0.0"))
    float ViewRange = 4000.0f;

    // --- Control ------------------------------------------------------------------

    /** Begin the tail (configures a fresh objective from the current tuning). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Tail")
    void StartTail();

    /** Abandon the tail without success/failure (no events fire). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Tail")
    void CancelTail()
    {
        bActive = false;
        SetComponentTickEnabled(false);
    }

    /** True while a tail is running and unresolved. */
    UFUNCTION(BlueprintPure, Category = "GTC|Tail")
    bool IsTailActive() const { return bActive; }

    // --- Published state (updated each active tick) --------------------------------

    /** How made the mark is, 0..1 (1 = spotted). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Tail")
    float Suspicion = 0.0f;

    /** Progress toward completing the tail, 0..1. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Tail")
    float TrackProgress = 0.0f;

    /** Distance from the follower to the mark (cm). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Tail")
    float DistanceToTarget = 0.0f;

    /** True while the follower sits in the mark's view cone (before any line-of-sight refinement). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Tail")
    bool bInTargetView = false;

    /** Current tail state. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Tail")
    EGTCTailState TailState = EGTCTailState::Tailing;

    // --- Events -------------------------------------------------------------------

    /** Fired once when the tail is completed. */
    UPROPERTY(BlueprintAssignable, Category = "GTC|Tail")
    FGTCTailSucceeded OnTailSucceeded;

    /** Fired once on a failed tail; carries the reason (FailedLost or FailedSpotted). */
    UPROPERTY(BlueprintAssignable, Category = "GTC|Tail")
    FGTCTailFailed OnTailFailed;

private:
    /** The follower's world location (Follower if set, else the owner). */
    FVector FollowerLocation() const;

    /** Copy the core's state onto the published UPROPERTYs. */
    void Publish(double Distance, bool bInView);

    /** The pure objective + its run state. */
    FTailObjective Tail;
    bool bActive = false;

    /** True once the tail has seen a live Target — so a Target that goes null AFTER StartTail
     *  (mark destroyed) reads as a lost mark, not as "no mark assigned yet". */
    bool bHadTarget = false;

    // View-cone tuning snapshotted at StartTail (frozen for the run, like the objective params),
    // so a mid-tail edit to ViewConeHalfAngleDeg / ViewRange can't change the cone under way.
    double ViewHalfAngleRadSnapshot = 0.0;
    double ViewRangeSnapshot = 0.0;
};
