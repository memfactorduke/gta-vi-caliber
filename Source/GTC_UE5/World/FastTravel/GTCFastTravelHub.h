// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FastTravelNetwork.h" // EHubKind
#include "GTCFastTravelHub.generated.h"

class AGTCKinematicVehiclePawn;
class USceneComponent;

/**
 * Editor-facing, reflected mirror of the pure-core EHubKind (FastTravelNetwork.h), which
 * can't carry UENUM because that header is also compiled headless by the shim. Identical
 * layout/order, so the two static_cast freely.
 */
UENUM(BlueprintType)
enum class EGTCHubKind : uint8
{
    Helipad,
    Marina,
    Airstrip,
    Generic,
};

/**
 * AGTCFastTravelHub — a hand-placeable air/sea hub: a helipad, marina or airstrip. Drop one
 * in the level and it tags itself `fast_travel_hub` on BeginPlay so the
 * AGTCFastTravelCoordinator can gather it into the FFastTravelNetwork graph, and it can
 * spawn a parked vehicle (a chopper on the pad, a boat at the slip) ready to board. The
 * helipad/marina/airstrip subclasses just set the default kind.
 */
UCLASS()
class GTC_UE5_API AGTCFastTravelHub : public AActor
{
    GENERATED_BODY()

public:
    AGTCFastTravelHub();

    virtual void BeginPlay() override;

    /** Tag the coordinator searches for. */
    static const FName HubTag;

    /** The pure-core kind (cast from the reflected editor enum) for FFastTravelNetwork. */
    EHubKind GetHubKind() const { return static_cast<EHubKind>(HubKind); }
    bool IsUnlocked() const { return bUnlocked; }
    int32 GetHubId() const { return HubId; }

protected:
    /** What kind of hub this is (drives nearest-of-kind queries + the spawned vehicle). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Hub")
    EGTCHubKind HubKind = EGTCHubKind::Generic;

    /** Whether the player may fast-travel to/from here yet. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Hub")
    bool bUnlocked = true;

    /** Stable id for this hub (for save/unlock bookkeeping). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Hub")
    int32 HubId = 0;

    /** Optional vehicle parked here on BeginPlay (e.g. BP_Helicopter on a helipad). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Hub")
    TSubclassOf<AGTCKinematicVehiclePawn> ParkedVehicleClass;

    /** Where the parked vehicle spawns, relative to the hub (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Hub")
    FVector ParkedVehicleOffset = FVector(0.0, 0.0, 50.0);

private:
    UPROPERTY(VisibleAnywhere, Category = "GTC|Hub", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USceneComponent> Root;

    void SpawnParkedVehicle();
};
