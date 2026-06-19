// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../TrafficBark.h"

/**
 * Behavioural tests for FTrafficBark: every voice has a startle line (generic
 * fallback), selection wraps by index, voice-flavoured banks differ from generic,
 * and the mime stays silent.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficBarkTest,
	"GTC.NPC.Dialogue.TrafficBark",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficBarkTest::RunTest(const FString& Parameters)
{
	// --- An unknown voice falls back to the generic bank ----------------------
	{
		const int32 Count = FTrafficBark::Count(TEXT("nobody_special"));
		TestTrue(TEXT("generic bank has lines"), Count > 0);
		TestNotEqual(TEXT("generic line non-empty"), FTrafficBark::Line(TEXT("nobody_special"), 0), FString());
		TestEqual(TEXT("index wraps (posmod)"),
			FTrafficBark::Line(TEXT("nobody_special"), Count), FTrafficBark::Line(TEXT("nobody_special"), 0));
		TestNotEqual(TEXT("negative index safe"), FTrafficBark::Line(TEXT("nobody_special"), -1), FString());
	}

	// --- A voice-flavoured bank differs from generic --------------------------
	{
		TestNotEqual(TEXT("doomsday startle != generic startle"),
			FTrafficBark::Line(TEXT("doomsday"), 0), FTrafficBark::Line(TEXT("nobody_special"), 0));
		TestTrue(TEXT("doomsday has its own bank"), FTrafficBark::Count(TEXT("doomsday")) > 0);
	}

	// --- The mime stays silent ------------------------------------------------
	{
		TestEqual(TEXT("mime has no traffic lines"), FTrafficBark::Count(TEXT("mime")), 0);
		TestEqual(TEXT("mime traffic line is empty"), FTrafficBark::Line(TEXT("mime"), 0), FString());
	}

	// --- Determinism ----------------------------------------------------------
	{
		TestEqual(TEXT("same voice+index is the same line"),
			FTrafficBark::Line(TEXT("influencer"), 3), FTrafficBark::Line(TEXT("influencer"), 3));
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
