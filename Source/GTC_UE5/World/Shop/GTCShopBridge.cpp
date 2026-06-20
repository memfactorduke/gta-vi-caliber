// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCShopBridge.h"

#include "../../Player/Stats/PlayerStats.h"
#include "../../Systems/Wanted/WantedSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY(LogGtcShop);

namespace GTCShop
{
    UPlayerStatsComponent* FindPlayerStats(AActor* Instigator)
    {
        // The wallet lives on the PlayerState (the W3 stats-ownership resolution); fall
        // back to the actor itself so a non-pawn instigator still resolves in tests.
        if (const APawn* Pawn = Cast<APawn>(Instigator))
        {
            if (APlayerState* PS = Pawn->GetPlayerState())
            {
                if (UPlayerStatsComponent* C = PS->FindComponentByClass<UPlayerStatsComponent>())
                {
                    return C;
                }
            }
        }
        if (Instigator != nullptr)
        {
            if (UPlayerStatsComponent* C = Instigator->FindComponentByClass<UPlayerStatsComponent>())
            {
                return C;
            }
        }
        return nullptr;
    }

    UPlayerStatsComponent* FindPlayerStats(UWorld* World)
    {
        APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
        if (PC == nullptr)
        {
            return nullptr;
        }
        if (PC->PlayerState != nullptr)
        {
            if (UPlayerStatsComponent* C = PC->PlayerState->FindComponentByClass<UPlayerStatsComponent>())
            {
                return C;
            }
        }
        if (APawn* P = PC->GetPawn())
        {
            if (UPlayerStatsComponent* C = P->FindComponentByClass<UPlayerStatsComponent>())
            {
                return C;
            }
        }
        return nullptr;
    }

    UWantedSubsystem* FindWanted(UWorld* World)
    {
        if (UGameInstance* GI = World ? World->GetGameInstance() : nullptr)
        {
            return GI->GetSubsystem<UWantedSubsystem>();
        }
        return nullptr;
    }
}
