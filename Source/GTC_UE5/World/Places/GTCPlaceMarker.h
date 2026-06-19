// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTCPlaceMarker.generated.h"

/**
 * AGTCPlaceMarker — a hand-placeable point of interest. Drop one in a level,
 * pick its Kind ("diner", "bar", "park", "home", "office", "gym", "street",
 * "restroom"), and on BeginPlay it registers itself with the world's
 * UGTCPlaceRegistrySubsystem so citizens start treating its spot as a real
 * destination of that kind. Removed cleanly on EndPlay.
 *
 * This is the authoring path that refines the crowd's runtime auto-seed: a
 * level can override the procedural POI layout simply by placing markers where
 * the actual diner / beach bar / promenade are. Nearest-of-kind selection means
 * authored markers and auto-seeded points coexist without conflict.
 */
UCLASS()
class GTC_UE5_API AGTCPlaceMarker : public AActor
{
    GENERATED_BODY()

public:
    AGTCPlaceMarker();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** The kind of place this marks ("diner", "bar", "park", ...). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Places")
    FName Kind = TEXT("street");

    /** Optional occupancy cap (0 = unlimited); spreads crowds across same-kind POIs. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Places")
    int32 Capacity = 0;

private:
    /** Registry handle, so EndPlay can remove exactly this marker. */
    int32 Handle = 0;
};
