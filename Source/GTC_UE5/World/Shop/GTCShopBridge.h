// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;
class UPlayerStatsComponent;
class UWantedSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogGtcShop, Log, All);

/**
 * GTCShop — small UObject-side glue shared by the storefront actors and the
 * GTC.Shop.* console commands: resolve the player's wallet (UPlayerStatsComponent,
 * owned by the PlayerState with a pawn fallback) and the wanted subsystem, from
 * either an interacting actor or a world. Kept off the actors so the three thin
 * storefronts don't each re-derive the same lookups (the lookup mirrors the mission
 * editor's FindPlayerStats).
 */
namespace GTCShop
{
    /** The player's stats/wallet component, reached from an interacting actor (a pawn). */
    GTC_UE5_API UPlayerStatsComponent* FindPlayerStats(AActor* Instigator);

    /** The player's stats/wallet component, reached from the world's first player. */
    GTC_UE5_API UPlayerStatsComponent* FindPlayerStats(UWorld* World);

    /** The wanted subsystem (heat/stars), or null. */
    GTC_UE5_API UWantedSubsystem* FindWanted(UWorld* World);
}
