// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCSmartSharpen.h"

#include "SmartSharpen.h"

#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

#if WITH_EDITORONLY_DATA
#include "Components/BillboardComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"
#endif

namespace
{
    // Default post-process sharpen material. A soft path (not a hard
    // ConstructorHelpers ref) so the actor compiles and loads even before the
    // .uasset is authored — the effect just stays inert until the material exists.
    const TCHAR* const DefaultSharpenMaterialPath =
        TEXT("/Game/Materials/PostProcess/M_GTC_SmartSharpen.M_GTC_SmartSharpen");

    // Material parameter names — must match the M_GTC_SmartSharpen parameters.
    const FName P_Amount(TEXT("Amount"));
    const FName P_PixelRadius(TEXT("PixelRadius"));
    const FName P_ShadowFloor(TEXT("ShadowFloor"));
    const FName P_DistanceEnable(TEXT("DistanceEnable"));
    const FName P_FarAmount(TEXT("FarAmount"));
    const FName P_DistanceNear(TEXT("DistanceNear"));
    const FName P_DistanceFar(TEXT("DistanceFar"));
    const FName P_FoliageEnable(TEXT("FoliageEnable"));
    const FName P_FoliageAmount(TEXT("FoliageAmount"));
}

AGTCSmartSharpen::AGTCSmartSharpen()
{
    // Pure post-process driver: no tick, no physics. Parameters are pushed once
    // on construction/begin-play and again whenever a property changes.
    PrimaryActorTick.bCanEverTick = false;

    PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
    SetRootComponent(PostProcess);

    SharpenMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(DefaultSharpenMaterialPath));

#if WITH_EDITORONLY_DATA
    Icon = CreateDefaultSubobject<UBillboardComponent>(TEXT("Icon"));
    if (Icon)
    {
        static ConstructorHelpers::FObjectFinderOptional<UTexture2D> IconTexture(
            TEXT("/Engine/EditorResources/S_VisBlockOn"));
        if (IconTexture.Succeeded())
        {
            Icon->Sprite = IconTexture.Get();
        }
        Icon->SetupAttachment(PostProcess);
        Icon->bIsScreenSizeScaled = true;
    }

    InfoText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("InfoText"));
    if (InfoText)
    {
        InfoText->SetupAttachment(PostProcess);
        InfoText->SetHorizontalAlignment(EHTA_Center);
        InfoText->SetWorldSize(40.0f);
        InfoText->SetRelativeLocation(FVector(0.0f, 0.0f, 30.0f));
        InfoText->SetTextRenderColor(FColor(80, 200, 255));
        InfoText->bIsEditorOnly = true;
    }
#endif
}

void AGTCSmartSharpen::BeginPlay()
{
    Super::BeginPlay();
    ApplySettings();
}

void AGTCSmartSharpen::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplySettings();
}

#if WITH_EDITOR
void AGTCSmartSharpen::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    ApplySettings();
}
#endif

void AGTCSmartSharpen::ApplySettings()
{
    if (!PostProcess)
    {
        return;
    }

    // Local vs global: unbound sharpens the whole scene; bounded blends over a
    // soft radius so multiple actors can sharpen areas differently.
    PostProcess->bUnbound = bUnbound;
    PostProcess->BlendWeight = FMath::Clamp(BlendWeight, 0.0f, 1.0f);
    PostProcess->BlendRadius = FMath::Max(0.0f, BlendRadius);
    PostProcess->Priority = Priority;

    EnsureMID();
    PushParameters();
    UpdateEditorIcon();
}

void AGTCSmartSharpen::EnsureMID()
{
    UMaterialInterface* BaseMaterial = SharpenMaterial.LoadSynchronous();
    if (!BaseMaterial)
    {
        // Material not authored / not found yet — leave the pass empty rather
        // than spamming. The actor is harmless until the material exists.
        PostProcess->Settings.WeightedBlendables.Array.Empty();
        SharpenMID = nullptr;
        return;
    }

    // Rebuild the MID only when the base material changed; otherwise reuse it so
    // re-applying parameters is cheap and doesn't churn allocations.
    if (!SharpenMID || SharpenMID->Parent != BaseMaterial)
    {
        SharpenMID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
    }

    // Re-register as the sole blendable so repeated ApplySettings() can't stack
    // duplicate passes. Component BlendWeight already carries the master strength.
    PostProcess->Settings.WeightedBlendables.Array.Empty();
    if (SharpenMID)
    {
        PostProcess->Settings.AddBlendable(SharpenMID, 1.0f);
    }
}

void AGTCSmartSharpen::PushParameters()
{
    if (!SharpenMID)
    {
        return;
    }

    SharpenMID->SetScalarParameterValue(P_Amount, FSmartSharpen::ClampAmount(SharpenAmount));
    SharpenMID->SetScalarParameterValue(P_PixelRadius, FMath::Max(0.0f, PixelRadius));
    SharpenMID->SetScalarParameterValue(P_ShadowFloor, FMath::Clamp(ShadowFloor, 0.0f, 1.0f));

    SharpenMID->SetScalarParameterValue(P_DistanceEnable, bDistanceControl ? 1.0f : 0.0f);
    SharpenMID->SetScalarParameterValue(P_FarAmount, FSmartSharpen::ClampAmount(FarSharpenAmount));
    SharpenMID->SetScalarParameterValue(P_DistanceNear, FMath::Max(0.0f, DistanceNear));
    // Keep Far strictly above Near so the shader's divide stays well-defined.
    SharpenMID->SetScalarParameterValue(P_DistanceFar, FMath::Max(DistanceNear + 1.0f, DistanceFar));

    SharpenMID->SetScalarParameterValue(P_FoliageEnable, bFoliageControl ? 1.0f : 0.0f);
    SharpenMID->SetScalarParameterValue(P_FoliageAmount, FSmartSharpen::ClampAmount(FoliageAmount));
}

void AGTCSmartSharpen::UpdateEditorIcon()
{
#if WITH_EDITORONLY_DATA
    if (InfoText)
    {
        // Persistent visualization: show the live amount + scope on the icon so
        // it reads at a glance in the viewport without selecting the actor.
        const TCHAR* Scope = bUnbound ? TEXT("global") : TEXT("local");
        InfoText->SetText(FText::FromString(
            FString::Printf(TEXT("Sharpen %.2f (%s)"), SharpenAmount, Scope)));
    }
#endif
}
