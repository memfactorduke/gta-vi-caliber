// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../SkillsCoordinator.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Engine/GameInstance.h"

using GtcTest::Eps;

namespace
{
    // UGameInstanceSubsystem requires a UGameInstance outer (ClassWithin); a transient
    // one suffices for these owned-model behavior assertions.
    USkillsCoordinator* MakeSkillsCoordinatorForTest()
    {
        UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
        return NewObject<USkillsCoordinator>(GameInstance);
    }
}

// Subsystem BEHAVIOR tests (Wave 2 rule): re-author of the 4 funcs in the Godot
// oracle game/tests/unit/test_skills_coordinator.gd. These verify the subsystem owns
// its FPlayerSkills model and that activity events drive the model correctly — the
// scene-graph wiring (Node3D resolution, signal binding) is Wave 3, so the ported
// motion math is exercised through the subsystem's explicit driver methods.
// USkillsCoordinator is created with a transient UGameInstance outer (ClassWithin); the
// owned model is lazily ensured, so queries return the default skill set.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsCoordOnFootTrainsStaminaTest,
    "GTC.Systems.Skills.SkillsCoordinator.OnFootDistanceTrainsStamina",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsCoordOnFootTrainsStaminaTest::RunTest(const FString& Parameters)
{
    USkillsCoordinator* Coordinator = MakeSkillsCoordinatorForTest();
    // Walk 25m along X (visible player): trains stamina, not driving.
    Coordinator->TrainFromOnFootMove(FVector::ZeroVector, FVector(25.0, 0.0, 0.0), /*bVisible*/ true);
    TestTrue(TEXT("stamina > 0"), Coordinator->Level(TEXT("stamina")) > 0.0);
    TestEqual(TEXT("driving == 0"), Coordinator->Level(TEXT("driving")), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsCoordVehicleTrainsDrivingTest,
    "GTC.Systems.Skills.SkillsCoordinator.DrivenVehicleDistanceTrainsDrivingNotStamina",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsCoordVehicleTrainsDrivingTest::RunTest(const FString& Parameters)
{
    USkillsCoordinator* Coordinator = MakeSkillsCoordinatorForTest();
    // Driven vehicle moves 80m along X: trains driving, not stamina.
    Coordinator->TrainFromVehicleMove(FVector::ZeroVector, FVector(80.0, 0.0, 0.0));
    TestTrue(TEXT("driving > 0"), Coordinator->Level(TEXT("driving")) > 0.0);
    TestEqual(TEXT("stamina == 0"), Coordinator->Level(TEXT("stamina")), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsCoordHitTrainsShootingTest,
    "GTC.Systems.Skills.SkillsCoordinator.HitConfirmedTrainsShooting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsCoordHitTrainsShootingTest::RunTest(const FString& Parameters)
{
    USkillsCoordinator* Coordinator = MakeSkillsCoordinatorForTest();
    Coordinator->OnHitConfirmed();
    TestTrue(TEXT("shooting > 0"), Coordinator->Level(TEXT("shooting")) > 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsCoordSaveRoundTripTest,
    "GTC.Systems.Skills.SkillsCoordinator.SaveRoundTripPreservesSkills",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsCoordSaveRoundTripTest::RunTest(const FString& Parameters)
{
    USkillsCoordinator* Coordinator = MakeSkillsCoordinatorForTest();
    Coordinator->Restore({
        TPair<FString, double>(TEXT("driving"), 40.0),
        TPair<FString, double>(TEXT("shooting"), 25.0),
    });
    const TArray<TPair<FString, double>> Snapshot = Coordinator->SerializeState();

    USkillsCoordinator* Restored = MakeSkillsCoordinatorForTest();
    Restored->Restore(Snapshot);
    TestEqual(TEXT("driving == 40"), Restored->Level(TEXT("driving")), 40.0, Eps);
    TestEqual(TEXT("shooting == 25"), Restored->Level(TEXT("shooting")), 25.0, Eps);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
