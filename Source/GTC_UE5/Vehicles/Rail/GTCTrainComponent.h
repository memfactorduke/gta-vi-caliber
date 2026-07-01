// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TrainMover.h"         // FTrainMover
#include "RailPathResolver.h"   // FRailPathResolver
#include "GTCTrainComponent.generated.h"

class AActor;

/**
 * UGTCTrainComponent — runs a train on a line: the on-rails mover for a metro/freight that gives the
 * city background motion. FTrainMover is the pure 1-DOF controller (a distance + speed along a
 * looping line that brakes to stop exactly at each station, dwells, and pulls away) and defers
 * "placing the track spline, mapping Position to a world transform" to its adapter; this is that
 * adapter.
 *
 * Author the line as RailPoints (a looping polyline of world points — e.g. a spline's points) and
 * Stations (distances in cm along it where the train stops). On StartLine the loop length is taken
 * from the geometry so the 1-DOF distance maps 1:1 onto the rail. Each tick it Advance()s the
 * controller and places the (kinematic) actor at the sampled world position facing the direction of
 * travel (pure FRailPathResolver), then publishes distance / speed / next-stop for a timetable HUD.
 * NO Chaos, no new subsystem; the train drives itself (no per-frame input).
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UGTCTrainComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTCTrainComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    /** Start running the line (installs the tuning + stations, takes the loop length from the rail
     *  geometry, resets the train to a standstill at distance 0). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Train")
    void StartLine();

    /** Stop running (the component stops moving the actor). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Train")
    void StopLine();

    UFUNCTION(BlueprintPure, Category = "GTC|Train")
    bool IsRunning() const { return bRunning; }

    // --- The line ------------------------------------------------------------------

    /** Looping polyline of world points the train follows (last point connects back to the first).
     *  Needs >= 2 points to place the train; populate from a spline's world points. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Train")
    TArray<FVector> RailPoints;

    /** Station stop distances (cm) along the loop; the train serves them in array order and wraps.
     *  Values are wrapped into the loop on StartLine, so use >= 2 DISTINCT in-range stops for a
     *  running line — a single stop (or several that coincide) parks the train at that platform.
     *  Empty = cruise the loop non-stop at LineSpeed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Train")
    TArray<double> Stations;

    // --- Line tuning (maps to FTrainMover::FParams) --------------------------------

    /** Fallback loop length (cm) used only when RailPoints has no usable geometry. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Train", meta = (ClampMin = "1.0"))
    float TrackLength = 100000.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Train", meta = (ClampMin = "0.0"))
    float LineSpeed = 2000.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Train", meta = (ClampMin = "0.0"))
    float Accel = 300.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Train", meta = (ClampMin = "0.0"))
    float Brake = 400.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Train", meta = (ClampMin = "0.0"))
    float DwellSeconds = 5.0f;

    /** The train actor to move. Null => this component's owner. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Train")
    TObjectPtr<AActor> Train = nullptr;

    // --- Published telemetry (for a timetable / debug HUD) -------------------------

    /** Distance along the loop (cm). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Train")
    float DistanceAlongLine = 0.0f;

    /** Current speed (cm/s). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Train")
    float Speed = 0.0f;

    /** Forward distance (cm) to the next station stop. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Train")
    float DistanceToNextStop = 0.0f;

    /** Index of the station being served (or -1 if none). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Train")
    int32 TargetStation = -1;

    /** True while stopped at a platform for its dwell. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Train")
    bool bDwelling = false;

    /** True while the train is placed on valid rail geometry this frame. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Train")
    bool bOnRail = false;

private:
    /** The train actor (Train if set, else the owner). */
    AActor* ResolveTrain() const;

    /** The pure 1-DOF controller. */
    FTrainMover Mover;

    bool bRunning = false;
};
