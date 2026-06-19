// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCNotorietyDirector.h"

#include "Engine/GameInstance.h"
#include "../Wanted/WantedSubsystem.h"

void UGTCNotorietyDirector::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Bind the existing public wanted signal so real crime feeds infamy.
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UWantedSubsystem* Wanted = GI->GetSubsystem<UWantedSubsystem>())
        {
            Wanted->OnStarsChanged.AddDynamic(this, &UGTCNotorietyDirector::HandleStarsChanged);
        }
    }

    bInitialized = true;
}

void UGTCNotorietyDirector::Deinitialize()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UWantedSubsystem* Wanted = GI->GetSubsystem<UWantedSubsystem>())
        {
            Wanted->OnStarsChanged.RemoveDynamic(this, &UGTCNotorietyDirector::HandleStarsChanged);
        }
    }
    bInitialized = false;

    Super::Deinitialize();
}

void UGTCNotorietyDirector::HandleStarsChanged(int32 Stars)
{
    // Only a RISE in heat builds infamy; cooling off (hiding) doesn't.
    if (Stars > LastStars)
    {
        Notoriety.Add(FNotoriety::ForCrime * (Stars - LastStars));
    }
    LastStars = Stars;
}

void UGTCNotorietyDirector::Tick(float DeltaTime)
{
    if (!bInitialized || DeltaTime <= 0.0f)
    {
        return;
    }

    Notoriety.Decay(DeltaTime);

    // High infamy off cooldown -> send the hunters, then restart the cooldown.
    if (Notoriety.HunterReady())
    {
        OnBountyHunters.Broadcast(Notoriety.WaveSize(), static_cast<float>(Notoriety.BountyAmount()));
        Notoriety.TriggerWave();
    }
}

TStatId UGTCNotorietyDirector::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTCNotorietyDirector, STATGROUP_Tickables);
}
