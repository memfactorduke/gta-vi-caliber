// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCVideoSettings.h"

#include "../../Camera/SmartSharpen.h"
#include "../../Camera/GTCSmartSharpen.h"

#include "Engine/World.h"
#include "EngineUtils.h"

UGTCVideoSettings* UGTCVideoSettings::Get()
{
    // The class-default object carries the Config-loaded values; mutate it and
    // SaveConfig() to persist. Mutable so callers can change settings at runtime.
    return GetMutableDefault<UGTCVideoSettings>();
}

float UGTCVideoSettings::ResolveSharpenAmount() const
{
    return FSmartSharpen::UserSharpnessToAmount(Sharpness, MaxUserSharpen);
}

void UGTCVideoSettings::SetSharpness(float NewSharpness, UWorld* World)
{
    Sharpness = FMath::Clamp(NewSharpness, 0.0f, 1.0f);
    SaveConfig();
    if (World)
    {
        ApplySharpen(World);
    }
}

void UGTCVideoSettings::ApplySharpen(UWorld* World)
{
    if (!World)
    {
        return;
    }

    const float Amount = ResolveSharpenAmount();

    // Prefer an existing global (unbound) sharpen actor; otherwise reuse any
    // sharpen actor present, and only spawn one if the level has none.
    AGTCSmartSharpen* Target = nullptr;
    for (TActorIterator<AGTCSmartSharpen> It(World); It; ++It)
    {
        Target = *It;
        if (Target->bUnbound)
        {
            break;
        }
    }

    if (!Target)
    {
        FActorSpawnParameters Params;
        Params.Name = TEXT("GTCGlobalSmartSharpen");
        Target = World->SpawnActor<AGTCSmartSharpen>(
            AGTCSmartSharpen::StaticClass(), FTransform::Identity, Params);
    }

    if (Target)
    {
        Target->SharpenAmount = Amount;
        Target->bUnbound = true;
        Target->ApplySettings();
    }
}
