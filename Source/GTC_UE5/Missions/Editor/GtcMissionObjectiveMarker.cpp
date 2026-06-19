// Copyright Epic Games, Inc. All Rights Reserved.

#include "GtcMissionObjectiveMarker.h"

#include "Components/TextRenderComponent.h"

AGtcMissionObjectiveMarker::AGtcMissionObjectiveMarker()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelText"));
    LabelText->SetupAttachment(SceneRoot);
    LabelText->SetHorizontalAlignment(EHTA_Center);
    LabelText->SetWorldSize(120.0f);
    LabelText->SetTextRenderColor(FColor(255, 210, 0));
    LabelText->SetRelativeLocation(FVector(0.0f, 0.0f, 160.0f));
    LabelText->SetText(FText::FromString(TEXT("Objective")));
}

void AGtcMissionObjectiveMarker::SetLabel(const FString& Text)
{
    if (LabelText)
    {
        LabelText->SetText(FText::FromString(Text));
    }
}

void AGtcMissionObjectiveMarker::SetLabelColor(FColor Color)
{
    if (LabelText)
    {
        LabelText->SetTextRenderColor(Color);
    }
}
