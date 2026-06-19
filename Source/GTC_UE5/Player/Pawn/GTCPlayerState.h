// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GTCPlayerState.generated.h"

class UPlayerStatsComponent;

/**
 * AGTCPlayerState — the canonical owner of the player's persistent stats.
 *
 * Per the W3 armor-ownership resolution (docs/W3_WIRING_NOTES.md), the
 * UPlayerStatsComponent (money + the SOLE armor pool) lives on the PlayerState,
 * so armor/money survive pawn respawns and replicate COND_OwnerOnly from a
 * single authoritative owner. The player CHARACTER owns health only (a
 * UPlayerHealthComponent whose model has ArmorMax=0).
 */
UCLASS()
class GTC_UE5_API AGTCPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    AGTCPlayerState();

    /** The money + armor store (canonical armor owner — W3 resolution). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|Player")
    TObjectPtr<UPlayerStatsComponent> StatsComponent;
};
