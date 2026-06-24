// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcCommute.h"

/**
 * Behavioural tests for FNpcCommute: car ownership is a deterministic minority,
 * driving is reserved for long trips to commute anchors (short hops and errands are
 * walked), and the enter/exit dwell timings behave.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcCommuteTest,
    "GTC.NPC.Decision.NpcCommute",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcCommuteTest::RunTest(const FString& Parameters)
{
    // --- Car ownership is deterministic and a minority -------------------------
    {
        TestEqual(TEXT("ownership is deterministic"),
            FNpcCommute::HasCar(123), FNpcCommute::HasCar(123));
        int32 Owners = 0;
        for (int32 Seed = 0; Seed < 2000; ++Seed)
        {
            if (FNpcCommute::HasCar(Seed)) { ++Owners; }
        }
        TestTrue(TEXT("some own cars"), Owners > 0);
        TestTrue(TEXT("but most do not (a minority drive)"), Owners < 1000);
    }

    // --- ShouldDrive: only far trips to commute anchors ------------------------
    {
        const double Far = FNpcCommute::DriveThresholdCm + 5000.0;
        const double Near = FNpcCommute::DriveThresholdCm - 1000.0;

        TestTrue(TEXT("an owner drives a long way home"),
            FNpcCommute::ShouldDrive(true, TEXT("home"), Far));
        TestTrue(TEXT("an owner drives a long way to the office"),
            FNpcCommute::ShouldDrive(true, TEXT("office"), Far));
        TestFalse(TEXT("nobody drives a short hop"),
            FNpcCommute::ShouldDrive(true, TEXT("home"), Near));
        TestFalse(TEXT("a carless citizen never drives"),
            FNpcCommute::ShouldDrive(false, TEXT("home"), Far));
        TestFalse(TEXT("you don't drive to the park bench"),
            FNpcCommute::ShouldDrive(true, TEXT("park"), Far));
        TestFalse(TEXT("you don't drive to a nearby diner"),
            FNpcCommute::ShouldDrive(true, TEXT("diner"), Far));
    }

    // --- Stage dwell timings ---------------------------------------------------
    {
        TestTrue(TEXT("entering has a dwell"), FNpcCommute::StageDuration(ENpcDriveStage::Entering) > 0.0);
        TestTrue(TEXT("exiting has a dwell"), FNpcCommute::StageDuration(ENpcDriveStage::Exiting) > 0.0);
        TestEqual(TEXT("walking/driving have no dwell"),
            FNpcCommute::StageDuration(ENpcDriveStage::Driving), 0.0);

        TestFalse(TEXT("enter dwell not yet elapsed"),
            FNpcCommute::StageDwellElapsed(ENpcDriveStage::Entering, FNpcCommute::EnterSeconds * 0.5));
        TestTrue(TEXT("enter dwell elapsed once the time is up"),
            FNpcCommute::StageDwellElapsed(ENpcDriveStage::Entering, FNpcCommute::EnterSeconds + 0.01));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
