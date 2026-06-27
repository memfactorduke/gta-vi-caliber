// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VehicleEntry.h" // FVehicleEntry — the pure-core this component drives
#include "VehicleSeatComponent.generated.h"

class APawn;
class APlayerController;
class UInputMappingContext;
class UAnimSequence;

/**
 * One seat on a vehicle, authored in the car Blueprint as a car-LOCAL offset (cm).
 * The component transforms these into world `FVehicleEntry::FSeat` anchors each
 * time it needs to score "which door is the player nearest" — so the seats track
 * the car as it drives without any per-frame bookkeeping.
 */
USTRUCT(BlueprintType)
struct FVehicleSeatDef
{
    GENERATED_BODY()

    /** Seat position relative to the car origin (cm). Where the occupant rides. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle")
    FVector AnchorOffset = FVector::ZeroVector;

    /** Step-out offset from the anchor (car-local, cm) — where the player lands on exit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle")
    FVector ExitOffset = FVector::ZeroVector;

    /** The seat you actually drive from. The player always ends up here. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle")
    bool bIsDriver = false;
};

/**
 * UVehicleSeatComponent — the get-in / get-out adapter for the pure-Blueprint car.
 *
 * The UE-side half of the Phase-1 Driving "enter/exit" slice: it wraps the tested
 * deterministic core `FVehicleEntry` (Vehicles/Entry/VehicleEntry.h) the same way
 * UGTCInteractionComponent wraps FInteraction — a UActorComponent that owns the
 * pure-core, exposes BlueprintCallable wrappers, and does the engine-only work
 * (possess, attach, input, camera) the core deliberately never touches.
 *
 * Lives on the car (BP_VehicleBase). The car implements IGTCInteractable and
 * forwards its three events here, so the player's EXISTING interaction system
 * (UGTCInteractionComponent: walk near -> prompt -> press E) drives entry with no
 * new input. The component answers the second question the core asks — WHICH door
 * / seat — via FVehicleEntry::NearestAvailableSeat.
 *
 * Flow (one occupant, the player):
 *   OnFoot --(E near car)--> Entering --(EnterSeconds)--> Seated --(E)--> Exiting --> OnFoot
 * On the single frame a transition COMPLETES (the core's one-shot edge) the
 * component possesses the car / re-possesses the player exactly once.
 *
 * Units: everything here is UE world cm (matching the project's 250 cm interaction
 * reach). The core is unit-agnostic (plain FVector::Dist), so cm is consistent.
 *
 * No ChaosVehicles dependency: the car is handled as a bare APawn/AActor, so this
 * needs no new build-module dependency (possession is AController-level; the input
 * swap uses EnhancedInput, already a module dep).
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UVehicleSeatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UVehicleSeatComponent();

    /**
     * PURE + testable: turn car-local seat defs into world-space `FVehicleEntry::FSeat`
     * anchors under a given car transform. Anchors are full transforms of the offset;
     * exit offsets are rotated (not translated) so "step left" stays left as the car
     * turns. Scene-free => headless-unit-tested (Tests/VehicleSeatComponentTest.cpp).
     */
    static void BuildWorldSeats(const TArray<FVehicleSeatDef>& Defs, const FTransform& CarXform,
        TArray<FVehicleEntry::FSeat>& Out);

    // --- IGTCInteractable forwarders (the car BP's interface events call these) ---

    /** True when OnFoot and a free seat is within reach of the instigator. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Vehicle")
    bool CanEnter(const AActor* Instigator) const;

    /** The prompt the existing interaction HUD shows ("Enter Vehicle"). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Vehicle")
    FText GetEnterPrompt() const;

    /**
     * Begin getting in: pick the nearest seat for the instigator and start the
     * Entering transition. The actual possession happens on the Seated edge in Tick.
     * Returns false if not OnFoot, no controller, or no seat in reach.
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|Vehicle")
    bool HandleEnter(AActor* Instigator);

    /**
     * Begin getting out (only valid while Seated). Bind this to IA_EXIT in the car
     * BP's input graph. The un-possession happens on the OnFoot edge in Tick.
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|Vehicle")
    bool BeginExit();

    /**
     * 0..1 door-open drive signal for the AnimBP: ramps up across the get-in and the
     * get-out, 0 while driving / on foot. Feed it into the Door_L/Door_R rotation
     * (smooth it with FInterpTo in the AnimBP to avoid the seated-frame snap).
     */
    UFUNCTION(BlueprintPure, Category = "GTC|Vehicle")
    float GetDoorOpenAlpha() const;

    // --- Tuning (designer-editable on the component in BP_VehicleBase) -----------

    /** Driver + passenger seat defs. Mark exactly one bIsDriver. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle")
    TArray<FVehicleSeatDef> Seats;

    /** How close (cm) the player must be to a seat to enter it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle")
    float ReachCm = 250.0f;

    /** Get-in transition duration (seconds). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle")
    float EnterSeconds = 0.9f;

    /** Get-out transition duration (seconds). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle")
    float ExitSeconds = 0.8f;

    /** Driving input context added on enter / removed on exit (e.g. /Game/CARINPUT/CAR_INPUT). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Input")
    TSoftObjectPtr<UInputMappingContext> DrivingContext;

    /** Priority for the driving context (above the player's contexts). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Input")
    int32 DrivingContextPriority = 110;

    /** Car-mesh socket the player attaches to while seated (add it on carGTC1_Skeleton). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Seat")
    FName DriverSeatSocket = TEXT("DriverSeat");

    /** Seated pose played (looping, single-node) on the player's body mesh; empty = no-op. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Seat")
    TSoftObjectPtr<UAnimSequence> SitPose;

    /** Camera blend time (seconds) when swapping to the car and back. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Camera")
    float ViewBlendSeconds = 0.35f;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    /** Entering->Seated edge: possess car, attach + pose the player, swap input + camera. */
    void OnSeated();

    /** Exiting->OnFoot edge: place + restore the player, un-possess, restore input + camera. */
    void OnExited();

    /** Index of the seat flagged bIsDriver (or 0 / None if unset). */
    int32 DriverSeatIndex() const;

    /** Build FParams from the editable tuning. */
    FVehicleEntry::FParams Params() const;

    /** The deterministic state machine (plain runtime state; not reflected). */
    FVehicleEntry Entry;

    /** The player pawn we hide/attach and re-possess on exit. */
    UPROPERTY(Transient)
    TObjectPtr<APawn> StoredPlayerPawn = nullptr;

    /** The controller that does the possess / un-possess. */
    UPROPERTY(Transient)
    TObjectPtr<APlayerController> StoredController = nullptr;

    /** Which door the player entered from (kept for the future passenger->driver slide). */
    int32 EnteredSeatIndex = -1;
};
