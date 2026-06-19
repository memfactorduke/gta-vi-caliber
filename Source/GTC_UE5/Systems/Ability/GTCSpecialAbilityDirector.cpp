// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCSpecialAbilityDirector.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "../Wanted/WantedSubsystem.h"

void UGTCSpecialAbilityDirector::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UWantedSubsystem* Wanted = GI->GetSubsystem<UWantedSubsystem>())
        {
            Wanted->OnStarsChanged.AddDynamic(this, &UGTCSpecialAbilityDirector::HandleStarsChanged);
        }
    }

    bInitialized = true;
}

void UGTCSpecialAbilityDirector::Deinitialize()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UWantedSubsystem* Wanted = GI->GetSubsystem<UWantedSubsystem>())
        {
            Wanted->OnStarsChanged.RemoveDynamic(this, &UGTCSpecialAbilityDirector::HandleStarsChanged);
        }
    }
    bInitialized = false;

    Super::Deinitialize();
}

void UGTCSpecialAbilityDirector::HandleStarsChanged(int32 Stars)
{
    if (Stars > LastStars)
    {
        Ability.AddCharge(ChargePerStar * (Stars - LastStars));
    }
    LastStars = Stars;
}

void UGTCSpecialAbilityDirector::NotifyKill(bool bHeadshot)
{
    Ability.AddCharge(bHeadshot ? FSpecialAbility::ChargeForHeadshot : FSpecialAbility::ChargeForKill);
}

void UGTCSpecialAbilityDirector::NotifyStunt()
{
    Ability.AddCharge(FSpecialAbility::ChargeForStunt);
}

void UGTCSpecialAbilityDirector::NotifyNearMiss()
{
    Ability.AddCharge(FSpecialAbility::ChargeForNearMiss);
}

void UGTCSpecialAbilityDirector::SetKind(int32 Kind)
{
    switch (Kind)
    {
    case 1:  Ability.Kind = EAbilityKind::Bruiser; break;
    case 2:  Ability.Kind = EAbilityKind::Driver; break;
    default: Ability.Kind = EAbilityKind::Marksman; break;
    }
}

void UGTCSpecialAbilityDirector::Tick(float DeltaTime)
{
    if (!bInitialized || DeltaTime <= 0.0f)
    {
        return;
    }
    Ability.Update(DeltaTime); // may auto-deactivate when the bar empties
    ApplyTimeDilation();
}

void UGTCSpecialAbilityDirector::ApplyTimeDilation()
{
    // Only touch global time dilation on the active<->inactive edges, so we don't stomp
    // other time-scale effects every frame.
    const bool bActiveNow = Ability.IsActive();
    if (bActiveNow == bDilationApplied)
    {
        return;
    }

    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    if (!World)
    {
        return;
    }

    const float Scale = bActiveNow ? static_cast<float>(Ability.TimeDilation()) : 1.0f;
    UGameplayStatics::SetGlobalTimeDilation(World, Scale);
    bDilationApplied = bActiveNow;
}

TStatId UGTCSpecialAbilityDirector::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTCSpecialAbilityDirector, STATGROUP_Tickables);
}
