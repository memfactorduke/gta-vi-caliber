// Copyright Epic Games, Inc. All Rights Reserved.

#include "WantedSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

// GTA-style wanted-level console drivers. The kit weapon fire does not yet route
// kills into UWantedSubsystem (the Phase-3 kit-damage -> ReportWitnessedCrime bridge
// is still to be built), so these commands are the interim way to exercise the
// crime -> wanted -> AGTCPoliceDirector dispatch loop on the armed player:
//
//   gtc.Wanted.Crime [0|1]   report a crime (1 = a kill, the default); call a few
//                            times to climb stars and watch the director field cops.
//   gtc.Wanted.Status        log current stars / wanted / crime-active / pending.
//
// Dev/test only — compiled out of shipping builds.
#if !UE_BUILD_SHIPPING

namespace
{
    UWantedSubsystem* GTCGetWanted(UWorld* World)
    {
        if (World)
        {
            if (UGameInstance* GI = World->GetGameInstance())
            {
                return GI->GetSubsystem<UWantedSubsystem>();
            }
        }
        return nullptr;
    }

    FAutoConsoleCommandWithWorldAndArgs GWantedCrimeCmd(
        TEXT("gtc.Wanted.Crime"),
        TEXT("Report a crime to the wanted system (raises heat). Arg: 0 for a non-lethal crime, 1 (default) for a kill."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& Args, UWorld* World)
            {
                UWantedSubsystem* Wanted = GTCGetWanted(World);
                if (!Wanted)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[gtc.Wanted.Crime] no UWantedSubsystem (is a game world running?)"));
                    return;
                }
                const bool bKilled = Args.Num() == 0 || Args[0] != TEXT("0");
                Wanted->ReportCrime(bKilled);
                UE_LOG(LogTemp, Log, TEXT("[gtc.Wanted.Crime] reported (killed=%d) -> stars=%d"),
                    bKilled ? 1 : 0, Wanted->Stars());
            }));

    FAutoConsoleCommandWithWorld GWantedStatusCmd(
        TEXT("gtc.Wanted.Status"),
        TEXT("Log the current wanted stars and state."),
        FConsoleCommandWithWorldDelegate::CreateLambda(
            [](UWorld* World)
            {
                UWantedSubsystem* Wanted = GTCGetWanted(World);
                if (!Wanted)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[gtc.Wanted.Status] no UWantedSubsystem (is a game world running?)"));
                    return;
                }
                UE_LOG(LogTemp, Log, TEXT("[gtc.Wanted.Status] stars=%d wanted=%d crimeActive=%d pendingReports=%d"),
                    Wanted->Stars(), Wanted->IsWanted() ? 1 : 0,
                    Wanted->IsCrimeActive() ? 1 : 0, Wanted->PendingReportCount());
            }));
}

#endif // !UE_BUILD_SHIPPING
