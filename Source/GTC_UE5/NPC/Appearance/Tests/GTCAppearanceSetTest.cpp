// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../GTCAppearanceSet.h"
#include "UObject/Package.h"

/**
 * Safety + selection tests for UGTCAppearanceSet (prefix GTC.NPC.Appearance).
 * Asset loading itself needs cooked content, so these cover the logic that must
 * hold with NO assets present: empty pools never crash and return null/fallback,
 * and skin-tone selection wraps deterministically by index.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGTCAppearanceSetTest,
    "GTC.NPC.Appearance.AppearanceSet",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGTCAppearanceSetTest::RunTest(const FString& Parameters)
{
    UGTCAppearanceSet* Set = NewObject<UGTCAppearanceSet>(GetTransientPackage());
    TestNotNull(TEXT("appearance set constructs"), Set);
    if (!Set)
    {
        return false;
    }

    // Empty pools: every resolver is null-safe and returns the fallback / null.
    TestNull(TEXT("empty body pool -> null"), Set->ResolveBody(0));
    TestNull(TEXT("empty head pool -> null"), Set->ResolveHead(3));
    TestNull(TEXT("empty hero head pool -> null"), Set->ResolveHeroHead(3));
    TestNull(TEXT("empty hair pool -> null"), Set->ResolveHair(2));
    TestNull(TEXT("empty outfit pool -> null"), Set->ResolveOutfit(4));
    TestNull(TEXT("unset anim class -> null"), Set->ResolveBodyAnimClass());
    const FLinearColor Fallback(0.2f, 0.4f, 0.6f, 1.0f);
    TestEqual(TEXT("empty skin pool -> fallback"), Set->ResolveSkinTone(5, Fallback), Fallback);

    // Skin-tone selection wraps by index and is deterministic.
    Set->SkinTones = {FLinearColor::Red, FLinearColor::Green, FLinearColor::Blue};
    TestEqual(TEXT("skin index 0"), Set->ResolveSkinTone(0, Fallback), FLinearColor::Red);
    TestEqual(TEXT("skin index 1"), Set->ResolveSkinTone(1, Fallback), FLinearColor::Green);
    TestEqual(TEXT("skin index wraps (3 -> 0)"), Set->ResolveSkinTone(3, Fallback), FLinearColor::Red);
    TestEqual(TEXT("skin index wraps (7 -> 1)"), Set->ResolveSkinTone(7, Fallback), FLinearColor::Green);
    // Same index -> same tone (determinism).
    TestEqual(TEXT("skin selection deterministic"),
        Set->ResolveSkinTone(11, Fallback), Set->ResolveSkinTone(11, Fallback));

    return true;
}

#endif // WITH_AUTOMATION_TESTS
