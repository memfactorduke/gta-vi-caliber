// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../Door.h"

// Pure-core tests for the door's swing math (FDoorSwing). The live actor wiring
// (overlap selection, Interact toggling, the hinge SetRelativeRotation) needs a
// UWorld and is PIE-deferred, exactly the adapter boundary FInteraction documents;
// the assertable unit here is just the scene-free Advance/YawDegrees.
BEGIN_DEFINE_SPEC(FDoorSwingSpec, "GTC.World.Interaction.DoorSwing",
    EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)
END_DEFINE_SPEC(FDoorSwingSpec)

void FDoorSwingSpec::Define()
{
    // --- YawDegrees: fraction -> eased hinge angle --------------------------
    It("maps a shut door to zero degrees", [this]()
    {
        TestEqual(TEXT("Frac 0 yields 0 deg"), FDoorSwing::YawDegrees(0.0f, 90.0f), 0.0f);
    });

    It("maps a fully open door to the full angle", [this]()
    {
        TestEqual(TEXT("Frac 1 yields OpenAngleDeg"), FDoorSwing::YawDegrees(1.0f, 90.0f), 90.0f);
    });

    It("eases the midpoint to half the angle", [this]()
    {
        // smoothstep(0.5) = 0.5, so the midpoint sits exactly at half the angle.
        TestEqual(TEXT("Frac 0.5 yields half the angle"),
            FDoorSwing::YawDegrees(0.5f, 90.0f), 45.0f, 1e-3f);
    });

    It("clamps out-of-range fractions to the angle endpoints", [this]()
    {
        TestEqual(TEXT("negative frac clamps to 0 deg"), FDoorSwing::YawDegrees(-1.0f, 90.0f), 0.0f);
        TestEqual(TEXT("frac above 1 clamps to full angle"), FDoorSwing::YawDegrees(2.0f, 90.0f), 90.0f);
    });

    // --- Advance: step the open fraction toward its target ------------------
    It("opens partway over a sub-duration step", [this]()
    {
        // 0.4s of an 0.8s sweep = +0.5 of travel from shut.
        TestEqual(TEXT("half a duration opens to 0.5"),
            FDoorSwing::Advance(0.0f, 0.4f, /*bOpening=*/true, 0.8f), 0.5f, 1e-3f);
    });

    It("clamps opening at fully open and never overshoots", [this]()
    {
        TestEqual(TEXT("a long open step clamps to 1"),
            FDoorSwing::Advance(0.8f, 10.0f, /*bOpening=*/true, 0.8f), 1.0f);
    });

    It("closes toward shut and clamps at zero", [this]()
    {
        TestEqual(TEXT("a long close step clamps to 0"),
            FDoorSwing::Advance(0.2f, 10.0f, /*bOpening=*/false, 0.8f), 0.0f);
    });

    It("is monotonic: opening never decreases, closing never increases", [this]()
    {
        const float Opened = FDoorSwing::Advance(0.3f, 0.1f, /*bOpening=*/true, 0.8f);
        TestTrue(TEXT("opening raises the fraction"), Opened >= 0.3f);
        const float Closed = FDoorSwing::Advance(0.3f, 0.1f, /*bOpening=*/false, 0.8f);
        TestTrue(TEXT("closing lowers the fraction"), Closed <= 0.3f);
    });

    It("snaps to the target when the duration is non-positive", [this]()
    {
        TestEqual(TEXT("zero duration snaps open"),
            FDoorSwing::Advance(0.0f, 0.016f, /*bOpening=*/true, 0.0f), 1.0f);
        TestEqual(TEXT("zero duration snaps shut"),
            FDoorSwing::Advance(1.0f, 0.016f, /*bOpening=*/false, 0.0f), 0.0f);
    });

    It("holds clamped state when no time elapses", [this]()
    {
        TestEqual(TEXT("zero delta holds the fraction"),
            FDoorSwing::Advance(0.42f, 0.0f, /*bOpening=*/true, 0.8f), 0.42f);
    });
}

#endif // WITH_AUTOMATION_TESTS
