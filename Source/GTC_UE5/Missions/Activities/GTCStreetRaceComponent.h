// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Misc/Optional.h"
#include "RaceSession.h"          // FRaceSession (+ StreetRace)
#include "RaceProgressResolver.h" // FRaceProgressResolver
#include "GTCStreetRaceComponent.generated.h"

class AActor;

/** Blueprint mirror of FRaceSession::EPhase (1:1 order so a static_cast bridges the seam). */
UENUM(BlueprintType)
enum class EGTCRacePhase : uint8
{
    Ready,    // on the grid, counting down
    Racing,   // green — clocks run, gates count
    Finished, // everyone in
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGTCRaceGreen);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGTCRaceCheckpoint, int32, CheckpointIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGTCRaceFinished, int32, PlayerPlacement, int32, Reward);

/**
 * UGTCStreetRaceComponent — the live street-race adapter: the "RaceController" the race cores
 * (StreetRace / FRaceSession / FRivalPacer) were built for and explicitly deferred. It turns an
 * authored ring of checkpoint FVectors + a set of rival pace times into a running, ranked race:
 * a countdown, a green flag, the player crossing gates by driving through them, AI rivals paced
 * down the course, live placement, and a finish with rewards.
 *
 * It follows the codebase's "extract the logic into a non-UObject core + a thin UObject shell"
 * pattern (as CLAUDE.md/AGENTS.md prescribe and e.g. URadioTunerComponent wraps the pure
 * FRadioTuner): the coordinate bridge and rival pacing live in the pure FRaceProgressResolver,
 * while this shell owns the FRaceSession, feeds the player's real world position and each rival's
 * paced gate count into it every tick, and publishes standings for the HUD. Plain UActorComponent —
 * NO new subsystem, NO ChaosVehicles dependency — and it reports OUT via Blueprint events (bind
 * OnRaceFinished to the reward / mission wiring). Put it on the player's car (or set Racer), author
 * Checkpoints, call StartRace.
 *
 * Pairs naturally with the vehicle-handling wet-road grip and the weather-reactive ambient traffic
 * for a race in the rain, but depends on neither — it only needs the racer's world position.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UGTCStreetRaceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTCStreetRaceComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // --- Course config -------------------------------------------------------------

    /** Ordered ring of checkpoint gates, in UE world space. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Race")
    TArray<FVector> Checkpoints;

    /** Laps around the ring (floored at 1). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Race", meta = (ClampMin = "1"))
    int32 Laps = 1;

    /** Pre-grid countdown before the lights go green. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Race", meta = (ClampMin = "0.0"))
    float CountdownSeconds = 3.0f;

    /** How close (cm) the racer must be to a gate to count it crossed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Race", meta = (ClampMin = "1.0"))
    float CheckpointRadius = 800.0f;

    /** 1st-place cash; lower places scale down (via StreetRace::Reward). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Race", meta = (ClampMin = "0"))
    int32 BaseReward = 1000;

    /** Each rival's solo finish time (seconds). The field is 1 player + this many rivals. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Race")
    TArray<float> RivalPaceSeconds;

    /** Rubber-band gain pulling rivals toward the player's progress (0 = honest pace clock). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Race", meta = (ClampMin = "0.0"))
    float RubberBandStrength = 0.3f;

    /** Hard cap on the rubber-band nudge (fraction of the course) so a far lead is never erased. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Race", meta = (ClampMin = "0.0"))
    float RubberBandMax = 0.05f;

    /** The racer whose position crosses gates. Null => this component's owner. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Race")
    TObjectPtr<AActor> Racer = nullptr;

    // --- Control -------------------------------------------------------------------

    /** Build the field and start the countdown. No-op with no checkpoints. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Race")
    void StartRace();

    /** Stop the race and clear the field (no events fire). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Race")
    void AbortRace();

    /** True while a race is running (grid or green, not yet finished). */
    UFUNCTION(BlueprintPure, Category = "GTC|Race")
    bool IsRaceActive() const { return bRacing; }

    // --- Published standings (updated each tick) -----------------------------------

    UPROPERTY(BlueprintReadOnly, Category = "GTC|Race")
    EGTCRacePhase RacePhase = EGTCRacePhase::Ready;

    /** Player's live 1-based placement in the field. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Race")
    int32 PlayerPlacement = 0;

    /** Number of racers (player + rivals). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Race")
    int32 FieldSize = 0;

    /** Player's current lap (1-based) and total laps. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Race")
    int32 CurrentLap = 0;

    UPROPERTY(BlueprintReadOnly, Category = "GTC|Race")
    int32 TotalLaps = 0;

    /** Player's overall race progress, 0..1. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Race")
    float PlayerProgress = 0.0f;

    /** Seconds left on the pre-grid countdown (0 once green). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Race")
    float CountdownRemaining = 0.0f;

    /** Player's race clock (seconds). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Race")
    float ElapsedSeconds = 0.0f;

    /** Player's best lap split (seconds, 0 until a lap completes). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Race")
    float BestLapSeconds = 0.0f;

    /** The gate the player is driving toward, in UE world space (for a HUD waypoint). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Race")
    FVector NextCheckpointLocation = FVector::ZeroVector;

    // --- Events --------------------------------------------------------------------

    /** Fired once when the countdown ends and the race goes green. */
    UPROPERTY(BlueprintAssignable, Category = "GTC|Race")
    FGTCRaceGreen OnRaceGreen;

    /** Fired when the player crosses a gate; carries the new checkpoint index within the lap. */
    UPROPERTY(BlueprintAssignable, Category = "GTC|Race")
    FGTCRaceCheckpoint OnCheckpoint;

    /** Fired once when the race finishes; carries the player's placement and reward. */
    UPROPERTY(BlueprintAssignable, Category = "GTC|Race")
    FGTCRaceFinished OnRaceFinished;

private:
    /** The tracked racer's world location (Racer if set, else the owner). */
    FVector RacerLocation() const;

    /** Copy live session state onto the published UPROPERTYs. */
    void Publish();

    /** Fire OnRaceGreen once, the first time the session is Racing (handles a 0s countdown too). */
    void MaybeAnnounceGreen();

    /** The race, built on StartRace (no default construction — held optional). */
    TOptional<FRaceSession> Session;

    // --- Snapshots taken at StartRace, so live edits to the config UPROPERTYs can't desync or
    //     over-index the running race (the session was built from these values once). ---
    /** The authored checkpoint ring (UE world space) at race start — indexed for the HUD waypoint. */
    TArray<FVector> CourseWaypoints;
    /** Each rival's pace (s) at race start — drives the field for the whole run. */
    TArray<double> RivalPaces;
    /** Total gates on the course (checkpoints-per-lap * laps) at race start. */
    int32 CourseGateCount = 0;

    /** Prev monotonic progress per rival (parallel to RivalPaces). */
    TArray<double> RivalProgress;

    bool bRacing = false;
    bool bReportedFinish = false;
    bool bAnnouncedGreen = false;
};
