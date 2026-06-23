// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../GTCGameInstance.h"
#include "../../Systems/Save/SaveSubsystem.h"

// The GameInstance is the persistent owner of player save state. It registers a "player"
// section hook with USaveSubsystem; this first slice persists the created look. Driven
// through BuildSaveText -> change -> ApplySaveText (no file IO). UGTCGameInstance is a
// UGameInstance, so it can serve as the USaveSubsystem's (ClassWithin) outer directly.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGameInstancePlayerLookSaveRoundTripTest,
    "GTC.Core.GameInstance.PlayerLookSaveRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGameInstancePlayerLookSaveRoundTripTest::RunTest(const FString& Parameters)
{
    UGTCGameInstance* GI = NewObject<UGTCGameInstance>(GetTransientPackage());
    USaveSubsystem* Save = NewObject<USaveSubsystem>(GI);

    TestTrue(TEXT("player hook registers"), GI->RegisterSaveHooks(Save));
    TestFalse(TEXT("null save rejected"), GI->RegisterSaveHooks(nullptr));

    FGTCPlayerLook Look;
    Look.Body = 2;
    Look.Face = 3;
    Look.Hair = 1;
    Look.Outfit = 4;
    Look.Skin = 5;
    GI->SetSavedLook(Look);
    GI->SetSavedThrowableAmmo(1, 4, 0);
    // No pawn in a headless test, so set the transform directly (simulates the save-time
    // pull from a live pawn).
    GI->SetSavedTransform(FVector(100.0, 200.0, 300.0), 45.0f);

    const FString Text = Save->BuildSaveText();

    // Mutate the live look + ammo + transform, then load the saved ones back over them.
    FGTCPlayerLook Other;
    Other.Body = 9;
    GI->SetSavedLook(Other);
    GI->SetSavedThrowableAmmo(3, 3, 3);
    GI->SetSavedTransform(FVector(-1.0, -1.0, -1.0), 0.0f);
    TestEqual(TEXT("look changed before load"), GI->GetSavedLook().Body, 9);

    TestTrue(TEXT("apply succeeded"), Save->ApplySaveText(Text));
    TestTrue(TEXT("look valid after load"), GI->HasSavedLook());
    TestEqual(TEXT("body round-trips"), GI->GetSavedLook().Body, 2);
    TestEqual(TEXT("face round-trips"), GI->GetSavedLook().Face, 3);
    TestEqual(TEXT("skin round-trips"), GI->GetSavedLook().Skin, 5);

    TestTrue(TEXT("ammo valid after load"), GI->HasSavedThrowableAmmo());
    int32 F = -1, G = -1, M = -1;
    GI->GetSavedThrowableAmmo(F, G, M);
    TestEqual(TEXT("flashbang ammo round-trips"), F, 1);
    TestEqual(TEXT("grenade ammo round-trips"), G, 4);
    TestEqual(TEXT("molotov ammo round-trips"), M, 0);

    TestTrue(TEXT("transform valid after load"), GI->HasSavedTransform());
    FVector Loc = FVector::ZeroVector;
    float Yaw = 0.0f;
    GI->GetSavedTransform(Loc, Yaw);
    TestEqual(TEXT("pos x round-trips"), Loc.X, 100.0, 0.01);
    TestEqual(TEXT("pos y round-trips"), Loc.Y, 200.0, 0.01);
    TestEqual(TEXT("pos z round-trips"), Loc.Z, 300.0, 0.01);
    TestEqual(TEXT("yaw round-trips"), Yaw, 45.0f, 0.01f);

    GI->UnregisterSaveHooks();
    return true;
}

#endif // WITH_AUTOMATION_TESTS
