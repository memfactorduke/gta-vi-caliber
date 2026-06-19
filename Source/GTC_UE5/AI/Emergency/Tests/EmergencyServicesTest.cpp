// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../EmergencyServices.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Parity tests for FEmergencyServices, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_emergency_services.gd. Tolerance mirrors the oracle's
 * is_equal_approx (Eps = 1e-4); enum equality, infinity, and empty/index cases
 * assert exactly.
 *
 * Grouped into the 6 oracle sections: ServiceFor, Eta, NearestResponder,
 * ShouldDispatch, ReviveChance, ResponseTimer (prefix GTC.AI.Emergency).
 *
 * PURE-CORE boundary: responder spawning / world positioning / navmesh distance
 * / per-frame tick wiring are the deferred Wave-3 adapter and are NOT tested.
 */

// --- service_for mapping ----------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEmergencyServicesServiceForTest,
    "GTC.AI.Emergency.ServiceFor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEmergencyServicesServiceForTest::RunTest(const FString& Parameters)
{
    // test_service_for_fire_is_fire_truck
    TestTrue(TEXT("fire is fire truck"),
        FEmergencyServices::ServiceFor(EEmergencyIncident::Fire) == EEmergencyService::FireTruck);
    // test_service_for_injury_and_wreck_are_ambulance
    TestTrue(TEXT("injury is ambulance"),
        FEmergencyServices::ServiceFor(EEmergencyIncident::Injury) == EEmergencyService::Ambulance);
    TestTrue(TEXT("wreck is ambulance"),
        FEmergencyServices::ServiceFor(EEmergencyIncident::Wreck) == EEmergencyService::Ambulance);
    // test_service_for_shooting_is_police_backup
    TestTrue(TEXT("shooting is police backup"),
        FEmergencyServices::ServiceFor(EEmergencyIncident::Shooting) == EEmergencyService::PoliceBackup);
    // test_service_for_unknown_defaults_to_ambulance
    TestTrue(TEXT("unknown defaults to ambulance"),
        FEmergencyServices::ServiceForInt(999) == EEmergencyService::Ambulance);

    return true;
}

// --- eta --------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEmergencyServicesEtaTest,
    "GTC.AI.Emergency.Eta",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEmergencyServicesEtaTest::RunTest(const FString& Parameters)
{
    // test_eta_is_distance_over_speed: 30 m on X at 10 m/s = 3.0 s; Y ignored.
    TestEqual(TEXT("distance over speed"),
        FEmergencyServices::EtaSeconds(FVector(0, 5, 0), FVector(30, 99, 0), 10.0), 3.0, Eps);
    // test_eta_uses_xz_plane: 3-4-5 on XZ, hypotenuse 5 at speed 5 = 1.0 s.
    TestEqual(TEXT("uses xz plane"),
        FEmergencyServices::EtaSeconds(FVector::ZeroVector, FVector(3, 0, 4), 5.0), 1.0, Eps);
    // test_eta_nonpositive_speed_guarded: zero and negative speed -> +infinity.
    const double Zero = FEmergencyServices::EtaSeconds(FVector::ZeroVector, FVector(10, 0, 0), 0.0);
    const double Negative = FEmergencyServices::EtaSeconds(FVector::ZeroVector, FVector(10, 0, 0), -4.0);
    TestFalse(TEXT("zero speed is infinite"), FMath::IsFinite(Zero));
    TestFalse(TEXT("negative speed is infinite"), FMath::IsFinite(Negative));

    return true;
}

// --- nearest_responder ------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEmergencyServicesNearestResponderTest,
    "GTC.AI.Emergency.NearestResponder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEmergencyServicesNearestResponderTest::RunTest(const FString& Parameters)
{
    // test_nearest_responder_picks_closest
    {
        TArray<FEmergencyServices::FResponder> Responders;
        Responders.Add({FVector(100, 0, 0), true, EEmergencyService::FireTruck});
        Responders.Add({FVector(5, 0, 0), true, EEmergencyService::Ambulance});
        Responders.Add({FVector(40, 0, 0), true, EEmergencyService::Ambulance});
        const int32 Best = FEmergencyServices::NearestResponderIndex(FVector::ZeroVector, Responders);
        TestEqual(TEXT("picks closest index"), Best, 1);
        TestEqual(TEXT("closest pos x"), Responders[Best].Pos.X, 5.0, Eps);
    }
    // test_nearest_responder_carries_service_through
    {
        TArray<FEmergencyServices::FResponder> Responders;
        Responders.Add({FVector(2, 0, 0), true, EEmergencyService::FireTruck});
        const int32 Best = FEmergencyServices::NearestResponderIndex(FVector::ZeroVector, Responders);
        TestTrue(TEXT("carries service through"),
            Responders[Best].Service == EEmergencyService::FireTruck);
    }
    // test_nearest_responder_empty_is_blank
    {
        TArray<FEmergencyServices::FResponder> Responders;
        TestEqual(TEXT("empty is INDEX_NONE"),
            FEmergencyServices::NearestResponderIndex(FVector::ZeroVector, Responders), (int32)INDEX_NONE);
    }
    // test_nearest_responder_skips_entries_without_pos
    {
        TArray<FEmergencyServices::FResponder> Responders;
        FEmergencyServices::FResponder NoPos;
        NoPos.bHasPos = false;
        NoPos.Service = EEmergencyService::Ambulance;
        Responders.Add(NoPos);
        TestEqual(TEXT("no-pos is INDEX_NONE"),
            FEmergencyServices::NearestResponderIndex(FVector::ZeroVector, Responders), (int32)INDEX_NONE);
    }

    return true;
}

// --- should_dispatch --------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEmergencyServicesShouldDispatchTest,
    "GTC.AI.Emergency.ShouldDispatch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEmergencyServicesShouldDispatchTest::RunTest(const FString& Parameters)
{
    // test_should_dispatch_medical_when_safe
    TestTrue(TEXT("medical when safe"),
        FEmergencyServices::ShouldDispatch(EEmergencyIncident::Injury, true, 0));
    // test_should_dispatch_medical_suppressed_when_player_caused_hot
    TestFalse(TEXT("medical suppressed player-caused hot"),
        FEmergencyServices::ShouldDispatch(EEmergencyIncident::Injury, true, 4));
    // test_should_dispatch_medical_at_hot_scene_player_not_caused
    TestTrue(TEXT("medical at hot scene not player caused"),
        FEmergencyServices::ShouldDispatch(EEmergencyIncident::Fire, false, 4));
    // test_should_dispatch_shooting_always_gets_backup
    TestTrue(TEXT("shooting always gets backup"),
        FEmergencyServices::ShouldDispatch(EEmergencyIncident::Shooting, true, 6));

    return true;
}

// --- revive_chance ----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEmergencyServicesReviveChanceTest,
    "GTC.AI.Emergency.ReviveChance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEmergencyServicesReviveChanceTest::RunTest(const FString& Parameters)
{
    // test_revive_chance_falls_with_severity
    const double Light = FEmergencyServices::ReviveChance(0.2);
    const double Heavy = FEmergencyServices::ReviveChance(0.8);
    TestEqual(TEXT("light severity"), Light, 0.8, Eps);
    TestEqual(TEXT("heavy severity"), Heavy, 0.2, Eps);
    TestTrue(TEXT("light > heavy"), Light > Heavy);
    // test_revive_chance_edges: fatal unrevivable (0.0); clamps below 0 to 1.0.
    TestEqual(TEXT("fatal is zero"), FEmergencyServices::ReviveChance(1.0), 0.0, Eps);
    TestEqual(TEXT("below zero clamps to one"), FEmergencyServices::ReviveChance(-0.5), 1.0, Eps);

    return true;
}

// --- stateful response timer ------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEmergencyServicesResponseTimerTest,
    "GTC.AI.Emergency.ResponseTimer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEmergencyServicesResponseTimerTest::RunTest(const FString& Parameters)
{
    // test_timer_starts_idle
    {
        FEmergencyServices Unit(6.0);
        TestFalse(TEXT("starts not arrived"), Unit.HasArrived());
        TestEqual(TEXT("starts at zero progress"), Unit.Progress(), 0.0, Eps);
    }
    // test_tick_before_begin_is_noop
    {
        FEmergencyServices Unit(6.0);
        Unit.Tick(10.0);
        TestFalse(TEXT("tick before begin not arrived"), Unit.HasArrived());
        TestEqual(TEXT("tick before begin zero progress"), Unit.Progress(), 0.0, Eps);
    }
    // test_timer_arrives_after_delay
    {
        FEmergencyServices Unit(4.0);
        Unit.Begin();
        Unit.Tick(2.0);
        const bool bMidway = !Unit.HasArrived();
        Unit.Tick(2.0);
        TestTrue(TEXT("midway not arrived"), bMidway);
        TestTrue(TEXT("arrives after delay"), Unit.HasArrived());
    }
    // test_progress_ramps
    {
        FEmergencyServices Unit(4.0);
        Unit.Begin();
        Unit.Tick(1.0);
        TestEqual(TEXT("progress ramps"), Unit.Progress(), 0.25, Eps);
    }
    // test_treating_only_after_arrival
    {
        FEmergencyServices Unit(4.0);
        Unit.Begin();
        Unit.Tick(1.0);
        const bool bBefore = !Unit.Treating();
        Unit.Tick(5.0);
        TestTrue(TEXT("not treating before arrival"), bBefore);
        TestTrue(TEXT("treating after arrival"), Unit.Treating());
    }
    // test_arrive_once_and_progress_full
    {
        FEmergencyServices Unit(4.0);
        Unit.Begin();
        Unit.Tick(100.0);
        const bool bArrived = Unit.HasArrived();
        Unit.Tick(100.0);
        TestTrue(TEXT("arrived after big tick"), bArrived);
        TestTrue(TEXT("stays arrived"), Unit.HasArrived());
        TestEqual(TEXT("progress full"), Unit.Progress(), 1.0, Eps);
    }
    // test_cancel_resets_to_idle
    {
        FEmergencyServices Unit(4.0);
        Unit.Begin();
        Unit.Tick(5.0);
        Unit.Cancel();
        TestFalse(TEXT("cancel not arrived"), Unit.HasArrived());
        TestEqual(TEXT("cancel zero progress"), Unit.Progress(), 0.0, Eps);
    }
    // test_reset_clears_and_allows_rebegin
    {
        FEmergencyServices Unit(4.0);
        Unit.Begin();
        Unit.Tick(5.0);
        Unit.Reset();
        Unit.Begin();
        Unit.Tick(2.0);
        TestEqual(TEXT("rebegin progress"), Unit.Progress(), 0.5, Eps);
        TestFalse(TEXT("rebegin not arrived"), Unit.HasArrived());
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
