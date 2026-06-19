// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GtcMissionDefinition.h"
#include "GtcMissionObjectiveMarker.generated.h"

class UTextRenderComponent;

/**
 * A god-mode authoring gizmo: a visible in-world marker for one mission objective's waypoint.
 * The editor subsystem spawns one per placed objective so a designer can SEE the mission laid
 * out in the level while authoring it. It carries the owning mission/objective ids and shows a
 * floating text label, so it needs no art assets to be useful (a Blueprint subclass can add a
 * mesh / beacon). Distinct from AGTCPlaceMarker, which registers NPC destinations.
 */
UCLASS(ClassGroup = (GTC))
class GTC_UE5_API AGtcMissionObjectiveMarker : public AActor
{
    GENERATED_BODY()

public:
    AGtcMissionObjectiveMarker();

    /** Mission this marker belongs to. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|MissionEditor")
    FGuid MissionId;

    /** Objective id this marker represents. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|MissionEditor")
    FString ObjectiveId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|MissionEditor")
    EGtcObjectiveKind Kind = EGtcObjectiveKind::Reach;

    /** Update the floating in-world label. */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    void SetLabel(const FString& Text);

    /** Recolor the label (e.g. blue for a mission-start giver vs gold for an objective). */
    UFUNCTION(BlueprintCallable, Category = "GTC|MissionEditor")
    void SetLabelColor(FColor Color);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|MissionEditor")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|MissionEditor")
    TObjectPtr<UTextRenderComponent> LabelText;
};
