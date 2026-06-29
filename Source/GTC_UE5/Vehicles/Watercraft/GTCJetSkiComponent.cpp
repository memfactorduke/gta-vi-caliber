// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCJetSkiComponent.h"

UGTCJetSkiComponent::UGTCJetSkiComponent()
{
    // Light + snappy: quicker to plane, looser, a tighter turn, a shallow draft.
    EngineAccel = 1200.0f;
    ForwardDragPlowing = 0.7f;
    ForwardDragPlaning = 0.22f;
    LateralDrag = 4.0f;       // bites harder out of a slide
    PlaningSpeed = 400.0f;    // climbs onto plane sooner
    RudderAuthority = 1.8f;   // darts
    RudderFlowSpeed = 200.0f;
    DraftCm = 18.0f;
    CapsizeAngleRad = 1.0f;   // flips more easily

    // A small single-rider hull.
    HullSamplesCm = {
        FVector2D(90.0, 35.0), FVector2D(90.0, -35.0),
        FVector2D(-90.0, 35.0), FVector2D(-90.0, -35.0),
    };
}
